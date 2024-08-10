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
#include "Interface.h"
#include "Objects/ObjectBase.h"
#include "../PluginEditor.h"

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>

#include <utility>

#include "g_undo.h"

extern void canvas_reload(t_symbol* name, t_symbol* dir, t_glist* except);
}

namespace pd {

Patch::Patch(pd::WeakReference patchPtr, Instance* parentInstance, bool ownsPatch, File patchFile)
    : instance(parentInstance)
    , closePatchOnDelete(ownsPatch)
    , lastViewportScale(SettingsFile::getInstance()->getProperty<float>("default_zoom") / 100.0f)
    , currentFile(std::move(patchFile))
    , ptr(patchPtr)
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
    return isPatchDirty.load();
}

bool Patch::canUndo() const
{
    return canPatchUndo.load();
}

bool Patch::canRedo() const
{
    return canPatchRedo.load();
}

void Patch::savePatch(URL const& locationURL)
{
    auto location = locationURL.getLocalFile();
    String fullPathname = location.getParentDirectory().getFullPathName();
    String filename = location.hasFileExtension("pd") ? location.getFileName() : location.getFileName() + ".pd";

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    if (auto patch = ptr.get<t_glist>()) {
        setTitle(filename);
        untitledPatchNum = 0;
        canvas_dirty(patch.get(), 0);

#if JUCE_IOS
        auto patchText = getCanvasContent();
        auto outputStream = locationURL.createOutputStream();

        // on iOS, saving with pd's normal method doesn't work
        // we need to use an outputstream on a URL
        outputStream->write(patchText.toRawUTF8(), patchText.getNumBytesAsUTF8());
        outputStream->flush();

        instance->logMessage("saved to: " + location.getFullPathName());
        canvas_rename(patch.get(), file, dir);
#else
        pd::Interface::saveToFile(patch.get(), file, dir);
#endif

        currentFile = location;
        currentURL = locationURL;
        instance->reloadAbstractions(location, patch.get());
    }
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

void Patch::updateUndoRedoState()
{
    if (auto patch = ptr.get<t_glist>()) {
        canPatchUndo = pd::Interface::canUndo(patch.get());
        canPatchRedo = pd::Interface::canRedo(patch.get());
        isPatchDirty = patch->gl_dirty;

        auto undoSize = pd::Interface::getUndoSize(patch.get());
        if (undoQueueSize != undoSize) {
            undoQueueSize = undoSize;
            updateUndoRedoString();
        }
    }
}

void Patch::savePatch()
{
    String fullPathname = currentFile.getParentDirectory().getFullPathName();
    String filename = currentFile.hasFileExtension("pd") ? currentFile.getFileName() : currentFile.getFileName() + ".pd";

    auto* dir = instance->generateSymbol(fullPathname.replace("\\", "/"));
    auto* file = instance->generateSymbol(filename);

    if (auto patch = ptr.get<t_glist>()) {
        setTitle(filename);
        untitledPatchNum = 0;
        canvas_dirty(patch.get(), 0);

        pd::Interface::saveToFile(patch.get(), file, dir);
    }

    MessageManager::callAsync([instance = juce::WeakReference(this->instance), file = this->currentFile, ptr = this->ptr]() {
        if (instance) {
            if(auto patch = ptr.get<t_glist>()) {
                instance->reloadAbstractions(file, patch.get());
            }
        }
    });
}

void Patch::setCurrent()
{
    if (auto patch = ptr.get<t_glist>()) {
        // Ugly fix: plugdata needs gl_havewindow to always be true!
        patch->gl_havewindow = true;
        canvas_create_editor(patch.get());
    }
}

void Patch::setVisible(bool shouldVis)
{
    if (auto patch = ptr.get<t_glist>()) {
        patch->gl_mapped = shouldVis;
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

        while ((oc = linetraverser_next_nosize(&t))) {
            connections.emplace_back(oc, t.tr_inno, t.tr_ob2, t.tr_outno, t.tr_ob);
        }
    }

    return connections;
}

std::vector<pd::WeakReference> Patch::getObjects()
{
    setCurrent();

    std::vector<pd::WeakReference> objects;
    if (auto patch = ptr.get<t_glist>()) {
        for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
            objects.push_back(pd::WeakReference(y, instance));
        }
    }

    return objects;
}

