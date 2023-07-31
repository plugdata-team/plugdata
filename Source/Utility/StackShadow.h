#pragma once

// A Stack Blur implementation, and a copy of JUCE's DropShadower that uses StackBlur
// We use stack blur for the shadows in the UI, and the window shadows on Linux and Windows

// The Stack Blur Algorithm was invented by Mario Klingemann,
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// Stackblur algorithm by Mario Klingemann
// Details here:
// http://www.quasimondo.com/StackBlurForCanvas/StackBlurDemo.html
// C++ implemenation base from:
// https://gist.github.com/benjamin9999/3809142
// http://www.antigrain.com/__code/include/agg_blur.h.html

#if JUCE_WINDOWS
#    include <juce_gui_basics/native/juce_win32_ScopedThreadDPIAwarenessSetter.h>
#endif

#include <juce_audio_processors/juce_audio_processors.h>

class StackShadow {

    static inline unsigned short const stackblur_mul[255] = {
        512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512,
        454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312, 292, 273, 512,
        482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456,
        437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512,
        497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328,
        320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496, 485, 475, 465, 456,
        446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335,
        329, 323, 318, 312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512,
        505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405,
        399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328,
        324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271,
        268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456,
        451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392, 388,
        385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335,
        332, 329, 326, 323, 320, 318, 315, 312, 310, 307, 304, 302, 299, 297, 294, 292,
        289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259
    };

