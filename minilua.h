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

/*
** masks for instruction properties. The format is:
** bits 0-1: op mode
** bits 2-3: C arg mode
** bits 4-5: B arg mode
** bit 6: instruction set register A
** bit 7: operator is a test
*/
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
#define lua_number2str(s, n)sprintf((s),"%.14g",(n))
#define lua_str2number(s, p)strtod((s),(p))
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

typedef struct luaL_Buffer {
    char *p;
    int lvl;
    lua_State *L;
    char buffer[BUFSIZ];
} luaL_Buffer;

#define IntPoint(p)((unsigned int)(lu_mem)(p))

// 在这个设计中，L_UMaxAlign 被用作一个占位符，它的大小被设计为以上三种数据类型中最大的那个。
// 这样可以确保在实际使用中，其他数据结构可以依据它的大小来做出正确的对齐。
// 这在处理底层内存布局时尤为重要，因为一些硬件平台要求特定类型的数据需要按照特定的地址对齐方式进行存储和访问。
typedef union {
    double u;
    void *s;
    long l;
} L_UMaxAlign;

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
    GCObject *next;     // 用于指向下一个垃圾回收对象
    lu_byte tt;         // 用于表示对象的类型标记（type tag）
    lu_byte marked;     // 用于表示对象的标记信息，通常用于在垃圾回收过程中标记对象的存活状态
} GCheader;

typedef union {
    GCObject *gc;   // 用于指向 Lua 中的垃圾回收对象
    void *p;        // 在 Lua 中，这种指针通常被用于表示通用的指针类型
    lua_Number n;   // 通常用于表示 Lua 中的浮点数
    int b;          // 用于表示布尔值
} Value;

typedef struct lua_TValue {
    Value value;    // 用于存储实际的值数据
    int tt;         // 它被用来标识值的类型（type tag）
} TValue;

#define ttype(o)((o)->tt)
#define ttisnil(o)(ttype(o)==LUA_TNIL)
#define ttisnumber(o)(ttype(o)==LUA_TNUMBER)
#define ttisstring(o)(ttype(o)==LUA_TSTRING)
#define ttistable(o)(ttype(o)==LUA_TTABLE)
#define ttisfunction(o)(ttype(o)==LUA_TFUNCTION)
#define ttisboolean(o)(ttype(o)==LUA_TBOOLEAN)
#define ttisuserdata(o)(ttype(o)==LUA_TUSERDATA)
#define ttisthread(o)(ttype(o)==LUA_TTHREAD)
#define ttislightuserdata(o)(ttype(o)==LUA_TLIGHTUSERDATA)

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
#define setnilvalue(obj)((obj)->tt=LUA_TNIL)
#define setnvalue(obj, x){TValue*i_o=(obj);i_o->value.n=(x);i_o->tt=3;}
#define setbvalue(obj, x){TValue*i_o=(obj);i_o->value.b=(x);i_o->tt=1;}
#define setsvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=4;checkliveness(G(L),i_o);}
#define setuvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=7;checkliveness(G(L),i_o);}
#define setthvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=8;checkliveness(G(L),i_o);}
#define setclvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=6;checkliveness(G(L),i_o);}
// 用于将新创建的表设置为Lua栈顶的值
#define sethvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=5;checkliveness(G(L),i_o);}
#define setptvalue(L, obj, x){TValue*i_o=(obj);i_o->value.gc=cast(GCObject*,(x));i_o->tt=(8+1);checkliveness(G(L),i_o);}
#define setobj(L, obj1, obj2){const TValue*o2=(obj2);TValue*o1=(obj1);o1->value=o2->value;o1->tt=o2->tt;checkliveness(G(L),o1);}
#define setttype(obj, tt)(ttype(obj)=(tt))
#define iscollectable(o)(ttype(o)>=4)

typedef TValue *StkId;

