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
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#ifdef _MSC_VER
typedef unsigned __int64 U64;
#else
typedef unsigned long long U64;
#endif

typedef enum {
    TM_INDEX,       // 当表中的一个键在表中不存在时，Lua 会调用这个元方法来获取该键对应的值
    TM_NEWINDEX,    // 当表中的一个键被添加或修改时，Lua 会调用这个元方法来设置该键对应的值
    TM_GC,          // 当 Lua 的垃圾回收器回收一个对象时，会调用该对象的 __gc 元方法
    TM_MODE,        // 可以通过设置该元方法将表作为弱表进行创建
    TM_EQ,          // 该元方法用于重载 == 操作符
    TM_ADD,         // 该元方法用于重载 + 操作符
    TM_SUB,         // 该元方法用于重载 - 操作符
    TM_MUL,         // 该元方法用于重载 * 操作符
    TM_DIV,         // 该元方法用于重载 \ 操作符
    TM_MOD,         // 该元方法用于重载 % 操作符
    TM_POW,         // 该元方法用于重载 ^ 操作符
    TM_UNM,         // 该元方法用于重载 - 操作符
    TM_LEN,         // 该元方法用于重载 # 操作符
    TM_LT,          // 该元方法用于重载 < 操作符
    TM_LE,          // 该元方法用于重载 <= 操作符
    TM_CONCAT,      // 该元方法用于重载 .. 操作符
    TM_CALL,        // 该元方法用于重载函数调用操作
    TM_N            // TM_N 是 TMS 中用于表示未定义元方法的占位符。
} TMS;

enum OpMode {
    // 这是最常见的指令模式。它使用一个9位字段来表示操作码（Opcode）和寄存器参数，并使用三个9位字段来表示操作数。
    iABC,
    // 该模式使用一个9位字段来表示操作码和寄存器参数，并使用一个18位字段来表示操作数。
    iABx,
    // 类似于 iABx，但是操作数是带符号的，可以表示负数值，在表示跳转时特别有用。
    iAsBx
};

typedef enum {
    OP_MOVE,        // 将一个寄存器中的值复制到另一个寄存器中
    OP_LOADK,       // 将一个常量加载到寄存器中
    OP_LOADBOOL,    // 将一个布尔值加载到寄存器中
    OP_LOADNIL,     // 将一段连续的寄存器置为 nil
    OP_GETUPVAL,    // 从上值表中获取一个值到寄存器中
    OP_GETGLOBAL,   // 从全局变量表中获取一个值到寄存器中
    OP_GETTABLE,    // 从表中获取一个值到寄存器中
    OP_SETGLOBAL,   // 将寄存器中的值设置到全局变量表中
    OP_SETUPVAL,    // 将寄存器中的值设置到上值表中
    OP_SETTABLE,    // 将寄存器中的值设置到表中
    OP_NEWTABLE,    // 创建一个新的表
    OP_SELF,        // 将表和键加载到寄存器中
    OP_ADD,         // [+]
    OP_SUB,         // [-]
    OP_MUL,         // [*]
    OP_DIV,         // [/]
    OP_MOD,         // [%]
    OP_POW,         // [^]
    OP_UNM,         // [-]相反数
    OP_NOT,         // not取反
    OP_LEN,         // [#]取长度
    OP_CONCAT,      // [..]字符串连接符
    OP_JMP,         // 无条件跳转
    OP_EQ,          // 等于
    OP_LT,          // 小于
    OP_LE,          // 小于等于
    OP_TEST,        // 条件测试
    OP_TESTSET,     // 条件测试 并在条件为真时设置寄存器
    OP_CALL,        // 函数调用
    OP_TAILCALL,    // 尾调用
    OP_RETURN,      // 函数返回
    OP_FORLOOP,     // 数字型循环的迭代器
    OP_FORPREP,     // 数字型循环的初始化
    OP_TFORLOOP,    // 泛型循环的迭代器
    OP_SETLIST,     // 批量设置表元素
    OP_CLOSE,       // 关闭一个函数
    OP_CLOSURE,     // 用于创建一个闭包
    OP_VARARG       // 用于获取可变数量的参数
} OpCode;

enum OpArgMask {
    OpArgN,     // 表示没有附加参数（None），指令不需要额外的操作数
    OpArgU,     // 表示一个未使用的参数（Unused），指令可能有一个未使用的参数
    OpArgR,     // 表示一个寄存器参数（Register），指令的操作数是一个寄存器索引
    OpArgK      // 表示一个常量参数（Constant），指令的操作数是一个常量索引
};

