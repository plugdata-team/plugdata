#include "src/Keyboard.cpp"
#include "src/KeyboardFactory.cpp"

#if defined (_WIN32)
#include "src/WindowsKeyboard.cpp"
#elif defined(__linux__)
#include "src/LinuxKeyboard.cpp"
#endif