t_gobj* Patch::createObject(int x, int y, String const& name)
{

    StringArray tokens;
    tokens.addTokens(name.replace("\\ ", "__%SPACE%__"), true); // Prevent "/ " from being tokenised

    ObjectThemeManager::get()->formatObject(tokens);

    if (tokens[0] == "garray") {
        if (auto patch = ptr.get<t_glist>()) {
            auto arrayPasta = "#N canvas 0 0 450 250 (subpatch) 0;\n#X array @arrName 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore " + String(x) + " " + String(y) + " graph;";

            instance->setThis();
            auto* newArraySymbol = pd::Interface::getUnusedArrayName();
            arrayPasta = arrayPasta.replace("@arrName", String::fromUTF8(newArraySymbol->s_name));

            pd::Interface::paste(patch.get(), arrayPasta.toRawUTF8());
            return pd::Interface::getNewest(patch.get());
        }
    } else if (tokens[0] == "graph") {
        if (auto patch = ptr.get<t_glist>()) {
            auto graphPasta = "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore " + String(x) + " " + String(y) + " graph;";
            pd::Interface::paste(patch.get(), graphPasta.toRawUTF8());
            return pd::Interface::getNewest(patch.get());
        }
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

    tokens.removeEmptyStrings();

    int argc = tokens.size() + 2;

    auto argv = std::vector<t_atom>(argc);

    // Set position
    SETFLOAT(argv.data(), static_cast<float>(x));
    SETFLOAT(argv.data() + 1, static_cast<float>(y));

    for (int i = 0; i < tokens.size(); i++) {
        // check if string is a valid number
        auto token = tokens[i].replace("__%SPACE%__", "\\ ");
        auto charptr = token.getCharPointer();
        auto ptr = charptr;
        CharacterFunctions::readDoubleValue(ptr); // This will read the number and increment the pointer to be past the number
        if (ptr - charptr == token.getNumBytesAsUTF8()) {
            SETFLOAT(argv.data() + i + 2, token.getFloatValue());
        } else {
            SETSYMBOL(argv.data() + i + 2, instance->generateSymbol(token));
        }
    }

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::getInstanceEditor()->canvas_undo_already_set_move = 1;
        return pd::Interface::createObject(patch.get(), typesymbol, argc, argv.data());
    }

    return nullptr;
}

t_gobj* Patch::renameObject(t_object* obj, String const& name)
{
    StringArray tokens;
    tokens.addTokens(name, false);

    ObjectThemeManager::get()->formatObject(tokens);
    String newName = tokens.joinIntoString(" ");

    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();

        pd::Interface::renameObject(patch.get(), &obj->te_g, newName.toRawUTF8(), newName.getNumBytesAsUTF8());
        return pd::Interface::getNewest(patch.get());
    }

    return nullptr;
}