typedef enum {
    VVOID,      // 表示空值表达式
    VNIL,       // 表示 nil 值表达式
    VTRUE,      // 表示 true 值表达式
    VFALSE,     // 表示 false 值表达式
    VK,         // 表示常量表达式
    VKNUM,      // 表示数字常量表达式
    VLOCAL,     // 表示局部变量表达式
    VUPVAL,     // 表示上值（upvalue）表达式
    VGLOBAL,    // 表示全局变量表达式
    VINDEXED,   // 表示索引表达式
    VJMP,       // 表示跳转表达式
    VRELOCABLE, // 表示可重定位表达式
    VNONRELOC,  // 表示非重定位表达式
    VCALL,      // 表示函数调用表达式
    VVARARG     // 表示可变参数表达式
} expkind;

// 保留字和标识符
enum RESERVED {
    TK_AND = 257, TK_BREAK,
    TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
    TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
    TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
    TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_NUMBER,
    TK_NAME, TK_STRING, TK_EOS
};

// 二元操作符
typedef enum BinOpr {
    OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_MOD, OPR_POW,
    OPR_CONCAT,
    OPR_NE, OPR_EQ,
    OPR_LT, OPR_LE, OPR_GT, OPR_GE,
    OPR_AND, OPR_OR,
    OPR_NOBINOPR
} BinOpr;

// 一元操作符
typedef enum UnOpr {
    OPR_MINUS, OPR_NOT, OPR_LEN, OPR_NOUNOPR
} UnOpr;

#define LUA_QL(x)"'"x"'"
#define luai_apicheck(L, o){(void)L;}
#define lua_number2str(s, n)sprintf((s),"%.14g",(n))
#define lua_str2number(s, p)strtod((s),(p))
#define luai_numadd(a, b)((a)+(b))
#define luai_numsub(a, b)((a)-(b))
#define luai_nummul(a, b)((a)*(b))
#define luai_numdiv(a, b)((a)/(b))
#define luai_nummod(a, b)((a)-floor((a)/(b))*(b))
#define luai_numpow(a, b)(pow(a,b))
#define luai_numunm(a)(-(a))
#define luai_numeq(a, b)((a)==(b))
#define luai_numlt(a, b)((a)<(b))
#define luai_numle(a, b)((a)<=(b))
#define luai_numisnan(a)(!luai_numeq((a),(a)))
#define lua_number2int(i, d)((i)=(int)(d))
#define lua_number2integer(i, d)((i)=(lua_Integer)(d))
#define LUAI_THROW(L, c)longjmp((c)->b,1)
#define LUAI_TRY(L, c, a)if(setjmp((c)->b)==0){a}
#define lua_pclose(L, file)((void)((void)L,file),0)
#define lua_upvalueindex(i)((-10002)-(i))

typedef struct lua_State lua_State;

typedef int(*lua_CFunction)(lua_State *L);

typedef double lua_Number;
typedef ptrdiff_t lua_Integer;

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

typedef struct luaL_Reg {
    const char *name;
    lua_CFunction func;
} luaL_Reg;

typedef struct luaL_Buffer {
    char *p;
    int lvl;
    lua_State *L;
    char buffer[BUFSIZ];
} luaL_Buffer;

typedef const char *(*lua_Reader)(lua_State *L, void *ud, size_t *sz);

typedef void *(*lua_Alloc)(void *ud, void *ptr, size_t osize, size_t nsize);

typedef void(*lua_Hook)(lua_State *L, lua_Debug *ar);

