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
#include <stdlib.h>

static HANDLE StandardOutput = INVALID_HANDLE_VALUE;
static HANDLE ScreenBuffer = INVALID_HANDLE_VALUE;

static COORD ScreenBufferSize = { 0 };
static CHAR_INFO *CopyBuffer = NULL;

static int OverlayBottom = 0;

static void initializeConsole(lua_State *L)
{
    StandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (StandardOutput == INVALID_HANDLE_VALUE)
    {
        luaL_error(L, "Couldn't get the standard output handle");
        return;
    }

    ScreenBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

    if (ScreenBuffer == INVALID_HANDLE_VALUE)
    {
        luaL_error(L, "Couldn't get the standard output handle");
        return;
    }

    /* Hide the cursor */
    CONSOLE_CURSOR_INFO cursorInfo = { 100, FALSE };
    SetConsoleCursorInfo(ScreenBuffer, &cursorInfo);

    /* Allocate the copy buffer */
    CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
    GetConsoleScreenBufferInfo(ScreenBuffer, &screenBufferInfo);

    ScreenBufferSize = screenBufferInfo.dwSize;
    CopyBuffer = malloc(sizeof(*CopyBuffer) * ScreenBufferSize.X * ScreenBufferSize.Y);
}

static void copyStandardOutput()
{
    int remainingHeight = ScreenBufferSize.Y - 1 - OverlayBottom;

    if (remainingHeight > 0)
    {
        CONSOLE_SCREEN_BUFFER_INFO standardOutputInfo;
        GetConsoleScreenBufferInfo(StandardOutput, &standardOutputInfo);

        const COORD *cursor = &standardOutputInfo.dwCursorPosition;
        const SMALL_RECT *outputWindow = &standardOutputInfo.srWindow;

        COORD bufferPosition = { 0, 0 };
        SMALL_RECT readRect = {
            outputWindow->Left,
            max(0, cursor->Y - remainingHeight),
            outputWindow->Right,
            cursor->Y
        };

        ReadConsoleOutputA(StandardOutput, CopyBuffer, ScreenBufferSize, bufferPosition, &readRect);

        int readHeight = readRect.Bottom - readRect.Top;

        SMALL_RECT writeRect;
        writeRect.Left = 0;
        writeRect.Top = OverlayBottom + 1;
        writeRect.Right = ScreenBufferSize.X - 1;
        writeRect.Bottom = writeRect.Top + readHeight;

        WriteConsoleOutputA(ScreenBuffer, CopyBuffer, ScreenBufferSize, bufferPosition, &writeRect);
    }
}

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

static int setOverlay(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    size_t lineCount = lua_objlen(L, 1);

    SMALL_RECT writeRect = { 0, 0, 0, lineCount - 1 };

    lua_getfield(L, 1, "x");
    writeRect.Left = lua_tointeger(L, -1);

    lineCount = min(lineCount, ScreenBufferSize.Y);

    size_t maxLength = 0;

    for (int y = 0; y < lineCount; ++y)
    {
        lua_pushinteger(L, y + 1);
        lua_gettable(L, 1);

        size_t length = 0;
        const char *line = lua_tolstring(L, -1, &length);

        maxLength = max(maxLength, length);

        length = min(length, ScreenBufferSize.X);

        for (int x = 0; x < length; ++x)
        {
            CHAR_INFO *charInfo = &CopyBuffer[x + (y * ScreenBufferSize.X)];

            charInfo->Char.AsciiChar = line[x];
            charInfo->Attributes = 0x07; /* White */
        }

        for (int x = length; x < ScreenBufferSize.X; ++x)
        {
            CHAR_INFO *charInfo = &CopyBuffer[x + (y * ScreenBufferSize.X)];

            charInfo->Char.AsciiChar = ' ';
            charInfo->Attributes = 0x07; /* White */
        }
    }

    writeRect.Right = writeRect.Left + maxLength - 1;

    COORD bufferPosition = { 0, 0 };

    WriteConsoleOutputA(ScreenBuffer, CopyBuffer, ScreenBufferSize, bufferPosition, &writeRect);

    OverlayBottom = writeRect.Bottom;

    copyStandardOutput();

    return 0;
}

static int start(lua_State *L)
{
    const int sleepTime = luaL_checkint(L, 1);

    const int callbackIndex = 2;
    luaL_checktype(L, callbackIndex, LUA_TFUNCTION);

    SetConsoleActiveScreenBuffer(ScreenBuffer);

    while (1)
    {
        lua_pushvalue(L, callbackIndex);

        getKeyPresses(L);

        lua_call(L, 1, 1);

        copyStandardOutput();

        if (!lua_toboolean(L, -1))
        {
            break;
        }

        Sleep(sleepTime);
    }

    SetConsoleActiveScreenBuffer(StandardOutput);

    return 0;
}

luaL_Reg StaticFunctions[] = {
    { "setOverlay", setOverlay },
    { "start", start },
    { NULL, NULL }
};

static void createKeyCodesTable(lua_State *L);
static void createKeyNamesTable(lua_State *L);

extern int LUATEXTLOOP_EXPORT luaopen_luaTextLoop(lua_State *L)
{
    initializeConsole(L);

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