    static inline unsigned char const stackblur_shr[255] = {
        9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
        17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
        19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
        21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
        21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
        22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
        22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
        23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
    };

public:
    static void applyStackBlurBW(Image& img, unsigned int radius)
    {
        auto const w = (unsigned int)img.getWidth();
        auto const h = (unsigned int)img.getHeight();

        Image::BitmapData data(img, Image::BitmapData::readWrite);

        radius = jlimit(2u, 254u, radius);

        unsigned char stack[(254 * 2 + 1) * 1];

        unsigned int x, y, xp, yp, i, sp, stack_start;

        unsigned char* stack_ptr = nullptr;
        unsigned char* src_ptr = nullptr;
        unsigned char* dst_ptr = nullptr;

        unsigned long sum, sum_in, sum_out;

        unsigned int wm = w - 1;
        unsigned int hm = h - 1;
        auto w1 = (unsigned int)data.lineStride;
        unsigned int div = (unsigned int)(radius * 2) + 1;
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        for (y = 0; y < h; ++y) {
            sum = sum_in = sum_out = 0;

            src_ptr = data.getLinePointer(int(y));

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[i];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (i + 1);
                sum_out += src_ptr[0];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= wm)
                    src_ptr += 1;

                stack_ptr = &stack[1 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (radius + 1 - i);
                sum_in += src_ptr[0];
            }

            sp = radius;
            xp = radius;
            if (xp > wm)
                xp = wm;

            src_ptr = data.getLinePointer(int(y)) + (unsigned int)data.pixelStride * xp;
            dst_ptr = data.getLinePointer(int(y));

            for (x = 0; x < w; ++x) {
                dst_ptr[0] = (unsigned char)((sum * mul_sum) >> shr_sum);
                dst_ptr += 1;

                sum -= sum_out;

                stack_start = sp + div - radius;

                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[1 * stack_start];

                sum_out -= stack_ptr[0];

                if (xp < wm) {
                    src_ptr += 1;
                    ++xp;
                }

                stack_ptr[0] = src_ptr[0];

                sum_in += src_ptr[0];
                sum += sum_in;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 1];

                sum_out += stack_ptr[0];
                sum_in -= stack_ptr[0];
            }
        }

        for (x = 0; x < w; ++x) {
            sum = sum_in = sum_out = 0;

            src_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[i * 1];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (i + 1);
                sum_out += src_ptr[0];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= hm)
                    src_ptr += w1;

                stack_ptr = &stack[1 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                sum += src_ptr[0] * (radius + 1 - i);
                sum_in += src_ptr[0];
            }

            sp = radius;
            yp = radius;
            if (yp > hm)
                yp = hm;

            src_ptr = data.getLinePointer(int(yp)) + (unsigned int)data.pixelStride * x;
            dst_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (y = 0; y < h; ++y) {
                dst_ptr[0] = (unsigned char)((sum * mul_sum) >> shr_sum);
                dst_ptr += w1;

                sum -= sum_out;

                stack_start = sp + div - radius;
                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[1 * stack_start];

                sum_out -= stack_ptr[0];

                if (yp < hm) {
                    src_ptr += w1;
                    ++yp;
                }

                stack_ptr[0] = src_ptr[0];

                sum_in += src_ptr[0];
                sum += sum_in;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 1];

                sum_out += stack_ptr[0];
                sum_in -= stack_ptr[0];
            }
        }
    }

    static void applyStackBlurRGB(Image& img, unsigned int radius)
    {
        auto const w = (unsigned int)img.getWidth();
        auto const h = (unsigned int)img.getHeight();

        Image::BitmapData data(img, Image::BitmapData::readWrite);

        radius = jlimit(2u, 254u, radius);

        unsigned char stack[(254 * 2 + 1) * 3];

        unsigned int x, y, xp, yp, i, sp, stack_start;

        unsigned char* stack_ptr = nullptr;
        unsigned char* src_ptr = nullptr;
        unsigned char* dst_ptr = nullptr;

        unsigned long sum_r, sum_g, sum_b, sum_in_r, sum_in_g, sum_in_b,
            sum_out_r, sum_out_g, sum_out_b;

        unsigned int wm = w - 1;
        unsigned int hm = h - 1;
        auto w3 = (unsigned int)data.lineStride;
        unsigned int div = (unsigned int)(radius * 2) + 1;
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        for (y = 0; y < h; ++y) {
            sum_r = sum_g = sum_b = sum_in_r = sum_in_g = sum_in_b = sum_out_r = sum_out_g = sum_out_b = 0;

            src_ptr = data.getLinePointer(int(y));

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[3 * i];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= wm)
                    src_ptr += 3;

                stack_ptr = &stack[3 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
            }

            sp = radius;
            xp = radius;
            if (xp > wm)
                xp = wm;

            src_ptr = data.getLinePointer(int(y)) + (unsigned int)data.pixelStride * xp;
            dst_ptr = data.getLinePointer(int(y));

            for (x = 0; x < w; ++x) {
                dst_ptr[0] = (unsigned char)((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char)((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char)((sum_b * mul_sum) >> shr_sum);
                dst_ptr += 3;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;

                stack_start = sp + div - radius;

                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[3 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];

                if (xp < wm) {
                    src_ptr += 3;
                    ++xp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 3];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
            }
        }

        for (x = 0; x < w; ++x) {
            sum_r = sum_g = sum_b = sum_in_r = sum_in_g = sum_in_b = sum_out_r = sum_out_g = sum_out_b = 0;

            src_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[i * 3];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= hm)
                    src_ptr += w3;

                stack_ptr = &stack[3 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
            }

            sp = radius;
            yp = radius;
            if (yp > hm)
                yp = hm;

            src_ptr = data.getLinePointer(int(yp)) + (unsigned int)data.pixelStride * x;
            dst_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (y = 0; y < h; ++y) {
                dst_ptr[0] = (unsigned char)((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char)((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char)((sum_b * mul_sum) >> shr_sum);
                dst_ptr += w3;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;

                stack_start = sp + div - radius;
                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[3 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];

                if (yp < hm) {
                    src_ptr += w3;
                    ++yp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 3];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
            }
        }
    }

    static void applyStackBlurARGB(Image& img, unsigned int radius)
    {
        auto const w = (unsigned int)img.getWidth();
        auto const h = (unsigned int)img.getHeight();

        Image::BitmapData data(img, Image::BitmapData::readWrite);

        radius = jlimit(2u, 254u, radius);

        unsigned char stack[(254 * 2 + 1) * 4];

        unsigned int x, y, xp, yp, i, sp, stack_start;

        unsigned char* stack_ptr = nullptr;
        unsigned char* src_ptr = nullptr;
        unsigned char* dst_ptr = nullptr;

        unsigned long sum_r, sum_g, sum_b, sum_a, sum_in_r, sum_in_g, sum_in_b, sum_in_a,
            sum_out_r, sum_out_g, sum_out_b, sum_out_a;

        unsigned int wm = w - 1;
        unsigned int hm = h - 1;
        auto w4 = (unsigned int)data.lineStride;
        unsigned int div = (unsigned int)(radius * 2) + 1;
        unsigned int mul_sum = stackblur_mul[radius];
        unsigned char shr_sum = stackblur_shr[radius];

        for (y = 0; y < h; ++y) {
            sum_r = sum_g = sum_b = sum_a = sum_in_r = sum_in_g = sum_in_b = sum_in_a = sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

            src_ptr = data.getLinePointer(int(y));

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[4 * i];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_a += src_ptr[3] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
                sum_out_a += src_ptr[3];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= wm)
                    src_ptr += 4;

                stack_ptr = &stack[4 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_a += src_ptr[3] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
            }

            sp = radius;
            xp = radius;
            if (xp > wm)
                xp = wm;

            src_ptr = data.getLinePointer(int(y)) + (unsigned int)data.pixelStride * xp;
            dst_ptr = data.getLinePointer(int(y));

            for (x = 0; x < w; ++x) {
                dst_ptr[0] = (unsigned char)((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char)((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char)((sum_b * mul_sum) >> shr_sum);
                dst_ptr[3] = (unsigned char)((sum_a * mul_sum) >> shr_sum);
                dst_ptr += 4;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;
                sum_a -= sum_out_a;

                stack_start = sp + div - radius;

                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[4 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];
                sum_out_a -= stack_ptr[3];

                if (xp < wm) {
                    src_ptr += 4;
                    ++xp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;
                sum_a += sum_in_a;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 4];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_out_a += stack_ptr[3];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
                sum_in_a -= stack_ptr[3];
            }
        }

        for (x = 0; x < w; ++x) {
            sum_r = sum_g = sum_b = sum_a = sum_in_r = sum_in_g = sum_in_b = sum_in_a = sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

            src_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (i = 0; i <= radius; ++i) {
                stack_ptr = &stack[i * 4];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (i + 1);
                sum_g += src_ptr[1] * (i + 1);
                sum_b += src_ptr[2] * (i + 1);
                sum_a += src_ptr[3] * (i + 1);
                sum_out_r += src_ptr[0];
                sum_out_g += src_ptr[1];
                sum_out_b += src_ptr[2];
                sum_out_a += src_ptr[3];
            }

            for (i = 1; i <= radius; ++i) {
                if (i <= hm)
                    src_ptr += w4;

                stack_ptr = &stack[4 * (i + radius)];
                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];
                sum_r += src_ptr[0] * (radius + 1 - i);
                sum_g += src_ptr[1] * (radius + 1 - i);
                sum_b += src_ptr[2] * (radius + 1 - i);
                sum_a += src_ptr[3] * (radius + 1 - i);
                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
            }

            sp = radius;
            yp = radius;
            if (yp > hm)
                yp = hm;

            src_ptr = data.getLinePointer(int(yp)) + (unsigned int)data.pixelStride * x;
            dst_ptr = data.getLinePointer(0) + (unsigned int)data.pixelStride * x;

            for (y = 0; y < h; ++y) {
                dst_ptr[0] = (unsigned char)((sum_r * mul_sum) >> shr_sum);
                dst_ptr[1] = (unsigned char)((sum_g * mul_sum) >> shr_sum);
                dst_ptr[2] = (unsigned char)((sum_b * mul_sum) >> shr_sum);
                dst_ptr[3] = (unsigned char)((sum_a * mul_sum) >> shr_sum);
                dst_ptr += w4;

                sum_r -= sum_out_r;
                sum_g -= sum_out_g;
                sum_b -= sum_out_b;
                sum_a -= sum_out_a;

                stack_start = sp + div - radius;
                if (stack_start >= div)
                    stack_start -= div;

                stack_ptr = &stack[4 * stack_start];

                sum_out_r -= stack_ptr[0];
                sum_out_g -= stack_ptr[1];
                sum_out_b -= stack_ptr[2];
                sum_out_a -= stack_ptr[3];

                if (yp < hm) {
                    src_ptr += w4;
                    ++yp;
                }

                stack_ptr[0] = src_ptr[0];
                stack_ptr[1] = src_ptr[1];
                stack_ptr[2] = src_ptr[2];
                stack_ptr[3] = src_ptr[3];

                sum_in_r += src_ptr[0];
                sum_in_g += src_ptr[1];
                sum_in_b += src_ptr[2];
                sum_in_a += src_ptr[3];
                sum_r += sum_in_r;
                sum_g += sum_in_g;
                sum_b += sum_in_b;
                sum_a += sum_in_a;

                ++sp;
                if (sp >= div)
                    sp = 0;

                stack_ptr = &stack[sp * 4];

                sum_out_r += stack_ptr[0];
                sum_out_g += stack_ptr[1];
                sum_out_b += stack_ptr[2];
                sum_out_a += stack_ptr[3];
                sum_in_r -= stack_ptr[0];
                sum_in_g -= stack_ptr[1];
                sum_in_b -= stack_ptr[2];
                sum_in_a -= stack_ptr[3];
            }
        }
    }

    static void applyStackBlur(Image& img, int radius)
    {
        if (img.getFormat() == Image::ARGB)
            applyStackBlurARGB(img, (unsigned int)radius);
        if (img.getFormat() == Image::RGB)
            applyStackBlurRGB(img, (unsigned int)radius);
        if (img.getFormat() == Image::SingleChannel)
            applyStackBlurBW(img, (unsigned int)radius);
    }

    static void renderDropShadow(Graphics& g, Path const& path, Colour color, int const radius = 1, Point<int> const offset = { 0, 0 }, int spread = 0)
    {
        if (radius < 1)
            return;

        auto area = (path.getBounds().getSmallestIntegerContainer() + offset)
                        .expanded(radius + spread + 1)
                        .getIntersection(g.getClipBounds().expanded(radius + spread + 1));

        if (area.getWidth() < 2 || area.getHeight() < 2)
            return;

        // spread enlarges or shrinks the path before blurring it
        auto spreadPath = Path(path);
        if (spread != 0) {
            area.expand(spread, spread);
            auto bounds = path.getBounds().expanded(spread);
            spreadPath.scaleToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true);
        }

        Image renderedPath(Image::SingleChannel, area.getWidth(), area.getHeight(), true);

        Graphics g2(renderedPath);
        g2.setColour(Colours::white);
        g2.fillPath((spread != 0) ? spreadPath : path, AffineTransform::translation((float)(offset.x - area.getX()), (float)(offset.y - area.getY())));
        applyStackBlur(renderedPath, radius);

        g.setColour(color);
        g.drawImageAt(renderedPath, area.getX(), area.getY(), true);
    }
};

