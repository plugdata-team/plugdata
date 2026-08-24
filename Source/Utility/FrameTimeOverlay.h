/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

// On-screen frame-time overlay for NVGSurface.
//
// This is drawn on the RENDER THREAD with direct (non-async) nanovg primitive
// calls, straight into the persistent main framebuffer just before it is blitted
// to screen (see NVGSurface::drawFrameTimeOverlay). Text rendering was removed
// from nanovg, and JUCE's Graphics path records into the async command buffer on
// the message thread - neither is usable here - so glyphs are drawn from a tiny
// built-in 5x7 pixel font as filled rectangles.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

#include <nanovg.h>

class FrameTimeOverlay final {
public:
    // --- timing intake -----------------------------------------------------

    // Render thread: how long the last real frame took to replay/render (ms).
    void addFrame(double renderMs)
    {
        std::lock_guard<std::mutex> const sl(mutex);

        renderMs = clampMs(renderMs);

        renderHead = (renderHead + 1) % historySize;
        renderTimes[renderHead] = static_cast<float>(renderMs);
        renderCount = std::min(renderCount + 1, historySize);

        currentRenderMs = renderMs;

        peakRenderMs = 0.0;
        minRenderMs = renderMs;
        for (int i = 0; i < renderCount; ++i) {
            auto const s = renderTimes[(renderHead - i + historySize) % historySize];
            peakRenderMs = std::max(peakRenderMs, static_cast<double>(s));
            minRenderMs = std::min(minRenderMs, static_cast<double>(s));
        }

        ++frameCount;
    }

    // Message thread: how long recording the last frame took on the message
    // thread (ms), measured in NVGSurface::recordFrame.
    void setPrepareTime(double prepareMs)
    {
        std::lock_guard<std::mutex> const sl(mutex);
        currentPrepareMs = clampMs(prepareMs);
    }

    // Render thread: elapsed GPU time reported asynchronously by the active
    // graphics backend for the most recently completed real frame (ms).
    void setGpuTime(double gpuMs)
    {
        std::lock_guard<std::mutex> const sl(mutex);
        currentGpuMs = clampMs(gpuMs);
        hasGpuTime = true;
    }

    // Message thread (VBlankAttachment): raw display refresh interval, tracked
    // from consecutive vblank timestamps regardless of whether a frame was
    // produced. This is the true refresh period used for the max-fps estimate.
    void addVBlank(double timestampSec)
    {
        std::lock_guard<std::mutex> const sl(mutex);

        if (!std::isfinite(timestampSec) || timestampSec <= 0.0)
            return;

        if (lastVBlankSec > 0.0) {
            auto const dtMs = (timestampSec - lastVBlankSec) * 1000.0;
            // Ignore absurd gaps (occluded window, display sleep, tab switch).
            if (std::isfinite(dtMs) && dtMs > 0.0 && dtMs < 200.0)
                averageVsyncMs = averageVsyncMs <= 0.0 ? dtMs : averageVsyncMs * 0.9 + dtMs * 0.1;
        }

        lastVBlankSec = timestampSec;
    }

    static auto sizeFor(float scale)
    {
        struct Size final {
            float width;
            float height;
        };

        auto const u = unit(scale);
        return Size{ panelCells * u, panelRows * u };
    }

