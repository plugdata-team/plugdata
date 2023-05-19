/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Patch.h"
#include "Instance.h"
#include "Objects/ObjectBase.h"

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>

#include "g_undo.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_multi.h"

struct _instanceeditor {
    t_binbuf* copy_binbuf;
    char* canvas_textcopybuf;
    int canvas_textcopybufsize;
    t_undofn canvas_undo_fn;      /* current undo function if any */
    int canvas_undo_whatnext;     /* whether we can now UNDO or REDO */
    void* canvas_undo_buf;        /* data private to the undo function */
    t_canvas* canvas_undo_canvas; /* which canvas we can undo on */
    char const* canvas_undo_name;
    int canvas_undo_already_set_move;
    double canvas_upclicktime;
    int canvas_upx, canvas_upy;
    int canvas_find_index, canvas_find_wholeword;
    t_binbuf* canvas_findbuf;
    int paste_onset;
    t_canvas* paste_canvas;
    t_glist* canvas_last_glist;
    int canvas_last_glist_x, canvas_last_glist_y;
    t_canvas* canvas_cursorcanvaswas;
    unsigned int canvas_cursorwas;
};

extern void canvas_reload(t_symbol* name, t_symbol* dir, t_glist* except);
}

namespace pd {

Patch::Patch(void* patchPtr, Instance* parentInstance, bool ownsPatch, File patchFile)
    : ptr(patchPtr)
    , instance(parentInstance)
    , currentFile(patchFile)
    , closePatchOnDelete(ownsPatch)
{
    jassert(parentInstance);
}

Patch::~Patch()
{
    // Only close the patch if this is a top-level patch
    // Otherwise, this is a subpatcher and it will get cleaned up by Pd
    // when the object is deleted
    if (closePatchOnDelete && ptr && instance) {
        instance->setThis();
        instance->lockAudioThread();
        instance->clearObjectImplementationsForPatch(this); // Make sure that there are no object implementations running in the background!
        libpd_closefile(ptr);
        instance->unlockAudioThread();
    }
}

Rectangle<int> Patch::getBounds() const
{
    if (ptr) {
        t_canvas* cnv = getPointer();

        if (cnv->gl_isgraph) {
            cnv->gl_pixwidth = std::max(15, cnv->gl_pixwidth);
            cnv->gl_pixheight = std::max(15, cnv->gl_pixheight);

            return { cnv->gl_xmargin, cnv->gl_ymargin, cnv->gl_pixwidth, cnv->gl_pixheight };
        } else {
            auto width = cnv->gl_screenx2 - cnv->gl_screenx1;
            auto height = cnv->gl_screeny2 - cnv->gl_screeny1;

            return { cnv->gl_screenx1, cnv->gl_screeny1, width, height };
        }
    }
    return { 0, 0, 0, 0 };
}

bool Patch::isDirty() const
{
    if (!ptr)
        return false;

    // Don't lock for now:
    // TODO: this is not thread safe, but otherwise this sometimes causes deadlocks
    // const ScopedLock audioLock(*instance->audioLock);

    return getPointer()->gl_dirty;
}

void Patch::savePatch(File const& location)
{
    if (!ptr)
        return;

    String fullPathname = location.getParentDirectory().getFullPathName();
    String filename = location.getFileName();

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    setTitle(filename);
    canvas_dirty(getPointer(), 0);

    instance->lockAudioThread();

    libpd_savetofile(getPointer(), file, dir);

    instance->reloadAbstractions(location, getPointer());

    instance->unlockAudioThread();

    currentFile = location;
}

t_glist* Patch::getRoot()
{
    if (!ptr)
        return nullptr;

    return canvas_getrootfor(getPointer());
}

bool Patch::isSubpatch()
{
    if (!ptr)
        return false;

    return getRoot() != ptr && !canvas_isabstraction(getPointer());
}

bool Patch::isAbstraction()
{
    if (!ptr)
        return false;

    return canvas_isabstraction(getPointer());
}

void Patch::savePatch()
{
    if (!ptr)
        return;

    String fullPathname = currentFile.getParentDirectory().getFullPathName();
    String filename = currentFile.getFileName();

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    setTitle(filename);
    canvas_dirty(getPointer(), 0);

    instance->lockAudioThread();

    libpd_savetofile(getPointer(), file, dir);
    instance->reloadAbstractions(currentFile, getPointer());

    instance->unlockAudioThread();
}

void Patch::setCurrent()
{
    if (!ptr)
        return;

    instance->setThis();

    instance->lockAudioThread();

    // This is the same as calling canvas_vis and canvas_map,
    // but all the other stuff inside those functions is just for tcl/tk anyway
    getPointer()->gl_havewindow = 1;
    getPointer()->gl_mapped = 1;

    canvas_create_editor(getPointer()); // can't hurt to make sure of this!

    instance->unlockAudioThread();
}

Connections Patch::getConnections() const
{
    if (!ptr)
        return {};

    Connections connections;

    t_outconnect* oc;
    t_linetraverser t;

    instance->lockAudioThread();
    // Get connections from pd
    linetraverser_start(&t, getPointer());

    // TODO: fix data race
    while ((oc = linetraverser_next(&t))) {
        connections.push_back({ oc, t.tr_inno, t.tr_ob2, t.tr_outno, t.tr_ob });
    }

    instance->unlockAudioThread();

    return connections;
}

std::vector<void*> Patch::getObjects()
{
    if (!ptr)
        return {};

    setCurrent();

    instance->lockAudioThread();

    std::vector<void*> objects;
    t_canvas const* cnv = getPointer();

    for (t_gobj* y = cnv->gl_list; y; y = y->g_next) {
        objects.push_back(static_cast<void*>(y));
    }

    instance->unlockAudioThread();

    return objects;
}

void* Patch::createGraphOnParent(int x, int y)
{
    if (!ptr)
        return nullptr;

    instance->lockAudioThread();
    setCurrent();
    t_pd* pdobject = libpd_creategraphonparent(getPointer(), x, y);
    instance->unlockAudioThread();

    assert(pdobject);

    return pdobject;
}

void* Patch::createGraph(int x, int y, String const& name, int size,int drawMode, bool saveContents, std::pair<float, float> range)
{
    if (!ptr)
        return nullptr;

    instance->lockAudioThread();
                              
    setCurrent();
    t_pd* pdobject = libpd_creategraph(getPointer(), name.toRawUTF8(), size, x, y, drawMode, saveContents, range.first, range.second);
                              
    instance->unlockAudioThread();

    assert(pdobject);

    return pdobject;
}

void* Patch::createObject(int x, int y, String const& name)
{
    if (!ptr)
        return nullptr;

    instance->setThis();

    StringArray tokens;
    tokens.addTokens(name, false);

    // See if we have preset parameters for this object
    // These parameters are designed to make the experience in plugdata better
    // Mostly larger GUI objects and a different colour scheme
    if (guiDefaults.find(tokens[0]) != guiDefaults.end()) {
        auto preset = guiDefaults.at(tokens[0]);

        auto bg = instance->getBackgroundColour();
        auto fg = instance->getForegroundColour();
        auto lbl = instance->getTextColour();
        auto ln = instance->getOutlineColour();

        auto bg_str = bg.toString().substring(2);
        auto fg_str = fg.toString().substring(2);
        auto lbl_str = lbl.toString().substring(2);
        auto ln_str = ln.toString().substring(2);

        preset = preset.replace("bgColour_rgb", String(bg.getRed()) + " " + String(bg.getGreen()) + " " + String(bg.getBlue()));
        preset = preset.replace("fgColour_rgb", String(fg.getRed()) + " " + String(fg.getGreen()) + " " + String(fg.getBlue()));
        preset = preset.replace("lblColour_rgb", String(lbl.getRed()) + " " + String(lbl.getGreen()) + " " + String(lbl.getBlue()));
        preset = preset.replace("lnColour_rgb", String(ln.getRed()) + " " + String(ln.getGreen()) + " " + String(ln.getBlue()));

        preset = preset.replace("bgColour", "#" + bg_str);
        preset = preset.replace("fgColour", "#" + fg_str);
        preset = preset.replace("lblColour", "#" + lbl_str);
        preset = preset.replace("lnColour", "#" + ln_str);

        tokens.addTokens(preset, false);
    }

    if (tokens[0] == "garray" && tokens.size() == 7) {
        return createGraph(x, y, tokens[1], tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue(), {tokens[5].getFloatValue(), tokens[6].getFloatValue()});
    } else if (tokens[0] == "graph") {
        return createGraphOnParent(x, y);
    }

    t_symbol* typesymbol = instance->generateSymbol("obj");

    if (tokens[0] == "msg") {
        typesymbol = instance->generateSymbol("msg");
        tokens.remove(0);
    }
    if (tokens[0] == "comment") {
        typesymbol = instance->generateSymbol("text");
        tokens.remove(0);
    }
    if (tokens[0] == "floatatom") {
        typesymbol = instance->generateSymbol("floatatom");
        tokens.remove(0);
    }
    if (tokens[0] == "listbox") {
        typesymbol = instance->generateSymbol("listbox");
        tokens.remove(0);
    }
    if (tokens[0] == "symbolatom") {
        typesymbol = instance->generateSymbol("symbolatom");
        tokens.remove(0);
    }
    if (tokens[0] == "+") {
        tokens.set(0, "\\+");
    }

    int argc = tokens.size() + 2;

    auto argv = std::vector<t_atom>(argc);

    // Set position
    SETFLOAT(argv.data(), static_cast<float>(x));
    SETFLOAT(argv.data() + 1, static_cast<float>(y));

    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].containsOnly("0123456789e.-+") && tokens[i] != "-") {
            SETFLOAT(argv.data() + i + 2, tokens[i].getFloatValue());
        } else {
            SETSYMBOL(argv.data() + i + 2, instance->generateSymbol(tokens[i]));
        }
    }

    instance->lockAudioThread();
    
    setCurrent();
    t_pd* pdobject = libpd_createobj(getPointer(), typesymbol, argc, argv.data());
    
    instance->unlockAudioThread();

    assert(pdobject);
    return pdobject;
}

