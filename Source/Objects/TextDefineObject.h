
// Actual text object, marked final for optimisation
struct TextDefineObject final : public TextBase
{
    TextDefineObject(void* obj, Box* parent, bool isValid = true) : TextBase(obj, parent, isValid)
    {
    }
};