#if !JUCE_BSD
namespace juce {
bool isWindowOnCurrentVirtualDesktop(void* x);
}
#endif

class StackDropShadower : private ComponentListener {
public:
    /** Creates a DropShadower. */
    explicit StackDropShadower(DropShadow const& shadowType, int cornerRadius = 0)
        : shadow(shadowType)
        , shadowCornerRadius(cornerRadius)
    {
    }

    /** Destructor. */
    ~StackDropShadower() override
    {
        if (virtualDesktopWatcher != nullptr)
            virtualDesktopWatcher->removeListener(this);

        if (owner != nullptr) {
            owner->removeComponentListener(this);
            owner = nullptr;
        }

        updateParent();

        ScopedValueSetter<bool> const setter(reentrant, true);
        shadowWindows.clear();
    }

    /** Attaches the DropShadower to the component you want to shadow. */
    void setOwner(Component* componentToFollow)
    {
        if (componentToFollow != owner) {
            if (owner != nullptr)
                owner->removeComponentListener(this);

            owner = componentToFollow;

            if (!owner) {
                for (int i = 4; --i >= 0;) {
                    // there seem to be rare situations where the dropshadower may be deleted by
                    // callbacks during this loop, so use a weak ref to watch out for this..
                    WeakReference<Component> sw(shadowWindows[i]);

                    if (sw != nullptr) {
                        sw->setVisible(false);
                    }
                }
                return;
            }

            updateParent();
            owner->addComponentListener(this);

            // The visibility of `owner` is transitively affected by the visibility of its parents. Thus we need to trigger the
            // componentVisibilityChanged() event in case it changes for any of the parents.
            visibilityChangedListener = std::make_unique<ParentVisibilityChangedListener>(*owner,
                static_cast<ComponentListener&>(*this));

            virtualDesktopWatcher = std::make_unique<VirtualDesktopWatcher>(*owner);
            virtualDesktopWatcher->addListener(this, [this]() { updateShadows(); });

            updateShadows();
        }
    }

