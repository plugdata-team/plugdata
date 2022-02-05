/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdObject.h"

#include "PdInstance.h"
extern "C"
{
#include <g_canvas.h>
#include <m_imp.h>
#include <m_pd.h>
#include <s_stuff.h>

#include "x_libpd_extra_utils.h"
}

struct _outconnect
{
  struct _outconnect* oc_next;
  t_pd* oc_to;
};

struct _outlet
{
  t_object* o_owner;
  struct _outlet* o_next;
  t_outconnect* o_connections;
  t_symbol* o_sym;
};

namespace pd
{
// ==================================================================================== //
//                                      OBJECT                                          //
// ==================================================================================== //

Object::Object(void* objectPtr, Patch* parentPatch, Instance* parentInstance) noexcept : ptr(objectPtr), patch(parentPatch), instance(parentInstance) {}

bool Object::operator==(Object const& other) const noexcept { return ptr == other.ptr; }

bool Object::operator!=(Object const& other) const noexcept { return ptr != other.ptr; }

std::string Object::getText()
{
  if (ptr && patch->checkObject(this))
  {
    char* text = nullptr;
    int size = 0;
    instance->setThis();
    libpd_get_object_text(ptr, &text, &size);
    if (text && size)
    {
      std::string txt(text, size);
      freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
      return txt;
    }
  }
  return {};
}

std::string Object::getName() const
{
  if (ptr)
  {
    char const* name = libpd_get_object_class_name(ptr);
    if (name)
    {
      return {name};
    }
  }
  return {};
}

std::array<int, 4> Object::getBounds() const noexcept
{
  if (ptr)
  {
    int x = 0, y = 0, w = 0, h = 0;
    instance->setThis();
    patch->setCurrent();

    libpd_get_object_bounds(patch->getPointer(), ptr, &x, &y, &w, &h);

    t_canvas const* cnv = patch->getPointer();
    if (cnv != nullptr)
    {
      // x -= cnv->gl_xmargin;
      // y -= cnv->gl_ymargin;
    }

    return {int(x * Patch::zoom), int(y * Patch::zoom), w, h};
  }
  return {0, 0, 0, 0};
}

//! @brief The name of the help file
Patch Object::getHelp() const
{
  static File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
  static File helpDir = appDir.getChildFile("Documentation/5.reference/");

  auto* pdclass = pd_class(static_cast<t_pd*>(ptr));
  auto* name = class_gethelpname(pdclass);
  auto* dir = class_gethelpdir(pdclass);

  char realname[MAXPDSTRING];
  /* make up a silly "dir" if none is supplied */

  const String& fullPath = helpDir.getFullPathName();
  const char* usedir = (*dir ? dir : fullPath.toRawUTF8());

  /* 1. "objectname-help.pd" */
  strncpy(realname, name, MAXPDSTRING - 10);
  realname[MAXPDSTRING - 10] = 0;
  if (strlen(realname) > 3 && !strcmp(realname + strlen(realname) - 3, ".pd")) realname[strlen(realname) - 3] = 0;
  strcat(realname, "-help.pd");

  if (File(usedir).getChildFile(realname).existsAsFile())
  {
    sys_lock();
    auto p = Patch(glob_evalfile(nullptr, gensym(realname), gensym(usedir)), instance);
    sys_unlock();
    return p;
  }

  /* 2. "help-objectname.pd" */
  strcpy(realname, "help-");
  strncat(realname, name, MAXPDSTRING - 10);
  realname[MAXPDSTRING - 1] = 0;

  if (File(dir).getChildFile(realname).existsAsFile())
  {
    sys_lock();
    auto p = Patch(glob_evalfile(nullptr, gensym(realname), gensym(usedir)), instance);
    sys_unlock();
    return p;
  }

  return {};
}

void Object::setWidth(int width) { static_cast<t_text*>(ptr)->te_width = std::max<short>(3, round(static_cast<float>(width) / sys_fontwidth(18))); }

int Object::getWidth() const { return static_cast<t_text*>(ptr)->te_width * sys_fontwidth(18); }

int Object::getNumInlets() noexcept
{
  if (auto* checked = patch->checkObject(this))
  {
    return libpd_ninlets(checked);
  }

  return 0;
}
int Object::getNumOutlets() noexcept
{
  if (auto* checked = patch->checkObject(this))
  {
    return libpd_noutlets(checked);
  }

  return 0;
}

bool Object::isSignalInlet(int idx) noexcept
{
  if (auto* checked = patch->checkObject(this))
  {
    return libpd_issignalinlet(checked, idx);
  }

  return false;
}
bool Object::isSignalOutlet(int idx) noexcept
{
  if (auto* checked = patch->checkObject(this))
  {
    return libpd_issignaloutlet(checked, idx);
  }

  return false;
}

}  // namespace pd