void Patch::copy(std::vector<t_gobj*> const& objects)
{
    if (auto patch = ptr.get<t_glist>()) {
        int size;
        char const* text = pd::Interface::copy(patch.get(), &size, objects);
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
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens[1] != "f" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    // blank message objects have a comma after their position: "#X msg 0 0, f 9;"
    // this normally isn't an issue, but because they are blank, the comma is next to the number token and doesn't get parsed correctly
    auto isMsg = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] == "msg";
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

        if (canvasDepth == 0) {
            if (isObject(tokens)) {
                minX = std::min(minX, tokens[2].getIntValue());
                minY = std::min(minY, tokens[3].getIntValue());
            } else if (isMsg(tokens)) {
                minX = std::min(minX, tokens[2].getIntValue());
                minY = std::min(minY, tokens[3].removeCharacters(",").getIntValue());
            }
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

        if (canvasDepth == 0) {
            if (isObject(tokens)) {
                tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));
                line = tokens.joinIntoString(" ");
            } else if (isMsg(tokens)) {
                tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                tokens.set(3, String(tokens[3].removeCharacters(",").getIntValue() - minY + position.y) + ",");
                line = tokens.joinIntoString(" ");
            }
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

    auto translatedObjects = translatePatchAsString(text, position);

    if (auto patch = ptr.get<t_glist>()) {
        pd::Interface::paste(patch.get(), translatedObjects.toRawUTF8());
    }
}

void Patch::duplicate(std::vector<t_gobj*> const& objects, t_outconnect* connection)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::duplicateSelection(patch.get(), objects, connection);
    }
}

void Patch::deselectAll()
{
    if (auto patch = ptr.get<t_glist>()) {
        glist_noselect(patch.get());
    }
}

bool Patch::hasConnection(t_object* src, int nout, t_object* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        return pd::Interface::hasConnection(patch.get(), src, nout, sink, nin);
    }

    return false;
}

bool Patch::canConnect(t_object* src, int nout, t_object* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {

        return pd::Interface::canConnect(patch.get(), src, nout, sink, nin);
    }

    return false;
}

void Patch::createConnection(t_object* src, int nout, t_object* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::createConnection(patch.get(), src, nout, sink, nin);
    }
}

t_outconnect* Patch::createAndReturnConnection(t_object* src, int nout, t_object* sink, int nin)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return pd::Interface::createConnection(patch.get(), src, nout, sink, nin);
    }

    return nullptr;
}

void Patch::removeConnection(t_object* src, int nout, t_object* sink, int nin, t_symbol* connectionPath)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::removeConnection(patch.get(), src, nout, sink, nin, connectionPath);
    }
}

t_outconnect* Patch::setConnctionPath(t_object* src, int nout, t_object* sink, int nin, t_symbol* oldConnectionPath, t_symbol* newConnectionPath)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        return pd::Interface::setConnectionPath(patch.get(), src, nout, sink, nin, oldConnectionPath, newConnectionPath);
    }

    return nullptr;
}

void Patch::moveObjects(std::vector<t_gobj*> const& objects, int dx, int dy)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::moveObjects(patch.get(), dx, dy, objects);
    }
}

void Patch::moveObjectTo(t_gobj* object, int x, int y)
{
    if (auto patch = ptr.get<t_glist>()) {
        // Originally this was +1544, but caused issues with alignment tools being off-by xy +2px.
        // FIXME: why do we have to do this at all?
        pd::Interface::moveObject(patch.get(), object, x + 1542, y + 1542);
    }
}

void Patch::finishRemove()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::finishRemove(patch.get());
    }
}

void Patch::removeObjects(std::vector<t_gobj*> const& objects)
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        pd::Interface::removeObjects(patch.get(), objects);
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

        updateUndoRedoString();
    }
}

void Patch::undo()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        auto x = patch.get();
        glist_noselect(x);

        pd::Interface::undo(patch.get());
        pd::Interface::getInstanceEditor()->canvas_undo_already_set_move = 1;

        updateUndoRedoString();
    }
}

void Patch::redo()
{
    if (auto patch = ptr.get<t_glist>()) {
        setCurrent();
        auto x = patch.get();
        glist_noselect(x);

        pd::Interface::redo(patch.get());
        pd::Interface::getInstanceEditor()->canvas_undo_already_set_move = 1;

        updateUndoRedoString();
    }
}

