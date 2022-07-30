/*
Copyright 2022 Ben Batt

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "common.h"
#include <conio.h>

static int asciiKeyCode(int code)
{
    if ('a' <= code && code <= 'z')
    {
        return code - 'a' + 'A';
    }

    return code;
}

static int extendedKeyCode(int codeA, int codeB)
{
    return codeB | 0x100;
}

static void getKeyPresses(lua_State *L)
{
    lua_newtable(L);

    for (int i = 1; _kbhit(); ++i)
    {
        int key = _getch();

        if (key == 0 || key == 0xE0)
        {
            key = extendedKeyCode(key, _getch());
        }
        else
        {
            key = asciiKeyCode(key);
        }

        lua_pushinteger(L, i);
        lua_pushinteger(L, key);
        lua_settable(L, -3);
    }
}

static int start(lua_State *L)
{
    const int sleepTime = luaL_checkint(L, 1);

    const int callbackIndex = 2;
    luaL_checktype(L, callbackIndex, LUA_TFUNCTION);

    while (1)
    {
        lua_pushvalue(L, callbackIndex);

        getKeyPresses(L);

        lua_call(L, 1, 1);

        if (!lua_toboolean(L, -1))
        {
            break;
        }

        Sleep(sleepTime);
    }

    return 0;
}

luaL_Reg StaticFunctions[] = {
    { "start", start },
    { NULL, NULL }
};

static void createKeyCodesTable(lua_State *L);
static void createKeyNamesTable(lua_State *L);

extern int LUATEXTLOOP_EXPORT luaopen_luaTextLoop(lua_State *L)
{
    luaL_register(L, "TextLoop", StaticFunctions);

    createKeyCodesTable(L);
    lua_setfield(L, -2, "KeyCode");

    createKeyNamesTable(L);
    lua_setfield(L, -2, "KeyName");

    return 1;
}

typedef void (*KeyCodeCallback)(lua_State *L, int code, const char *name);

typedef struct KeyCodeEntry {
    int code;
    const char *name;
} KeyCodeEntry;

KeyCodeEntry KeyCodeEntries[] = {
    /* ASCII codes */
    { 9, "Tab" },
    { 13, "Return" },
    { 27, "Escape" },
    { ' ', "Space" },
    { '0', "Zero" },
    { '1', "One" },
    { '2', "Two" },
    { '3', "Three" },
    { '4', "Four" },
    { '5', "Five" },
    { '6', "Six" },
    { '7', "Seven" },
    { '8', "Eight" },
    { '9', "Nine" },
    { ':', "Colon" },
    { ';', "Semicolon" },
    { '\'', "SingleQuote" },
    { '"', "DoubleQuote" },
    { ',', "Comma" },
    { '<', "LessThan" },
    { '.', "Period" },
    { '>', "GreaterThan" },
    { '[', "LeftBracket" },
    { ']', "RightBracket" },
    { '{', "LeftBrace" },
    { '}', "RightBrace" },
    { '\\', "Backslash" },
    { '|', "Pipe" },
    { '`', "Backtick" },
    { '~', "Tilde" },
    { '!', "ExclamationMark" },
    { '@', "At" },
    { '#', "Hash" },
    { '$', "Dollar" },
    { '%', "Percent" },
    { '^', "Caret" },
    { '&', "Ampersand" },
    { '*', "Asterisk" },
    { '(', "LeftParenthesis" },
    { ')', "RightParenthesis" },
    { '-', "Minus" },
    { '+', "Plus" },
    { '_', "Underscore" },
    { '=', "Equals" },
    { '/', "Slash" },
    { '?', "QuestionMark" },

    /* Extended codes */
    { 331, "Left" },
    { 333, "Right" },
    { 328, "Up" },
    { 336, "Down" },
    { 329, "PageUp" },
    { 337, "PageDown" },
    { 327, "Home" },
    { 335, "End" },
    { 338, "Insert" },
    { 339, "Delete" },
    { 315, "F1" },
    { 316, "F2" },
    { 317, "F3" },
    { 318, "F4" },
    { 319, "F5" },
    { 320, "F6" },
    { 321, "F7" },
    { 322, "F8" },
    { 323, "F9" },
    { 324, "F10" },
    { 389, "F11" },
    { 390, "F12" },
    { 0, NULL }
};

static void enumerateKeyCodes(lua_State *L, KeyCodeCallback callback)
{
    /* ASCII codes */
    for (int code = 'A'; code <= 'Z'; ++code) {
        char name[2] = { code, '\0' };
        callback(L, code, name);
    }

    for (KeyCodeEntry *entry = KeyCodeEntries; entry->name; ++entry)
    {
        callback(L, entry->code, entry->name);
    }
}

static void setKeycodeValue(lua_State *L, int code, const char *name)
{
    lua_pushinteger(L, code);
    lua_setfield(L, -2, name);
}

static void createKeyCodesTable(lua_State *L)
{
    lua_newtable(L);

    enumerateKeyCodes(L, setKeycodeValue);
}

static void setKeycodeName(lua_State *L, int code, const char *name)
{
    lua_pushinteger(L, code);
    lua_pushstring(L, name);
    lua_settable(L, -3);
}

static void createKeyNamesTable(lua_State *L)
{
    lua_newtable(L);

    enumerateKeyCodes(L, setKeycodeName);
}
