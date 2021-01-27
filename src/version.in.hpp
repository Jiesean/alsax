
#ifdef DEBUG
    #define VER ("[compile:Debug:@gitname@:@gitbranch@:@gitver@:@timestamp@]")
#else
    #define VER ("[compile:Release:@gitname@:@gitbranch@:@gitver@:@timestamp@]")
#endif
const char *basex_ver=VER;

#define GETVER ("BasexVer_@gitver@")
const char *basex_getver=GETVER;