    void repaint()
    {
        for (int i = 4; --i >= 0;) {
            // there seem to be rare situations where the dropshadower may be deleted by
            // callbacks during this loop, so use a weak ref to watch out for this..
            WeakReference<Component> sw(shadowWindows[i]);
            if (sw != nullptr) {
                sw->repaint();
            }
        }
    }

private:
    void componentMovedOrResized(Component& c, bool, bool) override
    {
        if (owner == &c)
            updateShadows();
    }

    void componentBroughtToFront(Component& c) override
    {
        if (owner == &c)
            updateShadows();
    }

    void componentChildrenChanged(Component&) override
    {
        updateShadows();
    }

    void componentParentHierarchyChanged(Component& c) override
    {
        if (owner == &c) {
            updateParent();
            updateShadows();
        }
    }

    void componentVisibilityChanged(Component& c) override
    {
        if (owner == &c)
            updateShadows();
    }

    void updateShadows()
    {
        if (reentrant)
            return;

        ScopedValueSetter<bool> const setter(reentrant, true);

        if (owner != nullptr
            && owner->isShowing()
            && owner->getWidth() > 0 && owner->getHeight() > 0
            && (ProjectInfo::canUseSemiTransparentWindows() || owner->getParentComponent() != nullptr)
            && (virtualDesktopWatcher == nullptr || !virtualDesktopWatcher->shouldHideDropShadow())) {
            while (shadowWindows.size() < 4)
                shadowWindows.add(new ShadowWindow(owner, shadow, shadowCornerRadius));

            int const shadowEdge = jmax(shadow.offset.x, shadow.offset.y) + shadow.radius * 2.0f;

            // Reduce by shadow radius. We'll compensate later by drawing the shadow twice as large
            auto b = owner->getBounds().reduced(shadow.radius);
            int const x = b.getX();
            int const y = b.getY() - shadowEdge;
            int const w = b.getWidth();
            int const h = b.getHeight() + shadowEdge + shadowEdge;

            for (int i = 4; --i >= 0;) {
                // there seem to be rare situations where the dropshadower may be deleted by
                // callbacks during this loop, so use a weak ref to watch out for this..
                WeakReference<Component> sw(shadowWindows[i]);

                if (sw != nullptr) {
                    sw->setVisible(true);
                    sw->setAlwaysOnTop(owner->isAlwaysOnTop());

                    if (sw == nullptr)
                        return;

                    switch (i) {
                    case 0:
                        sw->setBounds(x - shadowEdge, y, shadowEdge, h);
                        break;
                    case 1:
                        sw->setBounds(x + w, y, shadowEdge, h);
                        break;
                    case 2:
                        sw->setBounds(x, y, w, shadowEdge);
                        break;
                    case 3:
                        sw->setBounds(x, b.getBottom(), w, shadowEdge);
                        break;
                    default:
                        break;
                    }

                    if (sw == nullptr)
                        return;

                    sw->toBehind(owner.get());
                }
            }
        } else {
            shadowWindows.clear();
        }
    }