typedef union TString {
    L_UMaxAlign dummy;      // 这里定义了一个占位成员 dummy，可能是为了确保 TString 的大小满足特定的内存对齐要求
    struct {
        GCObject *next;     // 指向下一个字符串对象的指针
        lu_byte tt;         // 类型标记，用于表示该字符串对象的类型
        lu_byte marked;     // 标记字节，用于在垃圾回收过程中标记对象的状态
        lu_byte reserved;   // 保留的字节，可能用于未来扩展或对齐
        unsigned int hash;  // 字符串的哈希值，用于快速比较字符串是否相等
        size_t len;         // 字符串的长度
    } tsv;
} TString;

#define getstr(ts)cast(const char*,(ts)+1)
#define svalue(o)getstr(rawtsvalue(o))

typedef union Udata {
    L_UMaxAlign dummy;
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
    GCObject *next;                 // 指向下一个上值对象的指针
    lu_byte tt;                     // 类型标记，用于表示该上值对象的类型
    lu_byte marked;                 // 标记字节，用于在垃圾回收过程中标记对象的状态
    TValue *v;                      // 指向存储值的指针，即指向该上值对应的值对象
    union {
        TValue value;               // 直接存储值的情况
        struct {                    // 当上值处于开放状态时，
            struct UpVal *prev;     // 使用 prev 和 next 指针来构成链表结构
            struct UpVal *next;     // 以便将上值对象连接到开放上值的链表中
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
        Value value;            // 存储键的具体数值。
        int tt;                 // 类型标记，用于表示键的类型
        struct Node *next;      // 指向下一个节点的指针，用于在哈希冲突的情况下构成链表
    } nk;
    TValue tvk;                 // 作为备用方案，直接存储键的值
} TKey;

typedef struct Node {
    TValue i_val;   // 用于存储节点的值
    TKey i_key;     // 用于存储节点的键
} Node;

typedef struct Table {
    GCObject *next;             // 指向下一个表对象的指针
    lu_byte tt;                 // 类型标记，用于表示该表对象的类型
    lu_byte marked;             // 标记字节，用于在垃圾回收过程中标记对象的状态
    lu_byte flags;              // 标志字节，用于表示表的一些特殊标志或属性
    lu_byte lsizenode;          // 用于表示哈希部分的大小掩码
    struct Table *metatable;    // 指向该表的元表（metatable）
    TValue *array;              // 指向存储数组部分的指针
    Node *node;                 // 指向存储哈希部分节点的指针
    Node *lastfree;             // 指向最后一个空闲节点的指针，用于快速分配新节点
    GCObject *gclist;           // 指向下一个待回收对象的指针，用于构成待回收对象链表
    int sizearray;              // 表示数组部分的大小
} Table;


#define twoto(x)((size_t)1<<(x))
#define sizenode(t)(twoto((t)->lsizenode))

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

typedef struct MBuffer {
    char *buffer;       // 指向存储数据的缓冲区的指针
    size_t n;           // 表示当前缓冲区中已经存储的数据的大小
    size_t buffsize;    // 表示整个缓冲区的总大小，即缓冲区能够容纳的最大数据量
} MBuffer;

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

typedef struct {
    GCObject **hash;    // 用于存储指向字符串对象的指针，这里使用指针数组的方式来实现哈希表，以便快速查找和管理字符串对象
    lu_int32 nUse;      // 表示当前使用的字符串对象数量
    int size;           // 表示哈希表的大小，即能容纳的字符串对象的数量
} StringTable;

typedef struct CallInfo {
    StkId base;                     // 指向当前函数调用的栈底的指针
    StkId func;                     // 指向表示当前被调用函数的栈位置的指针
    StkId top;                      // 指向当前函数调用的栈顶的指针
    const Instruction *savedpc;     // 指向在执行函数调用时需要被恢复的程序计数器（即下一条待执行的指令）
    int nresults;                   // 期望接收的返回值数量
    int tailcalls;                  // 表示尾调用的次数
} CallInfo;

#define curr_func(L)(clvalue(L->ci->func))
#define ci_func(ci)(clvalue((ci)->func))
#define f_isLua(ci)(!ci_func(ci)->c.isC)
#define isLua(ci)(ttisfunction((ci)->func)&&f_isLua(ci))

typedef struct global_State {
    StringTable strt;                   // 用于存储字符串的哈希表结构，用于快速查找和管理字符串对象
    lua_Alloc frealloc;                 // 用于内存分配和重新分配的函数指针，可以根据实际需要进行内存管理
    void *userdata;                     // 一个通用的指针，用于存储用户自定义数据，可以在内存分配和重新分配函数中使用
    lu_byte currentwhite;               // 表示当前的垃圾回收状态
    lu_byte gcstate;                    // 表示垃圾回收的状态
    int sweepstrgc;                     // 用于记录字符串对象的垃圾回收状态
    GCObject *rootgc;                   // 是一个链表结构，用于管理所有的垃圾回收对象
    GCObject **sweepgc;                 // 指向当前需要进行垃圾回收的对象列表
    GCObject *gray;                     // 灰色对象，用于标记阶段的中间状态
    GCObject *grayagain;                // 再次标记阶段的灰色对象
    GCObject *weak;                     // 弱引用对象
    GCObject *tmudata;                  // 用于管理元方法的用户数据对象
    MBuffer buff;                       // 缓冲区，用于临时存储数据
    lu_mem GCthreshold;                 // 表示垃圾回收的阈值
    lu_mem totalbytes;                  // 总字节数
    lu_mem estimate;                    // 估计的字节数
    lu_mem gcdept;                      // 垃圾回收深度
    int gcpause;                        // 垃圾回收暂停
    int gcstepmul;                      // 垃圾回收步进倍数
    lua_CFunction panic;                // Lua 运行时的错误处理函数
    TValue l_registry;                  // 注册表，用于存储全局变量等信息
    struct lua_State *mainthread;       // 主线程
    UpVal uvhead;                       // 上值链表的头部
    struct Table *mt[(8 + 1)];          // 用于存储元表的数组
    TString *tmname[TM_N];              // 用于存储元方法名称的数组
} global_State;



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

void lua_getfield(lua_State *L, int idx, const char *k);

void lua_rawget(lua_State *L, int idx);

void lua_rawgeti(lua_State *L, int idx, int n);

void lua_createTable(lua_State *L, int nArr, int nRec);

void *lua_newuserdata(lua_State *L, size_t size);

int lua_getmetatable(lua_State *L, int objindex);

void lua_getfenv(lua_State *L, int idx);


// set functions (stack -> Lua)
void lua_settable(lua_State *L, int idx);

void lua_setfield(lua_State *L, int idx, const char *k);

void lua_rawset(lua_State *L, int idx);

void lua_rawseti(lua_State *L, int idx, int n);

int lua_setmetatable(lua_State *L, int objindex);

int lua_setfenv(lua_State *L, int idx);


// `load' and `call' functions (load and run Lua code)
void lua_call(lua_State *L, int nargs, int nresults);

int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc);

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
#define lua_setglobal(L, s) lua_setfield(L,LUA_GLOBALS_INDEX,(s))
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




