
struct TokenString
{
    std::vector<std::string> buffer;
    
    TokenString(const std::string& preset, char splitter, char escape_char)
    {
        std::stringstream test(preset);
        std::string segment;
        
        // split save file by seperator
        while(std::getline(test, segment, splitter)) buffer.push_back(segment);
    }
    
    std::string* begin() {
        return buffer.data();
    }
    
    size_t size() {
        return buffer.size();
    }
    
    std::string* end() {
        return buffer.data() + buffer.size();
    }
    
    std::string& operator[] (int idx) {
        return buffer[idx];
    }
};