    void updateParent()
    {
        if (Component* p = lastParentComp)
            p->removeComponentListener(this);

        lastParentComp = owner != nullptr ? owner->getParentComponent() : nullptr;

        if (Component* p = lastParentComp)
            p->addComponentListener(this);
    }

    class ShadowWindow : public Component {
    public:
        ShadowWindow(Component* comp, DropShadow const& ds, int cornerRadius)
            : target(comp)
            , shadow(ds)
            , shadowCornerRadius(cornerRadius)
        {
            setVisible(true);
            setAccessible(false);
            setInterceptsMouseClicks(false, false);

            if (comp->isOnDesktop()) {
#if JUCE_WINDOWS
                auto const scope = [&]() -> std::unique_ptr<ScopedThreadDPIAwarenessSetter> {
                    if (comp != nullptr)
                        if (auto* handle = comp->getWindowHandle())
                            return std::make_unique<ScopedThreadDPIAwarenessSetter>(handle);

                    return nullptr;
                }();
#endif

                setSize(1, 1); // to keep the OS happy by not having zero-size windows
                addToDesktop(ComponentPeer::windowIgnoresMouseClicks
                    | ComponentPeer::windowIsTemporary
                    | ComponentPeer::windowIgnoresKeyPresses);
            } else if (Component* const parent = comp->getParentComponent()) {
                parent->addChildComponent(this);
            }
        }