typedef unsigned int lu_int32;
typedef size_t lu_mem;
typedef ptrdiff_t l_mem;
typedef unsigned char lu_byte;
#define IntPoint(p)((unsigned int)(lu_mem)(p))
typedef union {
    double u;
    void *s;
    long l;
} L_Umaxalign;
typedef double l_uacNumber;
#define check_exp(c, e)(e)
#define UNUSED(x)((void)(x))
#define cast(t, exp)((t)(exp))
#define cast_byte(i)cast(lu_byte,(i))
#define cast_num(i)cast(lua_Number,(i))
#define cast_int(i)cast(int,(i))
typedef lu_int32 Instruction;
#define condhardstacktests(x)((void)0)
typedef union GCObject GCObject;
typedef struct GCheader {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
} GCheader;
typedef union {
    GCObject *gc;
    void *p;
    lua_Number n;
    int b;
} Value;
typedef struct lua_TValue {
    Value value;
    int tt;
} TValue;
#define ttisnil(o)(ttype(o)==0)
#define ttisnumber(o)(ttype(o)==3)
#define ttisstring(o)(ttype(o)==4)
#define ttistable(o)(ttype(o)==5)
#define ttisfunction(o)(ttype(o)==6)
#define ttisboolean(o)(ttype(o)==1)
#define ttisuserdata(o)(ttype(o)==7)
#define ttisthread(o)(ttype(o)==8)
#define ttislightuserdata(o)(ttype(o)==2)
#define ttype(o)((o)->tt)
#define gcvalue(o)check_exp(iscollectable(o),(o)->value.gc)
#define pvalue(o)check_exp(ttislightuserdata(o),(o)->value.p)
#define nvalue(o)check_exp(ttisnumber(o),(o)->value.n)
#define rawtsvalue(o)check_exp(ttisstring(o),&(o)->value.gc->ts)
#define tsvalue(o)(&rawtsvalue(o)->tsv)
#define rawuvalue(o)check_exp(ttisuserdata(o),&(o)->value.gc->u)
#define uvalue(o)(&rawuvalue(o)->uv)
#define clvalue(o)check_exp(ttisfunction(o),&(o)->value.gc->cl)
#define hvalue(o)check_exp(ttistable(o),&(o)->value.gc->h)
#define bvalue(o)check_exp(ttisboolean(o),(o)->value.b)
#define thvalue(o)check_exp(ttisthread(o),&(o)->value.gc->th)
#define l_isfalse(o)(ttisnil(o)||(ttisboolean(o)&&bvalue(o)==0))
#define checkconsistency(obj)
#define checkliveness(g, obj)
#define setnilvalue(obj)((obj)->tt=0)
#define setnvalue(obj, x){TValue*i_o=(obj);i_o->value.n=(x);i_o->tt=3;}
#define setbvalue(obj, x){TValue*i_o=(obj);i_o->value.b=(x);i_o->tt=1;}
#define setsvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=4;checkliveness(G(L),i_o);}
#define setuvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=7;checkliveness(G(L),i_o);}
#define setthvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=8;checkliveness(G(L),i_o);}
#define setclvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=6;checkliveness(G(L),i_o);}
#define sethvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=5;checkliveness(G(L),i_o);}
#define setptvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=(8+1);checkliveness(G(L),i_o);}
#define setobj(L, obj1, obj2){const TValue*o2=(obj2);TValue*o1=(obj1);o1->value=o2->value;o1->tt=o2->tt;checkliveness(G(L),o1);}
#define setttype(obj, tt)(ttype(obj)=(tt))
#define iscollectable(o)(ttype(o)>=4)
typedef TValue *StkId;


typedef union TString {
    L_Umaxalign dummy;
    struct {
        GCObject *next;
        lu_byte tt;
        lu_byte marked;
        lu_byte reserved;
        unsigned int hash;
        size_t len;
    } tsv;
} TString;
#define getstr(ts)cast(const char*,(ts)+1)
#define svalue(o)getstr(rawtsvalue(o))
typedef union Udata {
    L_Umaxalign dummy;
    struct {
        GCObject *next;
        lu_byte tt;
        lu_byte marked;
        struct Table *metatable;
        struct Table *env;
        size_t len;
    } uv;
} Udata;
typedef struct Proto {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    TValue *k;
    Instruction *code;
    struct Proto **p;
    int *lineinfo;
    struct LocVar *locvars;
    TString **upvalues;
    TString *source;
    int sizeupvalues;
    int sizek;
    int sizecode;
    int sizelineinfo;
    int sizep;
    int sizelocvars;
    int linedefined;
    int lastlinedefined;
    GCObject *gclist;
    lu_byte nups;
    lu_byte numparams;
    lu_byte is_vararg;
    lu_byte maxstacksize;
} Proto;
typedef struct LocVar {
    TString *varname;
    int startpc;
    int endpc;
} LocVar;
typedef struct UpVal {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    TValue *v;
    union {
        TValue value;
        struct {
            struct UpVal *prev;
            struct UpVal *next;
        } l;
    } u;
} UpVal;
typedef struct CClosure {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    lu_byte isC;
    lu_byte nupvalues;
    GCObject *gclist;
    struct Table *env;
    lua_CFunction f;
    TValue upvalue[1];
} CClosure;
typedef struct LClosure {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    lu_byte isC;
    lu_byte nupvalues;
    GCObject *gclist;
    struct Table *env;
    struct Proto *p;
    UpVal *upvals[1];
} LClosure;
typedef union Closure {
    CClosure c;
    LClosure l;
} Closure;