int luaL_typerror(lua_State *L, int narg, const char *tname);

void luaL_buffinit(lua_State *L, luaL_Buffer *B);

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


#define luaL_addchar(B, c)((void)((B)->p<((B)->buffer+BUFSIZ)||luaL_prepbuffer(B)),(*(B)->p++=(char)(c)))
#define luaL_addsize(B, n)((B)->p+=(n))
#define luaL_checkint(L, n)((int)luaL_checkinteger(L,(n)))
#define luaL_argcheck(L, cond, numarg, extramsg)((void)((cond)||luaL_argerror(L,(numarg),(extramsg))))
#define luaL_checkstring(L, n)(luaL_checklstring(L,(n),NULL))


#define luaL_optstring(L, n, d)(luaL_optlstring(L,(n),(d),NULL))

#define luaL_optint(L, n, d)((int)luaL_optinteger(L,(n),(d)))
#define luaL_typename(L, i)lua_typename(L,lua_type(L,(i)))
#define luaL_getmetatable(L, n)(lua_getfield(L,(-10000),(n)))
#define luaL_opt(L, f, n, d)(lua_isnoneornil(L,(n))?(d):f(L,(n)))

#define luaL_getn(L, i)((int)lua_objlen(L,i))
#define luaL_setn(L, i, j)((void)0)

TValue *index2adr(lua_State *L, int idx);

#endif //MINILUA_H