    void draw(NVGcontext* nvg, float const x, float const y, float const scale) const
    {
        auto const snapshot = getSnapshot();

        auto const u = unit(scale);
        auto const w = panelCells * u;
        auto const h = panelRows * u;
        auto const pad = 6.0f * u;

        nvgDrawRoundedRect(nvg, x, y, w, h, nvgRGBA(24, 24, 28, 255), nvgRGBA(255, 255, 255, 46), 4.0f * u);

        auto const left = x + pad;
        auto const right = x + w - pad;

        auto const label = nvgRGBA(200, 202, 210, 255);
        auto const value = nvgRGBA(255, 255, 255, 235);

        char buf[32];

        float rowY = y + pad;
        drawText(nvg, left, rowY, u, "FRAME", label);
        std::snprintf(buf, sizeof(buf), "#%llu", static_cast<unsigned long long>(snapshot.frameCount));
        drawTextRight(nvg, right, rowY, u, buf, value);

        auto const vsyncMs = snapshot.averageVsyncMs;
        auto const vsyncFps = vsyncMs > 0.0 ? 1000.0 / vsyncMs : 0.0;
        auto const budgetMs = std::max({ vsyncMs, snapshot.currentRenderMs, snapshot.currentGpuMs });
        auto const maxFps = budgetMs > 0.0 ? 1000.0 / budgetMs : 0.0;

        auto const renderCpu = vsyncMs > 0.0 ? snapshot.currentRenderMs / vsyncMs * 100.0 : 0.0;
        auto const prepareCpu = vsyncMs > 0.0 ? snapshot.currentPrepareMs / vsyncMs * 100.0 : 0.0;
        auto const gpuPercent = vsyncMs > 0.0 ? snapshot.currentGpuMs / vsyncMs * 100.0 : 0.0;

        auto const drawRow = [&](char const* name, char const* val, NVGcolor valCol) {
            rowY += 12.0f * u;
            drawText(nvg, left, rowY, u, name, label);
            drawTextRight(nvg, right, rowY, u, val, valCol);
        };

        std::snprintf(buf, sizeof(buf), "%.2f MS", snapshot.currentRenderMs);
        drawRow("RENDER", buf, value);

        std::snprintf(buf, sizeof(buf), "%.2f MS", snapshot.minRenderMs);
        drawRow("RENDER MIN", buf, value);

        std::snprintf(buf, sizeof(buf), "%.2f MS", snapshot.peakRenderMs);
        drawRow("RENDER MAX", buf, value);

        std::snprintf(buf, sizeof(buf), "%.0f%%", renderCpu);
        drawRow("RENDER CPU", buf, value);

        if (snapshot.hasGpuTime)
            std::snprintf(buf, sizeof(buf), "%.2f MS", snapshot.currentGpuMs);
        else
            std::snprintf(buf, sizeof(buf), "-- MS");
        drawRow("GPU TIME", buf, value);

        if (snapshot.hasGpuTime && vsyncMs > 0.0)
            std::snprintf(buf, sizeof(buf), "%.0f%%", gpuPercent);
        else
            std::snprintf(buf, sizeof(buf), "--%%");
        drawRow("GPU", buf, value);

        std::snprintf(buf, sizeof(buf), "%.2f MS", snapshot.currentPrepareMs);
        drawRow("PREPARE", buf, value);

        std::snprintf(buf, sizeof(buf), "%.0f%%", prepareCpu);
        drawRow("PREPARE CPU", buf, value);
        if (vsyncFps > 0.0)
            std::snprintf(buf, sizeof(buf), "%.1f FPS", vsyncFps);
        else
            std::snprintf(buf, sizeof(buf), "-- FPS");
        drawRow("VSYNC", buf, value);
        std::snprintf(buf, sizeof(buf), "%.1f FPS", maxFps);
        drawRow("MAX", buf, value);

        // Render-time history graph with a guide line at the vsync frame budget.
        auto const graphX = left;
        auto const graphW = right - left;
        auto const graphY = rowY + 8.0f * u;
        auto const graphH = 20.0f * u;

        nvgFillColor(nvg, nvgRGBA(255, 255, 255, 18));
        nvgFillRoundedRect(nvg, graphX, graphY, graphW, graphH, 2.0f * u);

        auto const graphMax = std::max({ 8.0, vsyncMs * 1.5, snapshot.peakRenderMs });

        if (vsyncMs > 0.0) {
            auto const guideY = graphY + graphH - static_cast<float>(std::clamp(vsyncMs / graphMax, 0.0, 1.0)) * graphH;
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, graphX, guideY);
            nvgLineTo(nvg, graphX + graphW, guideY);
            nvgStrokeColor(nvg, nvgRGBA(255, 255, 255, 60));
            nvgStrokeWidth(nvg, std::max(1.0f, u * 0.5f));
            nvgStroke(nvg);
        }

        if (snapshot.renderCount > 0) {
            nvgBeginPath(nvg);
            for (int i = 0; i < snapshot.renderCount; ++i) {
                auto const sample = std::clamp(static_cast<double>(snapshot.renderTimes[i]) / graphMax, 0.0, 1.0);
                auto const px = graphX + (snapshot.renderCount == 1 ? graphW : (static_cast<float>(i) / static_cast<float>(snapshot.renderCount - 1)) * graphW);
                auto const py = graphY + graphH - static_cast<float>(sample) * graphH;
                if (i == 0)
                    nvgMoveTo(nvg, px, py);
                else
                    nvgLineTo(nvg, px, py);
            }
            nvgStrokeColor(nvg, nvgRGBA(240, 201, 106, 255));
            nvgStrokeWidth(nvg, std::max(1.0f, u * 0.6f));
            nvgStroke(nvg);
        }
    }