void* Patch::renameObject(void* obj, String const& name)
{
    if (!obj || !ptr)
        return nullptr;

    StringArray tokens;
    tokens.addTokens(name, false);

    // See if we have preset parameters for this object
    // These parameters are designed to make the experience in plugdata better
    // Mostly larger GUI objects and a different colour scheme
    if (guiDefaults.find(tokens[0]) != guiDefaults.end()) {
        auto preset = guiDefaults.at(tokens[0]);

        auto bg = instance->getBackgroundColour();
        auto fg = instance->getForegroundColour();
        auto lbl = instance->getTextColour();
        auto ln = instance->getOutlineColour();

        auto bg_str = bg.toString().substring(2);
        auto fg_str = fg.toString().substring(2);
        auto lbl_str = lbl.toString().substring(2);
        auto ln_str = ln.toString().substring(2);

        preset = preset.replace("bgColour_rgb", String(bg.getRed()) + " " + String(bg.getGreen()) + " " + String(bg.getBlue()));
        preset = preset.replace("fgColour_rgb", String(fg.getRed()) + " " + String(fg.getGreen()) + " " + String(fg.getBlue()));
        preset = preset.replace("lblColour_rgb", String(lbl.getRed()) + " " + String(lbl.getGreen()) + " " + String(lbl.getBlue()));
        preset = preset.replace("lnColour_rgb", String(ln.getRed()) + " " + String(ln.getGreen()) + " " + String(ln.getBlue()));

        preset = preset.replace("bgColour", "#" + bg_str);
        preset = preset.replace("fgColour", "#" + fg_str);
        preset = preset.replace("lblColour", "#" + lbl_str);
        preset = preset.replace("lnColour", "#" + ln_str);

        tokens.addTokens(preset, false);
    }
    String newName = tokens.joinIntoString(" ");

    instance->lockAudioThread();
    
    setCurrent();
    libpd_renameobj(getPointer(), &checkObject(obj)->te_g, newName.toRawUTF8(), newName.getNumBytesAsUTF8());
    t_pd* pdobject = libpd_newest(getPointer());
    
    instance->unlockAudioThread();

    return pdobject;
}