        void paint(Graphics& g) override
        {
            if (auto* c = dynamic_cast<TopLevelWindow*>(target.get())) {
                auto shadowPath = Path();
                shadowPath.addRoundedRectangle(getLocalArea(c, c->getLocalBounds().reduced(shadow.radius * 0.9f)).toFloat(), windowCornerRadius);

                auto radius = c->isActiveWindow() ? shadow.radius * 2.0f : shadow.radius * 1.5f;
                StackShadow::renderDropShadow(g, shadowPath, shadow.colour, radius, shadow.offset);
            } else {
                auto shadowPath = Path();
                shadowPath.addRoundedRectangle(getLocalArea(target, target->getLocalBounds()).toFloat(), shadowCornerRadius);
                StackShadow::renderDropShadow(g, shadowPath, shadow.colour, shadow.radius, shadow.offset);
            }
        }

        void resized() override
        {
            repaint(); // (needed for correct repainting)
        }

        float getDesktopScaleFactor() const override
        {
            if (target != nullptr)
                return target->getDesktopScaleFactor();

            return Component::getDesktopScaleFactor();
        }

    private:
        WeakReference<Component> target;
        DropShadow shadow;

        inline static float const windowCornerRadius = 7.5f;

        int shadowCornerRadius;

        JUCE_DECLARE_NON_COPYABLE(ShadowWindow)
    };

