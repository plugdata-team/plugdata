
struct CodeEditor
{
    
};

struct TextDefineObject final : public TextBase
{

    CodeEditor editor;
    
    TextDefineObject(void* obj, Box* parent) : TextBase(obj, parent, true)
    {
        
    }
};
