/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


class VUScale : public ObjectLabel, public AsyncUpdater {
    StringArray scale = { "+12", "+6", "+2", "-0dB", "-2", "-6", "-12", "-20", "-30", "-50", "-99" };
    StringArray scaleDecim = { "+12", "", "", "-0dB", "", "", "-12", "", "", "", "-99" };
    NVGFramebuffer scalebuffer;
public:
    VUScale()
    {
    }

    ~VUScale()
    {
    }
        
    void performRender(NVGcontext* nvg)
    {
        nvgFontSize(nvg, 8);
        nvgFontFace(nvg, "Inter-Regular");
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(nvg, convertColour(findColour(Label::textColourId)));
        auto scaleToUse = getHeight() < 80 ? scaleDecim : scale;
        for (int i = 0; i < scale.size(); i++) {
            auto posY = ((getHeight() - 20) * (i / 10.0f)) + 10;
            // align the "-" and "+" text element centre
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(nvg, 2, posY, scaleToUse[i].substring(0, 1).toRawUTF8(), nullptr);
            // align the number text element left
            nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(nvg, 5, posY, scaleToUse[i].substring(1).toRawUTF8(), nullptr);
        }
    }
    
    float getImageScale(Canvas* cnv)
    {
        Canvas* topLevel = cnv;
        while (auto* nextCnv = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCnv;
        }
        return topLevel->isZooming ? topLevel->getRenderScale() * 2.0f : topLevel->getRenderScale() * std::max(1.0f, getValue<float>(topLevel->zoomScale));
    }
    
    void handleAsyncUpdate() override
    {
        if(auto* cnv = findParentComponentOfClass<Canvas>()) {
            auto renderScale = getImageScale(cnv);
            auto w = roundToInt(getWidth() * renderScale);
            auto h = roundToInt(getHeight() * renderScale);
            if(auto* nvg = cnv->editor->nvgSurface.getRawContext()) {
                scalebuffer.renderToFramebuffer(nvg, w, h, [this, w, h, renderScale](NVGcontext* nvg) {
                    nvgBeginFrame(nvg, w, h, 1.0f);
                    nvgScale(nvg, renderScale, renderScale);
                    performRender(nvg);
                    nvgEndFrame(nvg);
                });
            }
        }
    }

    virtual void renderLabel(NVGcontext* nvg, float scale) override
    {
        auto w = roundToInt(getWidth() * scale);
        auto h = roundToInt(getHeight() * scale);
        if (scalebuffer.needsUpdate(w, h)) {
            triggerAsyncUpdate();
            performRender(nvg);
        }
        else {
            scalebuffer.render(nvg, Rectangle<int>(getWidth(), getHeight()));
        }
    }
};


class VUMeterObject final : public ObjectBase {

    IEMHelper iemHelper;
    Value sizeProperty = SynchronousValue();
    Value showScale = SynchronousValue();

public:
    VUMeterObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        // we need to make this a specific size as it has two inlets
        // which will become squashed together if too close
        onConstrainerCreate = [this]() {
            constrainer->setMinimumSize(20, 20 * 2);
        };

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamReceiveSymbol(&iemHelper.receiveSymbol);
        objectParameters.addParamBool("Show scale", ParameterCategory::cAppearance, &showScale, { "No", "Yes" }, 1);
        iemHelper.addIemParameters(objectParameters, false, false, -1);

