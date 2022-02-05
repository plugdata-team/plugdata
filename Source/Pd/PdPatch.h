/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

#include "PdGui.h"
#include "x_libpd_mod_utils.h"

namespace pd
{

using Connections = std::vector<std::tuple<int, t_object*, int, t_object*>>;
class Instance;
// ==================================================================================== //
//                                          PATCHER                                     //
// ==================================================================================== //

//! @brief The Pd patch.
//! @details The class is a wrapper around a Pd patch. The lifetime of the internal patch\n
//! is not guaranteed by the class.
//! @see Instance, Object, Gui
class Patch
{
 public:
  Patch(void* ptr, Instance* instance) noexcept;

  Patch(const File& toOpen, Instance* instance) noexcept;

  //! @brief The default constructor.
  Patch() = default;

  //! @brief The copy constructor.
  Patch(const Patch&) = default;

  //! @brief The compare equal operator.
  bool operator==(Patch const& other) const noexcept { return getPointer() == other.getPointer(); }

  //! @brief The copy operator.
  Patch& operator=(const Patch& other) = default;

  //! @brief The destructor.
  ~Patch() noexcept = default;

  //! @brief Gets the bounds of the patch.
  [[nodiscard]] std::array<int, 4> getBounds() const noexcept;

  std::unique_ptr<Object> createGraph(const String& name, int size, int x, int y);
  std::unique_ptr<Object> createGraphOnParent(int x, int y);

  std::unique_ptr<Object> createObject(const String& name, int x, int y, bool undoable = true);
  void removeObject(Object* obj);
  std::unique_ptr<Object> renameObject(Object* obj, const String& name);

  void moveObjects(const std::vector<Object*>&, int x, int y);

  void finishRemove();
  void removeSelection();

  void selectObject(Object*);
  void deselectAll();

  void setZoom(int zoom);

  void copy();
  void paste();
  void duplicate();

  void undo();
  void redo();

  enum GroupUndoType
  {
    Remove = 0,
    Move
  };

  void setCurrent();

  bool canConnect(Object* src, int nout, Object* sink, int nin);
  bool createConnection(Object* src, int nout, Object* sink, int nin);
  void removeConnection(Object* src, int nout, Object* sink, int nin);

  [[nodiscard]] Connections getConnections() const;

  [[nodiscard]] t_canvas* getPointer() const { return static_cast<t_canvas*>(ptr); }

  //! @brief Gets the objects of the patch.
  std::vector<Object> getObjects(bool onlyGui = false) noexcept;

  String getCanvasContent()
  {
    if (!ptr) return {};
    char* buf;
    int bufsize;
    libpd_getcontent(static_cast<t_canvas*>(ptr), &buf, &bufsize);
    return {buf, static_cast<size_t>(bufsize)};
  }

  int getIndex(void* obj);

  t_gobj* getInfoObject();
  void setExtraInfoId(const String& oldId, const String& newId);

  void storeExtraInfo(bool undoable = true);

  void updateExtraInfo();
  [[nodiscard]] MemoryBlock getExtraInfo(const String& id) const;
  void setExtraInfo(const String& id, MemoryBlock& info);

  ValueTree extraInfo = ValueTree("PlugDataInfo");

  static t_object* checkObject(Object* obj) noexcept;

  void keyPress(int keycode, int shift);

  [[nodiscard]] String getTitle() const;
  void setTitle(const String& title);

  static inline float zoom = 1.5f;

 private:
  void* ptr = nullptr;
  Instance* instance = nullptr;

  friend class Instance;
  friend class Gui;
  friend class Object;
};
}  // namespace pd
