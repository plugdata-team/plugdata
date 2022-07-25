#include "../Dialogs/Dialogs.h"

struct t_fake_textbuf
{
    t_object b_ob;
    t_binbuf *b_binbuf;
    t_canvas *b_canvas;
    t_guiconnect *b_guiconnect;
    t_symbol *b_sym;
};


struct t_fake_text_define
{
    t_fake_textbuf x_textbuf;
    t_outlet *x_out;
    t_outlet *x_notifyout;
    t_symbol *x_bindsym;
    t_scalar *x_scalar;     /* faux scalar (struct text-scalar) to point to */
    t_gpointer x_gp;        /* pointer to it */
    t_canvas *x_canvas;     /* owning canvas whose stub we use for x_gp */
    unsigned char x_keep;   /* whether to embed contents in patch on save */
};



// Actual text object, marked final for optimisation
struct TextDefineObject final : public TextBase
{
    
    std::unique_ptr<Component> textEditor;
    
    TextDefineObject(void* obj, Box* parent, bool isValid = true) : TextBase(obj, parent, isValid), textEditor(nullptr)
    {
        
    }
    
    void lock(bool isLocked) override {
        setInterceptsMouseClicks(isLocked, false);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        openTextEditor();
    }
    
    void openTextEditor() {
        if(textEditor) {
            textEditor->toFront(true);
            return;
        }
        
        auto name = String(static_cast<t_fake_text_define*>(ptr)->x_bindsym->s_name);
        
        textEditor.reset(
                         Dialogs::showTextEditorDialog(getText(), name, [this](StringArray lastText, bool hasChanged){
                             
                             if(!hasChanged) {
                                 textEditor.reset(nullptr);
                                 return;
                             }
                             
                             Dialogs::showSaveDialog(textEditor.get(), "", [this, lastText](int result) mutable {
                                 if(result == 2) {
                                     setText(lastText);
                                     textEditor.reset(nullptr);
                                 }
                                 if(result == 1) {
                                     textEditor.reset(nullptr);
                                 }
                             });
                         }));
    }
    
    void setText(StringArray text) {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;
        auto* binbuf = textbuf.b_binbuf;
        binbuf_clear(binbuf);
        
        std::vector<t_atom> atoms;
        atoms.reserve(text.size());
        for(auto& line : text) {
            t_atom lineAtom;
            SETSYMBOL(&lineAtom, gensym(line.toRawUTF8()));
            atoms.push_back(lineAtom);
        }
        
        binbuf_restore(binbuf, static_cast<int>(atoms.size()), atoms.data());
    }
    
    String getText() {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;
        auto* binbuf = textbuf.b_binbuf;
        
        char** bufp = new char*;
        int* lenp = new int;
        
        String result;
        
        auto argc = binbuf_getnatom(binbuf);
        auto* argv = binbuf_getvec(binbuf);
        for(int i = 0; i < argc; i++) {
            result += String::fromUTF8(atom_getsymbol(argv + i)->s_name) + "\n";
        }
        
        return result;
        
    }
};
