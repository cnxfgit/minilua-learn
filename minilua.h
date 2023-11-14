//
// Created by hlx on 2023/11/2.
//

#ifndef MINILUA_H
#define MINILUA_H

/* This is a heavily customized and minimized copy of Lua 5.1.5. */
/* It's only used to build LuaJIT. It does NOT have all standard functions! */
/******************************************************************************
* Copyright (C) 1994-2012 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _MSC_VER
typedef unsigned __int64 U64;
#else
typedef unsigned long long U64;
#endif

#define LUA_TNONE           (-1)

#define LUA_TNIL            0
#define LUA_TBOOLEAN        1
#define LUA_TLIGHTUSERDATA  2
#define LUA_TNUMBER         3
#define LUA_TSTRING         4
#define LUA_TTABLE          5
#define LUA_TFUNCTION       6
#define LUA_TUSERDATA       7
#define LUA_TTHREAD         8


// Possible states of the Garbage Collector
#define GCS_PAUSE        0   // 表示垃圾回收器处于暂停状态
#define GCS_PROPAGATE    1   // 表示垃圾回收器正在遍历对象并标记可达对象
#define GCS_SWEEPSTRING    2   // 表示垃圾回收器正在清理字符串对象
#define GCS_SWEEP        3   // 表示垃圾回收器正在进行清理操作
#define GCS_FINALIZE    4   // 表示垃圾回收器正在执行对象的终结操作

// pseudo-indices
#define LUA_REGISTRY_INDEX        (-10000)                // 表示 Lua 注册表的索引
#define LUA_ENVIRON_INDEX        (-10001)                // 表示环境表的索引
#define LUA_GLOBALS_INDEX        (-10002)                // 表示全局环境的索引
#define LUA_UPVALUE_INDEX(i)    (LUA_GLOBALS_INDEX-(i))  // 计算上值的索引

#define LUAI_MAXCCALLS		200     // lua调用c函数的最大调用栈

#define LUA_YIELD	    1
#define LUA_ERRRUN	    2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	    4
#define LUA_ERRERR	    5

#define LUA_QL(x)"'"x"'"

typedef struct lua_State lua_State;

typedef int(*lua_CFunction)(lua_State *L);

typedef double lua_Number;
typedef ptrdiff_t lua_Integer;

typedef const char *(*lua_Reader)(lua_State *L, void *ud, size_t *sz);

typedef void *(*lua_Alloc)(void *userdata, void *ptr, size_t oldSize, size_t newSize);

typedef unsigned int lu_int32;
typedef size_t lu_mem;
typedef ptrdiff_t l_mem;
typedef unsigned char lu_byte;
typedef double l_uacNumber;

typedef struct luaL_Reg {
    const char *name;
    lua_CFunction func;
} luaL_Reg;


#define UNUSED(x)((void)(x))

typedef lu_int32 Instruction;

typedef union GCObject GCObject;


typedef struct Zio ZIO;
#define G(L) (L->l_G)

// state manipulation
lua_State *lua_newState(lua_Alloc f, void *userdata);

void lua_close(lua_State *L);

lua_CFunction lua_atPanic(lua_State *L, lua_CFunction panicFunc);


// basic stack manipulation
int lua_getTop(lua_State *L);

void lua_setTop(lua_State *L, int idx);

void lua_pushValue(lua_State *L, int idx);

void lua_remove(lua_State *L, int idx);

void lua_insert(lua_State *L, int idx);

void lua_replace(lua_State *L, int idx);

int lua_checkStack(lua_State *L, int size);


// access functions (stack -> C)

int lua_isnumber(lua_State *L, int idx);

int lua_isstring(lua_State *L, int idx);

int lua_iscfunction(lua_State *L, int idx);

int lua_type(lua_State *L, int idx);

const char *lua_typename(lua_State *L, int t);

int lua_rawequal(lua_State *L, int index1, int index2);

int lua_lessthan(lua_State *L, int index1, int index2);

lua_Number lua_tonumber(lua_State *L, int idx);

lua_Integer lua_tointeger(lua_State *L, int idx);

int lua_toboolean(lua_State *L, int idx);

const char *lua_tolstring(lua_State *L, int idx, size_t *len);

size_t lua_objlen(lua_State *L, int idx);

lua_CFunction lua_tocfunction(lua_State *L, int idx);

void *lua_touserdata(lua_State *L, int idx);


// push functions (C -> stack)
void lua_pushnil(lua_State *L);

void lua_pushnumber(lua_State *L, lua_Number n);

void lua_pushinteger(lua_State *L, lua_Integer n);

void lua_pushlstring(lua_State *L, const char *s, size_t len);

void lua_pushstring(lua_State *L, const char *s);

const char *lua_pushvfstring(lua_State *L, const char *fmt, va_list argp);

const char *lua_pushfstring(lua_State *L, const char *fmt, ...);

void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);

void lua_pushboolean(lua_State *L, int b);

int lua_pushthread(lua_State *L);


// get functions (Lua -> stack)
void lua_gettable(lua_State *L, int idx);

void lua_getField(lua_State *L, int idx, const char *k);

void lua_rawget(lua_State *L, int idx);

void lua_rawGetI(lua_State *L, int idx, int n);

void lua_createTable(lua_State *L, int nArr, int nRec);

void *lua_newUserdata(lua_State *L, size_t size);

int lua_getmetatable(lua_State *L, int objindex);

void lua_getfenv(lua_State *L, int idx);


// set functions (stack -> Lua)
void lua_settable(lua_State *L, int idx);

void lua_setField(lua_State *L, int idx, const char *k);

void lua_rawset(lua_State *L, int idx);

void lua_rawSetI(lua_State *L, int idx, int n);

int lua_setmetatable(lua_State *L, int objindex);

int lua_setfenv(lua_State *L, int idx);


// `load' and `call' functions (load and run Lua code)
void lua_call(lua_State *L, int nargs, int nResults);

int lua_pcall(lua_State *L, int nargs, int nResults, int errfunc);

int lua_load(lua_State *L, lua_Reader reader, void *data, const char *chunkname);


// miscellaneous functions
int lua_error(lua_State *L);

int lua_next(lua_State *L, int idx);

void lua_concat(lua_State *L, int n);


// some useful macros
#define lua_pop(L, n) lua_setTop(L,-(n)-1)
#define lua_newtable(L) lua_createTable(L,0,0)
#define lua_pushcfunction(L, f) lua_pushcclosure(L,(f),0)
#define lua_strlen(L, i) lua_objlen(L,(i))
#define lua_isfunction(L, n) (lua_type(L,(n))==LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type(L,(n))==LUA_TTABLE)
#define lua_isnil(L, n) (lua_type(L,(n))==LUA_TNIL)
#define lua_isboolean(L, n) (lua_type(L,(n))==LUA_TBOOLEAN)
#define lua_isnone(L, n) (lua_type(L,(n))==LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L,(n))<=0)

#define lua_pushliteral(L, s) lua_pushlstring(L,""s,(sizeof(s)/sizeof(char))-1)
#define lua_setglobal(L, s) lua_setField(L,LUA_GLOBALS_INDEX,(s))
#define lua_tostring(L, i) lua_tolstring(L,(i),NULL)


// Debug API
typedef struct lua_Debug {
    int event;
    const char *name;
    const char *namewhat;
    const char *what;
    const char *source;
    int currentline;
    int nups;
    int linedefined;
    int lastlinedefined;
    char short_src[60];
    int i_ci;
} lua_Debug;

typedef void(*lua_Hook)(lua_State *L, lua_Debug *ar);

int lua_getstack(lua_State *L, int level, lua_Debug *ar);

int lua_getinfo(lua_State *L, const char *what, lua_Debug *ar);

#endif //MINILUA_H
