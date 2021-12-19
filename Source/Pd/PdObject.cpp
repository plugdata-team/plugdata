/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdObject.hpp"
#include "PdInstance.hpp"
extern "C"
{
#include <z_libpd.h>
#include <m_pd.h>
#include <m_imp.h>
#include <s_stuff.h>
#include <g_canvas.h>
#include "x_libpd_extra_utils.h"
}

struct _outconnect
{
    struct _outconnect *oc_next;
    t_pd *oc_to;
};


struct _outlet
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};

namespace pd
{
// ==================================================================================== //
//                                      OBJECT                                          //
// ==================================================================================== //

Object::Object(void* ptr, Patch* patch, Instance* instance) noexcept :
m_ptr(ptr), m_patch(patch), m_instance(instance)
{
    
}

std::string Object::getText()
{
    if(m_ptr && m_patch->checkObject(this))
    {
        char* text = nullptr;
        int size = 0;
        m_instance->setThis();
        libpd_get_object_text(m_ptr, &text, &size);
        if(text && size)
        {
            std::string txt(text, size);
            freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
            return txt;
        }
    }
    return std::string();
}

std::string Object::getName() const
{
    if(m_ptr)
    {
        char const* name = libpd_get_object_class_name(m_ptr);
        if(name)
        {
            return std::string(name);
        }
    }
    return std::string();
}

std::array<int, 4> Object::getBounds() const noexcept
{
    if(m_ptr)
    {
        int x = 0, y = 0, w = 0, h = 0;
        m_instance->setThis();
        
        libpd_get_object_bounds(m_patch->getPointer(), m_ptr, &x, &y, &w, &h);
        
        t_canvas const* cnv = m_patch->getPointer();
        if(cnv != nullptr)
        {
            //x -= cnv->gl_xmargin;
            //y -= cnv->gl_ymargin;
        }
        
        return {int(x * Patch::zoom), int(y * Patch::zoom), w, h};
    }
    return {0, 0, 0, 0};
}

//! @brief The name of the help file
std::string Object::getHelp() const
{
    static File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
    static File helpDir = appDir.getChildFile("Documentation/5.reference/");
    
    auto* pdclass = pd_class((t_pd*)m_ptr);
    auto* name = class_gethelpname(pdclass);
    auto* dir = class_gethelpdir(pdclass);

    char realname[MAXPDSTRING], dirbuf[MAXPDSTRING];
        /* make up a silly "dir" if none is supplied */
    const char *usedir = (*dir ? dir :  helpDir.getFullPathName().toRawUTF8());

        /* 1. "objectname-help.pd" */
    strncpy(realname, name, MAXPDSTRING-10);
    realname[MAXPDSTRING-10] = 0;
    if (strlen(realname) > 3 && !strcmp(realname+strlen(realname)-3, ".pd"))
        realname[strlen(realname)-3] = 0;
    strcat(realname, "-help.pd");
    
    if(File(usedir).getChildFile(realname).existsAsFile()) {
        return File(usedir).getChildFile(realname).getFullPathName().toStdString();
    }

        /* 2. "help-objectname.pd" */
    strcpy(realname, "help-");
    strncat(realname, name, MAXPDSTRING-10);
    realname[MAXPDSTRING-1] = 0;
    
    if(File(dir).getChildFile(realname).existsAsFile()) {
        
        return File(dir).getChildFile(realname).getFullPathName().toStdString();
    }
    
    return std::string();
}



bool Object::operator==(Object const& other) const noexcept
{
    return m_ptr == other.m_ptr;
}

bool Object::operator!=(Object const& other) const noexcept
{
    return m_ptr != other.m_ptr;
}

int Object::getNumInlets() noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_ninlets(checked);
    }
    
    return 0;
}
int Object::getNumOutlets() noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_noutlets(checked);
    }
    
    return 0;
}

bool Object::isSignalInlet(int idx) noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_issignalinlet(checked, idx);
    }
    
    return 0;
}
bool Object::isSignalOutlet(int idx) noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_issignaloutlet(checked, idx);
    }
    
    return 0;
}

}