    class VirtualDesktopWatcher final : public ComponentListener
        , private Timer {
    public:
        explicit VirtualDesktopWatcher(Component& c)
            : component(&c)
        {
            component->addComponentListener(this);
            update();
        }

        ~VirtualDesktopWatcher() override
        {
            stopTimer();

            if (auto* c = component.get())
                c->removeComponentListener(this);
        }

        bool shouldHideDropShadow() const
        {
            return hasReasonToHide;
        }

        void addListener(void* listener, std::function<void()> cb)
        {
            listeners[listener] = std::move(cb);
        }

        void removeListener(void* listener)
        {
            listeners.erase(listener);
        }

        void componentParentHierarchyChanged(Component& c) override
        {
            if (component.get() == &c)
                update();
        }

    private:
        void update()
        {
            auto const newHasReasonToHide = [this]() {
                if (!component.wasObjectDeleted() && isWindows && component->isOnDesktop()) {
                    startTimerHz(5);
#if JUCE_BSD
                    return false;
#else
                    return !isWindowOnCurrentVirtualDesktop(component->getWindowHandle());
#endif
                }

                stopTimer();
                return false;
            }();

            if (std::exchange(hasReasonToHide, newHasReasonToHide) != newHasReasonToHide)
                for (auto& l : listeners)
                    l.second();
        }

        void timerCallback() override
        {
            update();
        }

        WeakReference<Component> component;
        bool const isWindows = (SystemStats::getOperatingSystemType() & SystemStats::Windows) != 0;
        bool hasReasonToHide = false;
        std::map<void*, std::function<void()>> listeners;
    };

    class ParentVisibilityChangedListener : public ComponentListener {
    public:
        ParentVisibilityChangedListener(Component& r, ComponentListener& l)
            : root(&r)
            , listener(&l)
        {
            updateParentHierarchy();
        }

        ~ParentVisibilityChangedListener() override
        {
            for (auto& compEntry : observedComponents)
                if (auto* comp = compEntry.get())
                    comp->removeComponentListener(this);
        }

        void componentVisibilityChanged(Component& component) override
        {
            if (root != &component)
                listener->componentVisibilityChanged(*root);
        }

        void componentParentHierarchyChanged(Component& component) override
        {
            if (root == &component)
                updateParentHierarchy();
        }

    private:
        class ComponentWithWeakReference {
        public:
            explicit ComponentWithWeakReference(Component& c)
                : ptr(&c)
                , ref(&c)
            {
            }

            Component* get() const { return ref.get(); }

            bool operator<(ComponentWithWeakReference const& other) const { return ptr < other.ptr; }

        private:
            Component* ptr;
            WeakReference<Component> ref;
        };

        void updateParentHierarchy()
        {
            auto const lastSeenComponents = std::exchange(observedComponents, [&] {
                std::set<ComponentWithWeakReference> result;

                for (auto node = root; node != nullptr; node = node->getParentComponent())
                    result.emplace(*node);

                return result;
            }());

            auto const withDifference = [](auto const& rangeA, auto const& rangeB, auto&& callback) {
                std::vector<ComponentWithWeakReference> result;
                std::set_difference(rangeA.begin(), rangeA.end(), rangeB.begin(), rangeB.end(), std::back_inserter(result));

                for (const auto& item : result)
                    if (auto* c = item.get())
                        callback(*c);
            };

            withDifference(lastSeenComponents, observedComponents, [this](auto& comp) { comp.removeComponentListener(this); });
            withDifference(observedComponents, lastSeenComponents, [this](auto& comp) { comp.addComponentListener(this); });
        }

        Component* root = nullptr;
        ComponentListener* listener = nullptr;
        std::set<ComponentWithWeakReference> observedComponents;

        JUCE_DECLARE_NON_COPYABLE(ParentVisibilityChangedListener)
        JUCE_DECLARE_NON_MOVEABLE(ParentVisibilityChangedListener)
    };

    WeakReference<Component> owner;
    OwnedArray<Component> shadowWindows;
    DropShadow shadow;
    bool reentrant = false;
    WeakReference<Component> lastParentComp;

    int shadowCornerRadius;

    std::unique_ptr<ParentVisibilityChangedListener> visibilityChangedListener;
    std::unique_ptr<VirtualDesktopWatcher> virtualDesktopWatcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StackDropShadower);
    JUCE_DECLARE_WEAK_REFERENCEABLE(StackDropShadower);
};
