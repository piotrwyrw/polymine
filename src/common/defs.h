#ifndef DEBUG_MSGS
#define DEBUG_MSGS
#endif

#ifdef DEBUG_MSGS
#define DEBUG(...) __VA_ARGS__
#else
#define DEBUG(...)
#endif