typedef union TKey {
    struct {
        Value value;
        int tt;
        struct Node *next;
    } nk;
    TValue tvk;
} TKey;
typedef struct Node {
    TValue i_val;
    TKey i_key;
} Node;
typedef struct Table {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    lu_byte flags;
    lu_byte lsizenode;
    struct Table *metatable;
    TValue *array;
    Node *node;
    Node *lastfree;
    GCObject *gclist;
    int sizearray;
} Table;
#define lmod(s, size)(check_exp((size&(size-1))==0,(cast(int,(s)&((size)-1)))))
#define twoto(x)((size_t)1<<(x))
#define sizenode(t)(twoto((t)->lsizenode))
static const TValue luaO_nilobject_;
#define ceillog2(x)(luaO_log2((x)-1)+1)

static int luaO_log2(unsigned int x);

#define gfasttm(g, et, e)((et)==NULL?NULL:((et)->flags&(1u<<(e)))?NULL:luaT_gettm(et,e,(g)->tmname[e]))
#define fasttm(l, et, e)gfasttm(G(l),et,e)

static const TValue *luaT_gettm(Table *events, TMS event, TString *ename);

#define luaM_reallocv(L, b, on, n, e)((cast(size_t,(n)+1)<=((size_t)(~(size_t)0)-2)/(e))?luaM_realloc_(L,(b),(on)*(e),(n)*(e)):luaM_toobig(L))
#define luaM_freemem(L, b, s)luaM_realloc_(L,(b),(s),0)
#define luaM_free(L, b)luaM_realloc_(L,(b),sizeof(*(b)),0)
#define luaM_freearray(L, b, n, t)luaM_reallocv(L,(b),n,0,sizeof(t))
#define luaM_malloc(L, t)luaM_realloc_(L,NULL,0,(t))
#define luaM_new(L, t)cast(t*,luaM_malloc(L,sizeof(t)))
#define luaM_newvector(L, n, t)cast(t*,luaM_reallocv(L,NULL,0,n,sizeof(t)))
#define luaM_growvector(L, v, nelems, size, t, limit, e)if((nelems)+1>(size))((v)=cast(t*,luaM_growaux_(L,v,&(size),sizeof(t),limit,e)))
#define luaM_reallocvector(L, v, oldn, n, t)((v)=cast(t*,luaM_reallocv(L,v,oldn,n,sizeof(t))))

#define iscfunction(o)(ttype(o)==6&&clvalue(o)->c.isC)

typedef struct Zio ZIO;
#define char2int(c)cast(int,cast(unsigned char,(c)))
#define zgetc(z)(((z)->n--)>0?char2int(*(z)->p++):luaZ_fill(z))
typedef struct Mbuffer {
    char *buffer;
    size_t n;
    size_t buffsize;
} Mbuffer;
#define luaZ_initbuffer(L, buff)((buff)->buffer=NULL,(buff)->buffsize=0)
#define luaZ_buffer(buff)((buff)->buffer)
#define luaZ_sizebuffer(buff)((buff)->buffsize)
#define luaZ_bufflen(buff)((buff)->n)
#define luaZ_resetbuffer(buff)((buff)->n=0)
#define luaZ_resizebuffer(L, buff, size)(luaM_reallocvector(L,(buff)->buffer,(buff)->buffsize,size,char),(buff)->buffsize=size)
#define luaZ_freebuffer(L, buff)luaZ_resizebuffer(L,buff,0)
struct Zio {
    size_t n;
    const char *p;
    lua_Reader reader;
    void *data;
    lua_State *L;
};

static int luaZ_fill(ZIO *z);

