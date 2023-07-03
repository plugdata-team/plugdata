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

#include <utility>

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
    : ptr(patchPtr, parentInstance)
    , instance(parentInstance)
    , currentFile(std::move(patchFile))
    , closePatchOnDelete(ownsPatch)
{
    jassert(parentInstance);
}

Patch::~Patch()
{
    // Only close the patch if this is a top-level patch
    // Otherwise, this is a subpatcher and it will get cleaned up by Pd
    // when the object is deleted
    if (closePatchOnDelete && instance) {
        instance->setThis();
        instance->clearObjectImplementationsForPatch(this); // Make sure that there are no object implementations running in the background!

        if (auto patch = ptr.get<void>()) {
            libpd_closefile(patch.get());
        }
    }
}

Rectangle<int> Patch::getBounds() const
{
    if (auto cnv = ptr.get<t_canvas>()) {

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
    if (auto patch = ptr.get<t_glist>()) {
        return patch->gl_dirty;
    }

    return false;
}

void Patch::savePatch(File const& location)
{

    String fullPathname = location.getParentDirectory().getFullPathName();
    String filename = location.getFileName();

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    if (auto patch = ptr.get<t_glist>()) {
        setTitle(filename);
        canvas_dirty(patch.get(), 0);

        libpd_savetofile(patch.get(), file, dir);

        instance->reloadAbstractions(location, patch.get());
    }

    currentFile = location;
}

t_glist* Patch::getRoot()
{
    if (auto patch = ptr.get<t_canvas>()) {
        return canvas_getrootfor(patch.get());
    }

    return nullptr;
}

bool Patch::isSubpatch()
{
    if (auto patch = ptr.get<t_canvas>()) {
        return getRoot() != patch.get() && !canvas_isabstraction(patch.get());
    }

    return false;
}

bool Patch::isAbstraction()
{
    if (auto patch = ptr.get<t_canvas>()) {
        return canvas_isabstraction(patch.get());
    }

    return false;
}

void Patch::savePatch()
{
    String fullPathname = currentFile.getParentDirectory().getFullPathName();
    String filename = currentFile.getFileName();

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    if (auto patch = ptr.get<t_glist>()) {
        setTitle(filename);
        canvas_dirty(patch.get(), 0);

        libpd_savetofile(patch.get(), file, dir);
        instance->reloadAbstractions(currentFile, patch.get());
    }

    instance->lockAudioThread();

    instance->unlockAudioThread();
}

void Patch::setCurrent()
{
    if (auto patch = ptr.get<t_glist>()) {
        // This is the same as calling canvas_vis and canvas_map,
        // but all the other stuff inside those functions is just for tcl/tk anyway

        patch->gl_havewindow = 1;
        patch->gl_mapped = 1;

        canvas_create_editor(patch.get()); // can't hurt to make sure of this!
    }
}

Connections Patch::getConnections() const
{
    Connections connections;

    t_outconnect* oc;
    t_linetraverser t;

    if (auto patch = ptr.get<t_glist>()) {
        // Get connections from pd
        linetraverser_start(&t, patch.get());

        // TODO: fix data race
        while ((oc = linetraverser_next(&t))) {
            connections.emplace_back(oc, t.tr_inno, t.tr_ob2, t.tr_outno, t.tr_ob);
        }
    }

    return connections;
}

std::vector<void*> Patch::getObjects()
{
    setCurrent();

    std::vector<void*> objects;
    if (auto patch = ptr.get<t_glist>()) {
        for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
            objects.push_back(static_cast<void*>(y));
        }
    }

    return objects;
}

void* Patch::createGraphOnParent(int x, int y)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return libpd_creategraphonparent(patch.get(), x, y);
    }

    return nullptr;
}

void* Patch::createGraph(int x, int y, String const& name, int size, int drawMode, bool saveContents, std::pair<float, float> range)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return libpd_creategraph(patch.get(), name.toRawUTF8(), size, x, y, drawMode, saveContents, range.first, range.second);
    }

    return nullptr;
}

