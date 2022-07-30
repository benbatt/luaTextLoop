#ifndef COMMON_H
#define COMMON_H

#include <lauxlib.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #define LUATEXTLOOP_EXPORT __declspec(dllexport)
#endif

#endif /* COMMON_H */