        updateLabel();
        if(auto vu = ptr.get<t_vu>()) showScale = vu->x_scale;
        valueChanged(showScale);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(iem->x_w), var(iem->x_h) });
        }
    }

    bool inletIsSymbol() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        String const text = iemHelper.labelText.toString();

        ObjectLabel* label = nullptr;
        VUScale* vuScale = nullptr;
        if (labels.isEmpty()) {
            label = labels.add(new ObjectLabel());
            vuScale = reinterpret_cast<VUScale*>(labels.add(new VUScale()));
            object->cnv->addChildComponent(label);
            object->cnv->addChildComponent(vuScale);
        }
        else {
            label = labels[0];
            vuScale = reinterpret_cast<VUScale*>(labels[1]);
        }

        if (label && text.isNotEmpty()) {
            auto bounds = iemHelper.getLabelBounds();
            bounds.translate(0, bounds.getHeight() / -2.0f);

            label->setFont(Font(bounds.getHeight()));
            label->setBounds(bounds);
            label->setText(text, dontSendNotification);
            label->setColour(Label::textColourId, iemHelper.getLabelColour());
            label->setVisible(true);
        }
        if (vuScale) {
            auto vuScaleBounds = Rectangle<int>(object->getBounds().getTopRight().x - 3, object->getBounds().getTopRight().y, 20, object->getBounds().getHeight());
            vuScale->setBounds(vuScaleBounds);
            vuScale->setVisible(true);
            vuScale->setColour(Label::textColourId, iemHelper.getLabelColour());
        }
    }
    
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto vu = ptr.get<t_vu>()) {
                vu->x_gui.x_w = width;
                vu->x_gui.x_h = height;
            }

            object->updateBounds();
        } else if (v.refersToSameSourceAs(showScale)) {
            if (auto vu = ptr.get<t_vu>()) {
                auto showVU = getValue<bool>(showScale);
                vu->x_scale = showVU;
                //if(auto* vuScale = getExtraLabel()) vuScale->setVisible(showVU);
            }
        } else {
            iemHelper.valueChanged(v);
        }
    }

    void update() override
    {
        if (auto vu = ptr.get<t_vu>()) {
            sizeProperty = Array<var> { var(vu->x_gui.x_w), var(vu->x_gui.x_h) };
        }

        iemHelper.update();
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b);
    }

    void render(NVGcontext* nvg) override
    {
        if(!ptr.isValid()) return;
        
        auto values = std::vector<float> { ptr.get<t_vu>()->x_fp, ptr.get<t_vu>()->x_fr };
        auto backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        int height = getHeight();
        int width = getWidth();

        nvgDrawRoundedRect(nvg, 0, 0, width, height, backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        auto outerBorderWidth = 2.0f;
        auto totalBlocks = 30;
        auto spacingFraction = 0.05f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockHeight = (height - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockWidth = width - doubleOuterBorderWidth;
        auto blockRectHeight = (1.0f - 2.0f * spacingFraction) * blockHeight;
        auto blockRectSpacing = spacingFraction * blockHeight;
        auto blockCornerSize = 0.1f * blockHeight;

        float rms = Decibels::decibelsToGain(values[1] - 12.0f);
        float lvl = (float)std::exp(std::log(rms) / 3.0) * (rms > 0.002);
        auto numBlocks = roundToInt(totalBlocks * lvl);

        auto verticalGradient1 = nvgLinearGradient(nvg, 0, getHeight() * 0.25f, 0, getHeight() * 0.5f, nvgRGBA(255, 127, 0, 1), nvgRGBA(66, 163, 198, 255));
        auto verticalGradient2 = nvgLinearGradient(nvg, 0, 0, 0, getHeight() * 0.25f, nvgRGBA(255, 0, 0, 255), nvgRGBA(255, 127, 0, 255));

        for (auto i = 1; i < totalBlocks; ++i) {
            NVGpaint gradient;
            if (i >= numBlocks) {
                nvgFillColor(nvg, nvgRGBA(0.3f, 0.3f, 0.3f, 1.0f)); // Dark grey for inactive blocks
            } else {
                gradient = (i < totalBlocks * 0.75f) ? verticalGradient1 : verticalGradient2;
                nvgFillPaint(nvg, gradient);
            }
            nvgFillRoundedRect(nvg, outerBorderWidth, outerBorderWidth + ((totalBlocks - i) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight, blockCornerSize);
        }

        float peak = Decibels::decibelsToGain(values[0] - 12.0f);
        float lvl2 = (float)std::exp(std::log(peak) / 3.0) * (peak > 0.002);
        auto numBlocks2 = roundToInt(totalBlocks * lvl2);

        nvgFillColor(nvg, nvgRGBA(255, 255, 255, 255)); // White for the peak block
        nvgFillRoundedRect(nvg, outerBorderWidth, outerBorderWidth + ((totalBlocks - numBlocks2) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight / 2.0f, blockCornerSize);
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"): {
            repaint();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            break;
        }
        }
    }
};
