#ifdef __ANDROID__
// To make upx work for Android
void _init(void) __attribute__((constructor));
void _init(void) {}
#endif