void Patch::copy()
{
    if (!ptr)
        return;
    
    instance->lockAudioThread();
    
    int size;
    const char* text = libpd_copy(getPointer(), &size);
    auto copied = String::fromUTF8(text, size);
    MessageManager::callAsync([copied]() mutable { SystemClipboard::copyTextToClipboard(copied); });
    
    instance->unlockAudioThread();
}

String Patch::translatePatchAsString(String patchAsString, Point<int> position)
{
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();

    int canvasDepth = 0;

    auto isObject = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isStartingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
    };

    auto isEndingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    for (auto& line : StringArray::fromLines(patchAsString)) {

        line = line.upToLastOccurrenceOf(";", false, false);

        auto tokens = StringArray::fromTokens(line, true);

        if (isStartingCanvas(tokens)) {
            canvasDepth++;
        }

        if (canvasDepth == 0 && isObject(tokens)) {
            minX = std::min(minX, tokens[2].getIntValue());
            minY = std::min(minY, tokens[3].getIntValue());
        }

        if (isEndingCanvas(tokens)) {
            if (canvasDepth == 1) {
                minX = std::min(minX, tokens[2].getIntValue());
                minY = std::min(minY, tokens[3].getIntValue());
            }
            canvasDepth--;
        }
    }

    canvasDepth = 0;
    auto toPaste = StringArray::fromLines(patchAsString);
    for (auto& line : toPaste) {
        line = line.upToLastOccurrenceOf(";", false, false);
        auto tokens = StringArray::fromTokens(line, true);
        if (isStartingCanvas(tokens)) {
            canvasDepth++;
        }

        if (canvasDepth == 0 && isObject(tokens)) {
            tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
            tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));

            line = tokens.joinIntoString(" ");
        }

        if (isEndingCanvas(tokens)) {
            if (canvasDepth == 1) {
                tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));
            }

            line = tokens.joinIntoString(" ");

            canvasDepth--;
        }

        line += ";";
    }
    return toPaste.joinIntoString("\n");
}