struct lua_longjmp;
#define gt(L)(&L->l_gt)
#define registry(L)(&G(L)->l_registry)
typedef struct stringtable {
    GCObject **hash;
    lu_int32 nuse;
    int size;
} stringtable;
typedef struct CallInfo {
    StkId base;
    StkId func;
    StkId top;
    const Instruction *savedpc;
    int nresults;
    int tailcalls;
} CallInfo;
#define curr_func(L)(clvalue(L->ci->func))
#define ci_func(ci)(clvalue((ci)->func))
#define f_isLua(ci)(!ci_func(ci)->c.isC)
#define isLua(ci)(ttisfunction((ci)->func)&&f_isLua(ci))
typedef struct global_State {
    stringtable strt;
    lua_Alloc frealloc;
    void *ud;
    lu_byte currentwhite;
    lu_byte gcstate;
    int sweepstrgc;
    GCObject *rootgc;
    GCObject **sweepgc;
    GCObject *gray;
    GCObject *grayagain;
    GCObject *weak;
    GCObject *tmudata;
    Mbuffer buff;
    lu_mem GCthreshold;
    lu_mem totalbytes;
    lu_mem estimate;
    lu_mem gcdept;
    int gcpause;
    int gcstepmul;
    lua_CFunction panic;
    TValue l_registry;
    struct lua_State *mainthread;
    UpVal uvhead;
    struct Table *mt[(8 + 1)];
    TString *tmname[TM_N];
} global_State;
struct lua_State {
    GCObject *next;
    lu_byte tt;
    lu_byte marked;
    lu_byte status;
    StkId top;
    StkId base;
    global_State *l_G;
    CallInfo *ci;
    const Instruction *savedpc;
    StkId stack_last;
    StkId stack;
    CallInfo *end_ci;
    CallInfo *base_ci;
    int stacksize;
    int size_ci;
    unsigned short nCcalls;
    unsigned short baseCcalls;
    lu_byte hookmask;
    lu_byte allowhook;
    int basehookcount;
    int hookcount;
    lua_Hook hook;
    TValue l_gt;
    TValue env;
    GCObject *openupval;
    GCObject *gclist;
    struct lua_longjmp *errorJmp;
    ptrdiff_t errfunc;
};
#define G(L)(L->l_G)
union GCObject {
    GCheader gch;
    union TString ts;
    union Udata u;
    union Closure cl;
    struct Table h;
    struct Proto p;
    struct UpVal uv;
    struct lua_State th;
};

lua_Number lua_tonumber(lua_State *L, int idx);

int lua_toboolean(lua_State *L, int idx);

void *lua_touserdata(lua_State *L, int idx);

lua_Integer lua_tointeger(lua_State *L, int idx);

const char *lua_tolstring(lua_State *L, int idx, size_t *len);

lua_CFunction lua_tocfunction(lua_State *L, int idx);

int lua_isnumber(lua_State *L, int idx);

int lua_isstring(lua_State *L, int idx);

int lua_gettop(lua_State *L);

int lua_getstack(lua_State *L, int level, lua_Debug *ar);

int lua_getinfo(lua_State *L, const char *what, lua_Debug *ar);

void lua_getfield(lua_State *L, int idx, const char *k);

int lua_getmetatable(lua_State *L, int objindex);

void lua_gettable(lua_State *L, int idx);

void lua_getfenv(lua_State *L, int idx);

void lua_rawgeti(lua_State *L, int idx, int n);

void lua_settop(lua_State *L, int idx);

void lua_setfield(lua_State *L, int idx, const char *k);

int lua_setmetatable(lua_State *L, int objindex);

int lua_setfenv(lua_State *L, int idx);

int lua_type(lua_State *L, int idx);

void lua_call(lua_State *L, int nargs, int nresults);

int lua_lessthan(lua_State *L, int index1, int index2);

void lua_rawseti(lua_State *L, int idx, int n);

void lua_rawset(lua_State *L, int idx);

void lua_rawget(lua_State *L, int idx);

int lua_rawequal(lua_State *L, int index1, int index2);

void lua_replace(lua_State *L, int idx);

void lua_createtable(lua_State *L, int narr, int nrec);

int lua_next(lua_State *L, int idx);

int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc);

void *lua_newuserdata(lua_State *L, size_t size);

void lua_pushnil(lua_State *L);

int lua_pushthread(lua_State *L);

void lua_pushnumber(lua_State *L, lua_Number n);

void lua_pushlstring(lua_State *L, const char *s, size_t len);

void lua_pushinteger(lua_State *L, lua_Integer n);

void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);