void* Patch::createObject(int x, int y, String const& name)
{

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
        return createGraph(x, y, tokens[1], tokens[2].getIntValue(), tokens[3].getIntValue(), tokens[4].getIntValue(), { tokens[5].getFloatValue(), tokens[6].getFloatValue() });
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
        // check if string is a valid number
        auto charptr = tokens[i].getCharPointer();
        auto ptr = charptr;
        auto value = CharacterFunctions::readDoubleValue(ptr);
        if (ptr - charptr == tokens[i].getNumBytesAsUTF8()) {
            SETFLOAT(argv.data() + i + 2, tokens[i].getFloatValue());
        } else {
            SETSYMBOL(argv.data() + i + 2, instance->generateSymbol(tokens[i]));
        }
    }

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return libpd_createobj(patch.get(), typesymbol, argc, argv.data());
    }

    return nullptr;
}

void* Patch::renameObject(void* obj, String const& name)
{
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

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_renameobj(patch.get(), &checkObject(obj)->te_g, newName.toRawUTF8(), newName.getNumBytesAsUTF8());
        return libpd_newest(patch.get());
    }

    return nullptr;
}

void Patch::copy()
{
    if (auto patch = ptr.get<t_glist>()) {
        int size;
        char const* text = libpd_copy(patch.get(), &size);
        auto copied = String::fromUTF8(text, size);
        MessageManager::callAsync([copied]() mutable { SystemClipboard::copyTextToClipboard(copied); });
    }
}

String Patch::translatePatchAsString(String const& patchAsString, Point<int> position)
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
    auto text = SystemClipboard::getTextFromClipboard();

    // for some reason when we paste into PD, we need to apply a translation? 
    auto translatedObjects = translatePatchAsString(text, position.translated(1540, 1540));

    if (auto patch = ptr.get<t_glist>()) {
        libpd_paste(patch.get(), translatedObjects.toRawUTF8());
    }
}

void Patch::duplicate()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_duplicate(patch.get());
    }
}

void Patch::selectObject(void* obj)
{

    if (auto patch = ptr.get<t_glist>()) {
        auto* checked = &checkObject(obj)->te_g;
        if (!glist_isselected(patch.get(), checked)) {
            glist_select(patch.get(), checked);
        }
    }
}

void Patch::deselectAll()
{
    if (auto patch = ptr.get<t_glist>()) {
        glist_noselect(patch.get());
        libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    }
}

void Patch::removeObject(void* obj)
{
    instance->lockAudioThread();

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_removeobj(patch.get(), &checkObject(obj)->te_g);
    }
}

bool Patch::hasConnection(void* src, int nout, void* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        return libpd_hasconnection(patch.get(), checkObject(src), nout, checkObject(sink), nin);
    }

    return false;
}

bool Patch::canConnect(void* src, int nout, void* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        return libpd_canconnect(patch.get(), checkObject(src), nout, checkObject(sink), nin);
    }

    return false;
}

void Patch::createConnection(void* src, int nout, void* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_createconnection(patch.get(), checkObject(src), nout, checkObject(sink), nin);
    }
}

void* Patch::createAndReturnConnection(void* src, int nout, void* sink, int nin)
{
    void* outconnect = nullptr;

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return libpd_createconnection(patch.get(), checkObject(src), nout, checkObject(sink), nin);
    }

    return nullptr;
}

void Patch::removeConnection(void* src, int nout, void* sink, int nin, t_symbol* connectionPath)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_removeconnection(patch.get(), checkObject(src), nout, checkObject(sink), nin, connectionPath);
    }
}

void* Patch::setConnctionPath(void* src, int nout, void* sink, int nin, t_symbol* oldConnectionPath, t_symbol* newConnectionPath)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return libpd_setconnectionpath(patch.get(), checkObject(src), nout, checkObject(sink), nin, oldConnectionPath, newConnectionPath);
    }

    return nullptr;
}