void Patch::paste(Point<int> position)
{
    if (!ptr)
        return;

    auto text = SystemClipboard::getTextFromClipboard();

    auto translatedObjects = translatePatchAsString(text, position);

    instance->lockAudioThread();
    libpd_paste(getPointer(), translatedObjects.toRawUTF8());
    instance->unlockAudioThread();
}

void Patch::duplicate()
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    setCurrent();
    libpd_duplicate(getPointer());
    instance->unlockAudioThread();
}

void Patch::selectObject(void* obj)
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    
    auto* checked = &checkObject(obj)->te_g;
    if (!glist_isselected(getPointer(), checked)) {
        glist_select(getPointer(), checked);
    }
    
    instance->unlockAudioThread();
}

void Patch::deselectAll()
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    
    glist_noselect(getPointer());
    libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    
    instance->unlockAudioThread();
}

void Patch::removeObject(void* obj)
{
    if (!obj || !ptr)
        return;

    instance->lockAudioThread();
    
    setCurrent();
    libpd_removeobj(getPointer(), &checkObject(obj)->te_g);
    
    instance->unlockAudioThread();
}

bool Patch::hasConnection(void* src, int nout, void* sink, int nin)
{
    if (!ptr)
        return false;


    instance->lockAudioThread();
    auto hasConnection = libpd_hasconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    instance->unlockAudioThread();

    return hasConnection;
}

bool Patch::canConnect(void* src, int nout, void* sink, int nin)
{
    if (!ptr)
        return false;

    instance->lockAudioThread();
    bool canConnect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);;
    instance->unlockAudioThread();
    
    return canConnect;
}

void Patch::createConnection(void* src, int nout, void* sink, int nin)
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    bool canConnect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    if(canConnect)
    {
        setCurrent();
        libpd_createconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    }
    instance->unlockAudioThread();
}

void* Patch::createAndReturnConnection(void* src, int nout, void* sink, int nin)
{
    if (!src || !sink || !ptr)
        return nullptr;

     void* outconnect = nullptr;

    instance->lockAudioThread();
    
    bool canConnect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);

    if (canConnect) {
        setCurrent();
        outconnect = libpd_createconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    }

    instance->unlockAudioThread();

    return outconnect;
}

void Patch::removeConnection(void* src, int nout, void* sink, int nin, t_symbol* connectionPath)
{
    if (!src || !sink || !ptr)
        return;

    instance->lockAudioThread();
    
    setCurrent();
    libpd_removeconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin, connectionPath);
    
    instance->unlockAudioThread();
}

void* Patch::setConnctionPath(void* src, int nout, void* sink, int nin, t_symbol* oldConnectionPath, t_symbol* newConnectionPath)
{
    if (!ptr)
        return nullptr;

    instance->lockAudioThread();
    
    setCurrent();
    void* outconnect = libpd_setconnectionpath(getPointer(), checkObject(src), nout, checkObject(sink), nin, oldConnectionPath, newConnectionPath);

    instance->unlockAudioThread();

    return outconnect;
}

void Patch::moveObjects(std::vector<void*> const& objects, int dx, int dy)
{
    if (!ptr)
        return;
    
    instance->lockAudioThread();

    setCurrent();

    glist_noselect(getPointer());

    for (auto* obj : objects) {
        glist_select(getPointer(), &checkObject(obj)->te_g);
    }

    libpd_moveselection(getPointer(), dx, dy);

    glist_noselect(getPointer());

    libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    setCurrent();
    
    instance->unlockAudioThread();
}