void lua_pushvalue(lua_State *L, int idx);

void lua_pushboolean(lua_State *L, int b);

void lua_pushstring(lua_State *L, const char *s);

const char *lua_pushfstring(lua_State *L, const char *fmt, ...);

size_t lua_objlen(lua_State *L, int idx);

const char *lua_typename(lua_State *L, int t);

void lua_concat(lua_State *L, int n);

int lua_checkstack(lua_State *L, int size);

int lua_error(lua_State *L);

void lua_insert(lua_State *L, int idx);

lua_Number luaL_checknumber(lua_State *L, int narg);

const char *luaL_optlstring(lua_State *L, int numArg, const char *def, size_t *l);

const char *luaL_checklstring(lua_State *L, int numArg, size_t *l);

lua_Integer luaL_checkinteger(lua_State *L, int numArg);

int luaL_getmetafield(lua_State *L, int obj, const char *event);

void *luaL_checkudata(lua_State *L, int ud, const char *tname);

void luaL_checkany(lua_State *L, int narg);

void luaL_register(lua_State *L, const char *libname, const luaL_Reg *l);

int luaL_typerror(lua_State *L, int narg, const char *tname);

void luaL_buffinit(lua_State *L, luaL_Buffer *B);

lua_Integer luaL_optinteger(lua_State *L, int nArg, lua_Integer def);

char *luaL_prepbuffer(luaL_Buffer *B);

void luaL_pushresult(luaL_Buffer *B);

int luaL_loadbuffer(lua_State *L, const char *buff, size_t size, const char *name);

int luaL_loadfile(lua_State *L, const char *filename);

void luaL_addlstring(luaL_Buffer *B, const char *s, size_t l);

int luaL_error(lua_State *L, const char *fmt, ...);

void luaL_where(lua_State *L, int level);

void luaL_checkstack(lua_State *L, int space, const char *mes);

void luaL_checktype(lua_State *L, int narg, int t);

int luaL_argerror(lua_State *L, int numarg, const char *extramsg);

int luaL_newmetatable(lua_State *L, const char *tname);

void luaL_addvalue(luaL_Buffer *B);

#define lua_tostring(L, i)lua_tolstring(L,(i),NULL)
#define lua_isnone(L, n)(lua_type(L,(n))==(-1))
#define lua_pushliteral(L, s)lua_pushlstring(L,""s,(sizeof(s)/sizeof(char))-1)

#define luaL_addchar(B, c)((void)((B)->p<((B)->buffer+BUFSIZ)||luaL_prepbuffer(B)),(*(B)->p++=(char)(c)))
#define luaL_addsize(B, n)((B)->p+=(n))
#define luaL_checkint(L, n)((int)luaL_checkinteger(L,(n)))
#define luaL_argcheck(L, cond, numarg, extramsg)((void)((cond)||luaL_argerror(L,(numarg),(extramsg))))
#define luaL_checkstring(L, n)(luaL_checklstring(L,(n),NULL))

#define lua_pop(L, n)lua_settop(L,-(n)-1)
#define lua_newtable(L)lua_createtable(L,0,0)
#define lua_pushcfunction(L, f)lua_pushcclosure(L,(f),0)
#define lua_strlen(L, i)lua_objlen(L,(i))
#define lua_isfunction(L, n)(lua_type(L,(n))==6)
#define lua_istable(L, n)(lua_type(L,(n))==5)
#define lua_isnil(L, n)(lua_type(L,(n))==0)
#define lua_isboolean(L, n)(lua_type(L,(n))==1)

#define lua_isnoneornil(L, n)(lua_type(L,(n))<=0)

#define lua_setglobal(L, s)lua_setfield(L,(-10002),(s))

#define luaL_optstring(L, n, d)(luaL_optlstring(L,(n),(d),NULL))

#define luaL_optint(L, n, d)((int)luaL_optinteger(L,(n),(d)))
#define luaL_typename(L, i)lua_typename(L,lua_type(L,(i)))
#define luaL_getmetatable(L, n)(lua_getfield(L,(-10000),(n)))
#define luaL_opt(L, f, n, d)(lua_isnoneornil(L,(n))?(d):f(L,(n)))

#define luaL_getn(L, i)((int)lua_objlen(L,i))
#define luaL_setn(L, i, j)((void)0)


TValue *index2adr(lua_State *L, int idx);

#endif //MINILUA_H
