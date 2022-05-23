/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "DrawableTemplate.h"


struct ScalarObject : public ObjectBase
{
    OwnedArray<DrawableTemplate> templates;
        
    ScalarObject(void* obj, Box* box) : ObjectBase(obj, box)
    {
        box->setVisible(false);
        
        auto* x = reinterpret_cast<t_scalar*>(obj);
        auto* templ = template_findbyname(x->sc_template);
        auto* templatecanvas = template_findcanvas(templ);
        t_gobj* y;
        t_float basex, basey;
        scalar_getbasexy(x, &basex, &basey);

        for (y = templatecanvas->gl_list; y; y = y->g_next)
        {
            const t_parentwidgetbehavior* wb = pd_getparentwidget(&y->g_pd);
            if (!wb) continue;

            auto* drawable = templates.add(new DrawableTemplate(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            cnv->addAndMakeVisible(drawable);
            //drawable->setBounds(cnv->getLocalBounds());
        }
    }
    
    ~ScalarObject() {
    }
    
    void updateValue() override {
        for(auto& drawable : templates)
        {
            drawable->updateIfMoved();
        }
    };
    
    void parentSizeChanged() override
    {
        for(auto& drawable : templates)
        {
            drawable->updateIfMoved();
        }
    }
    
    void resized() override {
        for(auto& drawable : templates)
        {
            drawable->updateIfMoved();
        }
    };
    
    void updateDrawables() override {
        for(auto& drawable : templates)
        {
            drawable->update();
        }
    }
    
    void updateBounds() override {
        
    }
    

    void applyBounds() override {
        
    }
    
};