void Patch::finishRemove()
{
    if (!ptr)
        return;
    
    instance->lockAudioThread();
    
    setCurrent();
    libpd_finishremove(getPointer());
    
    instance->unlockAudioThread();
}

void Patch::removeSelection()
{
    if (!ptr)
        return;

    
    instance->lockAudioThread();
    
    setCurrent();
    libpd_removeselection(getPointer());
    
    instance->unlockAudioThread();
}

void Patch::startUndoSequence(String name)
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    
    canvas_undo_add(getPointer(), UNDO_SEQUENCE_START, instance->generateSymbol(name)->s_name, 0);

    instance->unlockAudioThread();
}

void Patch::endUndoSequence(String name)
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    canvas_undo_add(getPointer(), UNDO_SEQUENCE_END, instance->generateSymbol(name)->s_name, 0);
    instance->unlockAudioThread();
}

void Patch::undo()
{
    if (!ptr)
        return;
    
    instance->lockAudioThread();
    
    setCurrent();
    glist_noselect(getPointer());
    libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;

    libpd_undo(getPointer());
    
    instance->unlockAudioThread();
}

void Patch::redo()
{
    if (!ptr)
        return;

    instance->lockAudioThread();
    
    setCurrent();
    glist_noselect(getPointer());
    libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;

    libpd_redo(getPointer());
    
    instance->unlockAudioThread();
}

t_object* Patch::checkObject(void* obj)
{
    return pd_checkobject(static_cast<t_pd*>(obj));
}

String Patch::getTitle() const
{
    if (!ptr)
        return "";

    instance->lockAudioThread();
    
    String name = String::fromUTF8(getPointer()->gl_name->s_name);
    
    instance->unlockAudioThread();
    
    return name.isEmpty() ? "Untitled Patcher" : name;
}

void Patch::setTitle(String const& title)
{
    if (!ptr)
        return;

    setCurrent();

    auto* pathSym = instance->generateSymbol(getCurrentFile().getFullPathName());

    t_atom args[2];
    SETSYMBOL(args, instance->generateSymbol(title));
    SETSYMBOL(args + 1, pathSym);

    instance->lockAudioThread();
    
    pd_typedmess(static_cast<t_pd*>(ptr), instance->generateSymbol("rename"), 2, args);

    instance->unlockAudioThread();
    
    MessageManager::callAsync([instance = this->instance]() {
        instance->titleChanged();
    });
}

File Patch::getCurrentFile() const
{
    return currentFile;
}

// This gets the raw patch path from Pd instead of our own stored path
// We should probably move over to using this everywhere eventually
File Patch::getPatchFile() const
{
    auto* dir = canvas_getdir(getPointer())->s_name;
    auto* name = getPointer()->gl_name->s_name;

    return File(String::fromUTF8(dir)).getChildFile(String::fromUTF8(name)).getFullPathName();
}

void Patch::setCurrentFile(File newFile)
{
    currentFile = newFile;
}

String Patch::getCanvasContent()
{
    if (!ptr)
        return {};
    
    instance->lockAudioThread();

    char* buf;
    int bufsize;
    libpd_getcontent(static_cast<t_canvas*>(ptr), &buf, &bufsize);

    auto content = String::fromUTF8(buf, static_cast<size_t>(bufsize));

    freebytes(static_cast<void*>(buf), static_cast<size_t>(bufsize) * sizeof(char));

    instance->unlockAudioThread();
    
    return content;
}

void Patch::reloadPatch(File changedPatch, t_glist* except)
{
    auto* dir = gensym(changedPatch.getParentDirectory().getFullPathName().replace("\\", "/").toRawUTF8());
    auto* file = gensym(changedPatch.getFileName().toRawUTF8());
    canvas_reload(file, dir, except);
}

bool Patch::objectWasDeleted(void* ptr)
{
    if (!ptr)
        return true;

    t_canvas const* cnv = getPointer();

    for (t_gobj* y = cnv->gl_list; y; y = y->g_next) {
        if (y == ptr)
            return false;
    }

    return true;
}
bool Patch::connectionWasDeleted(void* ptr)
{
    if (!ptr)
        return true;

    t_outconnect* oc;
    t_linetraverser t;

    instance->lockAudioThread();

    // Get connections from pd
    linetraverser_start(&t, getPointer());

    while ((oc = linetraverser_next(&t))) {
        if (oc == ptr) {
            instance->unlockAudioThread();
            return false;
        }
    }

    instance->unlockAudioThread();

    return true;
}

} // namespace pd