void Patch::moveObjects(std::vector<void*> const& objects, int dx, int dy)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();

        glist_noselect(patch.get());

        for (auto* obj : objects) {
            glist_select(patch.get(), &checkObject(obj)->te_g);
        }

        libpd_moveselection(patch.get(), dx, dy);

        glist_noselect(patch.get());

        libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
        setCurrent();
    }
}

void Patch::finishRemove()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_finishremove(patch.get());
    }
}

void Patch::removeSelection()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        libpd_removeselection(patch.get());
    }
}

void Patch::startUndoSequence(String const& name)
{
    if (auto patch = ptr.get<t_glist>()) {
        canvas_undo_add(patch.get(), UNDO_SEQUENCE_START, instance->generateSymbol(name)->s_name, nullptr);
    }
}

void Patch::endUndoSequence(String const& name)
{
    if (auto patch = ptr.get<t_glist>()) {
        canvas_undo_add(patch.get(), UNDO_SEQUENCE_END, instance->generateSymbol(name)->s_name, nullptr);
    }
}

void Patch::undo()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        glist_noselect(patch.get());
        libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;

        libpd_undo(patch.get());
    }
}

void Patch::redo()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        glist_noselect(patch.get());
        libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
        libpd_redo(patch.get());
    }
}

t_object* Patch::checkObject(void* obj)
{
    return pd_checkobject(static_cast<t_pd*>(obj));
}

String Patch::getTitle() const
{
    if (auto patch = ptr.get<t_glist>()) {
        String name = String::fromUTF8(patch->gl_name->s_name);
        return name.isEmpty() ? "Untitled Patcher" : name;
    }

    return "Untitled Patcher";
}

void Patch::setTitle(String const& title)
{
    auto* pathSym = instance->generateSymbol(getCurrentFile().getFullPathName());

    t_atom args[2];
    SETSYMBOL(args, instance->generateSymbol(title));
    SETSYMBOL(args + 1, pathSym);

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd_typedmess(patch.cast<t_pd>(), instance->generateSymbol("rename"), 2, args);
    }

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
    if (auto patch = ptr.get<t_glist>()) {
        auto* dir = canvas_getdir(patch.get())->s_name;
        auto* name = patch->gl_name->s_name;
        return File(String::fromUTF8(dir)).getChildFile(String::fromUTF8(name)).getFullPathName();
    }

    return File();
}

void Patch::setCurrentFile(File newFile)
{
    currentFile = std::move(newFile);
}

String Patch::getCanvasContent()
{
    char* buf;
    int bufsize;

    if (auto patch = ptr.get<t_canvas>()) {
        libpd_getcontent(patch.get(), &buf, &bufsize);
    } else {
        return {};
    }

    auto content = String::fromUTF8(buf, static_cast<size_t>(bufsize));

    freebytes(static_cast<void*>(buf), static_cast<size_t>(bufsize) * sizeof(char));

    instance->unlockAudioThread();

    return content;
}

void Patch::reloadPatch(File const& changedPatch, t_glist* except)
{
    auto* dir = gensym(changedPatch.getParentDirectory().getFullPathName().replace("\\", "/").toRawUTF8());
    auto* file = gensym(changedPatch.getFileName().toRawUTF8());
    canvas_reload(file, dir, except);
}

bool Patch::objectWasDeleted(void* objectPtr) const
{
    if (auto patch = ptr.get<t_glist>()) {
        for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
            if (y == objectPtr)
                return false;
        }
    }

    return true;
}
bool Patch::connectionWasDeleted(void* connectionPtr) const
{
    t_outconnect* oc;
    t_linetraverser t;

    if (auto patch = ptr.get<t_glist>()) {
        // Get connections from pd
        linetraverser_start(&t, patch.get());

        while ((oc = linetraverser_next(&t))) {
            if (oc == connectionPtr) {
                return false;
            }
        }

        return true;
    }

    return true;
}

} // namespace pd