void Patch::updateUndoRedoString()
{
    if (auto patch = ptr.get<t_glist>()) {
        auto cnv = patch.get();
        auto currentUndo = canvas_undo_get(cnv)->u_last;
        auto undo = currentUndo;
        auto redo = currentUndo->next;

#ifdef DEBUG_UNDO_QUEUE
        auto undoDbg = undo;
        auto redoDbg = redo;
#endif

        lastUndoSequence = "";
        lastRedoSequence = "";

        // undo / redo list will contain pd undo events
        while (undo) {
            String undoName = String::fromUTF8(undo->name);
            if (undoName == "props") {
                lastUndoSequence = "Change property";
                break;
            } else if (undoName != "no") {
                lastUndoSequence = undoName.substring(0, 1).toUpperCase() + undoName.substring(1);
                break;
            }
            undo = undo->prev;
        }

        while (redo) {
            String redoName = String::fromUTF8(redo->name);
            if (redoName == "props") {
                lastRedoSequence = "Change property";
                break;
            } else if (redoName != "no") {
                lastRedoSequence = redoName.substring(0, 1).toUpperCase() + redoName.substring(1);
                break;
            }
            redo = redo->next;
        }
// #define DEBUG_UNDO_QUEUE
#ifdef DEBUG_UNDO_QUEUE
        std::cout << "<<<<<< undo list:" << std::endl;
        while (undoDbg) {
            std::cout << undoDbg->name << std::endl;
            undoDbg = undoDbg->prev;
        }
        std::cout << ">>>>>> redo list:" << std::endl;
        while (redoDbg) {
            std::cout << redoDbg->name << std::endl;
            redoDbg = redoDbg->next;
        }
        std::cout << "-------------------" << std::endl;
#endif
    }
}

String Patch::getTitle() const
{
    if (auto patch = ptr.get<t_glist>()) {
        String name = String::fromUTF8(patch->gl_name->s_name);

        int argc = 0;
        t_atom* argv = nullptr;

        canvas_setcurrent(patch.get());
        canvas_getargs(&argc, &argv);
        canvas_unsetcurrent(patch.get());

        if (argc) {
            char namebuf[MAXPDSTRING];
            name += " (";
            for (int i = 0; i < argc; i++) {
                atom_string(&argv[i], namebuf, MAXPDSTRING);
                name += String::fromUTF8(namebuf);
                if (i != argc - 1)
                    name += " ";
            }
            name += ")";
        }

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

void Patch::setUntitled()
{
    // find the lowest `Untitled-N` number, for the new patch title
    int lowestNumber = 0;
    for (auto patch : instance->patches) {
        lowestNumber = std::max(lowestNumber, patch->untitledPatchNum);
    }
    lowestNumber += 1;

    untitledPatchNum = lowestNumber;
    setTitle("Untitled-" + String(lowestNumber));
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

void Patch::setCurrentFile(URL const& newURL)
{
    currentFile = newURL.getLocalFile();
    currentURL = newURL;
}

String Patch::getCanvasContent()
{
    char* buf;
    int bufsize;

    if (auto patch = ptr.get<t_canvas>()) {
        pd::Interface::getCanvasContent(patch.get(), &buf, &bufsize);
    } else {
        return {};
    }

    auto content = String::fromUTF8(buf, static_cast<size_t>(bufsize));

    freebytes(static_cast<void*>(buf), static_cast<size_t>(bufsize) * sizeof(char));

    return content;
}

void Patch::reloadPatch(File const& changedPatch, t_glist* except)
{
    auto* dir = gensym(changedPatch.getParentDirectory().getFullPathName().replace("\\", "/").toRawUTF8());
    auto* file = gensym(changedPatch.getFileName().toRawUTF8());
    canvas_reload(file, dir, except);
}

bool Patch::objectWasDeleted(t_gobj* objectPtr) const
{
    if (auto patch = ptr.get<t_glist>()) {
        for (t_gobj* y = patch->gl_list; y; y = y->g_next) {
            if (y == objectPtr)
                return false;
        }
    }

    return true;
}

} // namespace pd