private:
    static constexpr int historySize = 96;
    static constexpr float panelCells = 128.0f;   // width  in font-pixel units
    static constexpr float panelRows = 162.0f;    // height in font-pixel units

    // One font-pixel, in device pixels, at the given render scale.
    static float unit(float scale) { return std::max(1.0f, std::round(scale)); }

    static double clampMs(double ms)
    {
        if (!std::isfinite(ms) || ms < 0.0)
            return 0.0;
        return std::min(ms, 1000.0);
    }

    struct Snapshot final {
        float renderTimes[historySize]{};
        int renderCount = 0;
        std::uint64_t frameCount = 0;
        double currentRenderMs = 0.0;
        double currentPrepareMs = 0.0;
        double currentGpuMs = 0.0;
        double peakRenderMs = 0.0;
        double minRenderMs = 0.0;
        double averageVsyncMs = 0.0;
        bool hasGpuTime = false;
    };

    Snapshot getSnapshot() const
    {
        std::lock_guard<std::mutex> const sl(mutex);

        Snapshot s;
        s.renderCount = renderCount;
        s.frameCount = frameCount;
        s.currentRenderMs = currentRenderMs;
        s.currentPrepareMs = currentPrepareMs;
        s.currentGpuMs = currentGpuMs;
        s.peakRenderMs = peakRenderMs;
        s.minRenderMs = minRenderMs;
        s.averageVsyncMs = averageVsyncMs;
        s.hasGpuTime = hasGpuTime;

        auto const first = (renderHead - renderCount + 1 + historySize) % historySize;
        for (int i = 0; i < renderCount; ++i)
            s.renderTimes[i] = renderTimes[(first + i) % historySize];

        return s;
    }

    static const std::uint8_t* glyph(char c)
    {
        switch (c) {
        case '0': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 }; return g; }
        case '1': { static const std::uint8_t g[7] = { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 }; return g; }
        case '2': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111 }; return g; }
        case '3': { static const std::uint8_t g[7] = { 0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110 }; return g; }
        case '4': { static const std::uint8_t g[7] = { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 }; return g; }
        case '5': { static const std::uint8_t g[7] = { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 }; return g; }
        case '6': { static const std::uint8_t g[7] = { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 }; return g; }
        case '7': { static const std::uint8_t g[7] = { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 }; return g; }
        case '8': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 }; return g; }
        case '9': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 }; return g; }
        case 'A': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 }; return g; }
        case 'C': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 }; return g; }
        case 'D': { static const std::uint8_t g[7] = { 0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100 }; return g; }
        case 'E': { static const std::uint8_t g[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 }; return g; }
        case 'F': { static const std::uint8_t g[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 }; return g; }
        case 'G': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110 }; return g; }
        case 'I': { static const std::uint8_t g[7] = { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 }; return g; }
        case 'M': { static const std::uint8_t g[7] = { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 }; return g; }
        case 'N': { static const std::uint8_t g[7] = { 0b10001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10001, 0b10001 }; return g; }
        case 'P': { static const std::uint8_t g[7] = { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 }; return g; }
        case 'R': { static const std::uint8_t g[7] = { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 }; return g; }
        case 'S': { static const std::uint8_t g[7] = { 0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110 }; return g; }
        case 'T': { static const std::uint8_t g[7] = { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 }; return g; }
        case 'U': { static const std::uint8_t g[7] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 }; return g; }
        case 'V': { static const std::uint8_t g[7] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 }; return g; }
        case 'X': { static const std::uint8_t g[7] = { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 }; return g; }
        case 'Y': { static const std::uint8_t g[7] = { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 }; return g; }
        case '.': { static const std::uint8_t g[7] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100 }; return g; }
        case '-': { static const std::uint8_t g[7] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 }; return g; }
        case '#': { static const std::uint8_t g[7] = { 0b01010, 0b01010, 0b11111, 0b01010, 0b11111, 0b01010, 0b01010 }; return g; }
        case '%': { static const std::uint8_t g[7] = { 0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011 }; return g; }
        default: return nullptr; // space / unknown: advance only
        }
    }

    static float textWidth(char const* s, float u) { return static_cast<float>(std::strlen(s)) * 6.0f * u; }

    static void drawText(NVGcontext* nvg, float x, float const y, float const u, char const* s, NVGcolor col)
    {
        nvgBeginPath(nvg);
        for (char const* p = s; *p != '\0'; ++p) {
            if (auto const* rows = glyph(*p)) {
                for (int r = 0; r < 7; ++r) {
                    auto const bits = rows[r];
                    for (int c = 0; c < 5; ++c) {
                        if (bits & (1u << (4 - c)))
                            nvgRect(nvg, x + static_cast<float>(c) * u, y + static_cast<float>(r) * u, u, u);
                    }
                }
            }
            x += 6.0f * u;
        }
        nvgFillColor(nvg, col);
        nvgFill(nvg);
    }

    static void drawTextRight(NVGcontext* nvg, float const rightX, float const y, float const u, char const* s, NVGcolor col)
    {
        drawText(nvg, rightX - textWidth(s, u), y, u, s, col);
    }

    mutable std::mutex mutex;

    float renderTimes[historySize]{};
    int renderHead = -1;
    int renderCount = 0;
    std::uint64_t frameCount = 0;
    double currentRenderMs = 0.0;
    double currentPrepareMs = 0.0;
    double currentGpuMs = 0.0;
    double peakRenderMs = 0.0;
    double minRenderMs = 0.0;
    double lastVBlankSec = 0.0;
    double averageVsyncMs = 0.0;
    bool hasGpuTime = false;
};
