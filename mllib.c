#include "minilua.h"
#include <time.h>

#define BRET(b)lua_pushnumber(L,(lua_Number)(int)(b));return 1;

typedef unsigned int UB;

static UB barg(lua_State *L, int idx) {
    union {
        lua_Number n;
        U64 b;
    } bn;
    bn.n = lua_tonumber(L, idx) + 6755399441055744.0;
    if (bn.n == 0.0 && !lua_isnumber(L, idx))luaL_typerror(L, idx, "number");
    return (UB) bn.b;
}

static int tobit(lua_State *L) {
    BRET(barg(L, 1))
}

static int bnot(lua_State *L) {
    BRET(~barg(L, 1))
}

static int band(lua_State *L) {
    int i;
    UB b = barg(L, 1);
    for (i = lua_getTop(L); i > 1; i--)b &= barg(L, i);
    BRET(b)
}

static int bor(lua_State *L) {
    int i;
    UB b = barg(L, 1);
    for (i = lua_getTop(L); i > 1; i--)b |= barg(L, i);
    BRET(b)
}

static int bxor(lua_State *L) {
    int i;
    UB b = barg(L, 1);
    for (i = lua_getTop(L); i > 1; i--)b ^= barg(L, i);
    BRET(b)
}

static int lshift(lua_State *L) {
    UB b = barg(L, 1), n = barg(L, 2) & 31;
    BRET(b << n)
}

static int rshift(lua_State *L) {
    UB b = barg(L, 1), n = barg(L, 2) & 31;
    BRET(b >> n)
}

static int arshift(lua_State *L) {
    UB b = barg(L, 1), n = barg(L, 2) & 31;
    BRET((int) b >> n)
}

static int rol(lua_State *L) {
    UB b = barg(L, 1), n = barg(L, 2) & 31;
    BRET((b << n) | (b >> (32 - n)))
}

static int ror(lua_State *L) {
    UB b = barg(L, 1), n = barg(L, 2) & 31;
    BRET((b >> n) | (b << (32 - n)))
}

static int bswap(lua_State *L) {
    UB b = barg(L, 1);
    b = (b >> 24) | ((b >> 8) & 0xff00) | ((b & 0xff00) << 8) | (b << 24);
    BRET(b)
}

static int tohex(lua_State *L) {
    UB b = barg(L, 1);
    int n = lua_isnone(L, 2) ? 8 : (int) barg(L, 2);
    const char *hexdigits = "0123456789abcdef";
    char buf[8];
    int i;
    if (n < 0) {
        n = -n;
        hexdigits = "0123456789ABCDEF";
    }
    if (n > 8)n = 8;
    for (i = (int) n; --i >= 0;) {
        buf[i] = hexdigits[b & 15];
        b >>= 4;
    }
    lua_pushlstring(L, buf, (size_t) n);
    return 1;
}

const luaL_Reg bitLib[] = {
        {"tobit",   tobit},
        {"bnot",    bnot},
        {"band",    band},
        {"bor",     bor},
        {"bxor",    bxor},
        {"lshift",  lshift},
        {"rshift",  rshift},
        {"arshift", arshift},
        {"rol",     rol},
        {"ror",     ror},
        {"bswap",   bswap},
        {"tohex",   tohex},
        {NULL, NULL}
};

static void tag_error(lua_State *L, int narg, int tag) {
    luaL_typerror(L, narg, lua_typename(L, tag));
}

static lua_Number luaL_checknumber(lua_State *L, int narg) {
    lua_Number d = lua_tonumber(L, narg);
    if (d == 0 && !lua_isnumber(L, narg))
        tag_error(L, narg, 3);
    return d;
}

static void *l_alloc(void *ud, void *ptr, size_t oldSize, size_t newSize) {
    UNUSED(ud);
    UNUSED(oldSize);
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, newSize);
}

static int panic(lua_State *L) {
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
            lua_tostring(L, -1));
    return 0;
}

lua_State *luaL_newState() {
    lua_State *L = lua_newState(l_alloc, NULL);
    if (L) {
        lua_atPanic(L, &panic);
    }
    return L;
}

static int libsize(const luaL_Reg *l) {
    int size = 0;
    for (; l->name; l++)size++;
    return size;
}

static const char *luaL_findtable(lua_State *L, int idx, const char *fname, int szhint) {
    const char *e;
    lua_pushValue(L, idx);
    do {
        e = strchr(fname, '.');
        if (e == NULL)e = fname + strlen(fname);
        lua_pushlstring(L, fname, e - fname);
        lua_rawget(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_createTable(L, 0, (*e == '.' ? 1 : szhint));
            lua_pushlstring(L, fname, e - fname);
            lua_pushValue(L, -2);
            lua_settable(L, -4);
        } else if (!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return fname;
        }
        lua_remove(L, -2);
        fname = e + 1;
    } while (*e == '.');
    return NULL;
}

static void luaI_openlib(lua_State *L, const char *libname, const luaL_Reg *l, int nup) {
    if (libname) {
        int size = libsize(l);
        luaL_findtable(L, LUA_REGISTRY_INDEX, "_LOADED", 1);
        lua_getfield(L, -1, libname);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            if (luaL_findtable(L, (-10002), libname, size) != NULL)
                luaL_error(L, "name conflict for module "LUA_QL("%s"), libname);
            lua_pushValue(L, -1);
            lua_setfield(L, -3, libname);
        }
        lua_remove(L, -2);
        lua_insert(L, -(nup + 1));
    }
    for (; l->name; l++) {
        int i;
        for (i = 0; i < nup; i++)
            lua_pushValue(L, -nup);
        lua_pushcclosure(L, l->func, nup);
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_pop(L, nup);
}

void luaL_register(lua_State *L, const char *libname, const luaL_Reg *l) {
    luaI_openlib(L, libname, l, 0);
}

static void luaL_checkany(lua_State *L, int narg) {
    if (lua_type(L, narg) == (-1))
        luaL_argerror(L, narg, "value expected");
}

static lua_Integer luaL_checkinteger(lua_State *L, int narg) {
    lua_Integer d = lua_tointeger(L, narg);
    if (d == 0 && !lua_isnumber(L, narg))
        tag_error(L, narg, 3);
    return d;
}

static lua_Integer luaL_optinteger(lua_State *L, int narg, lua_Integer def) {
    return luaL_opt(L, luaL_checkinteger, narg, def);
}

static int luaL_getmetafield(lua_State *L, int obj, const char *event) {
    if (!lua_getmetatable(L, obj))
        return 0;
    lua_pushstring(L, event);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return 0;
    } else {
        lua_remove(L, -2);
        return 1;
    }
}

static void *luaL_checkudata(lua_State *L, int ud, const char *tname) {
    void *p = lua_touserdata(L, ud);
    if (p != NULL) {
        if (lua_getmetatable(L, ud)) {
            lua_getfield(L, LUA_REGISTRY_INDEX, tname);
            if (lua_rawequal(L, -1, -2)) {
                lua_pop(L, 2);
                return p;
            }
        }
    }
    luaL_typerror(L, ud, tname);
    return NULL;
}

static const char *luaL_checklstring(lua_State *L, int narg, size_t *len) {
    const char *s = lua_tolstring(L, narg, len);
    if (!s)tag_error(L, narg, 4);
    return s;
}

static const char *luaL_optlstring(lua_State *L, int narg, const char *def, size_t *len) {
    if (lua_isnoneornil(L, narg)) {
        if (len)
            *len = (def ? strlen(def) : 0);
        return def;
    } else return luaL_checklstring(L, narg, len);
}

#define uchar(c)((unsigned char)(c))

static ptrdiff_t posrelat(ptrdiff_t pos, size_t len) {
    if (pos < 0)pos += (ptrdiff_t) len + 1;
    return (pos >= 0) ? pos : 0;
}

static int str_sub(lua_State *L) {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    ptrdiff_t start = posrelat(luaL_checkinteger(L, 2), l);
    ptrdiff_t end = posrelat(luaL_optinteger(L, 3, -1), l);
    if (start < 1)start = 1;
    if (end > (ptrdiff_t) l)end = (ptrdiff_t) l;
    if (start <= end)
        lua_pushlstring(L, s + start - 1, end - start + 1);
    else
        lua_pushliteral(L, "");
    return 1;
}

static int str_lower(lua_State *L) {
    size_t l;
    size_t i;
    luaL_Buffer b;
    const char *s = luaL_checklstring(L, 1, &l);
    luaL_buffinit(L, &b);
    for (i = 0; i < l; i++)
        luaL_addchar(&b, tolower(uchar(s[i])));
    luaL_pushresult(&b);
    return 1;
}

static int str_upper(lua_State *L) {
    size_t l;
    size_t i;
    luaL_Buffer b;
    const char *s = luaL_checklstring(L, 1, &l);
    luaL_buffinit(L, &b);
    for (i = 0; i < l; i++)
        luaL_addchar(&b, toupper(uchar(s[i])));
    luaL_pushresult(&b);
    return 1;
}

static int str_rep(lua_State *L) {
    size_t l;
    luaL_Buffer b;
    const char *s = luaL_checklstring(L, 1, &l);
    int n = luaL_checkint(L, 2);
    luaL_buffinit(L, &b);
    while (n-- > 0)
        luaL_addlstring(&b, s, l);
    luaL_pushresult(&b);
    return 1;
}

static int str_byte(lua_State *L) {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    ptrdiff_t posi = posrelat(luaL_optinteger(L, 2, 1), l);
    ptrdiff_t pose = posrelat(luaL_optinteger(L, 3, posi), l);
    int n, i;
    if (posi <= 0)posi = 1;
    if ((size_t) pose > l)pose = l;
    if (posi > pose)return 0;
    n = (int) (pose - posi + 1);
    if (posi + n <= pose)
        luaL_error(L, "string slice too long");
    luaL_checkstack(L, n, "string slice too long");
    for (i = 0; i < n; i++)
        lua_pushinteger(L, uchar(s[posi + i - 1]));
    return n;
}

static int str_char(lua_State *L) {
    int n = lua_getTop(L);
    int i;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (i = 1; i <= n; i++) {
        int c = luaL_checkint(L, i);
        luaL_argcheck(L, uchar(c) == c, i, "invalid value");
        luaL_addchar(&b, uchar(c));
    }
    luaL_pushresult(&b);
    return 1;
}

typedef struct MatchState {
    const char *src_init;
    const char *src_end;
    lua_State *L;
    int level;
    struct {
        const char *init;
        ptrdiff_t len;
    } capture[32];
} MatchState;

static int check_capture(MatchState *ms, int l) {
    l -= '1';
    if (l < 0 || l >= ms->level || ms->capture[l].len == (-1))
        return luaL_error(ms->L, "invalid capture index");
    return l;
}

static int capture_to_close(MatchState *ms) {
    int level = ms->level;
    for (level--; level >= 0; level--)
        if (ms->capture[level].len == (-1))return level;
    return luaL_error(ms->L, "invalid pattern capture");
}

static const char *classend(MatchState *ms, const char *p) {
    switch (*p++) {
        case '%': {
            if (*p == '\0')
                luaL_error(ms->L, "malformed pattern (ends with "LUA_QL("%%")")");
            return p + 1;
        }
        case '[': {
            if (*p == '^')p++;
            do {
                if (*p == '\0')
                    luaL_error(ms->L, "malformed pattern (missing "LUA_QL("]")")");
                if (*(p++) == '%' && *p != '\0')
                    p++;
            } while (*p != ']');
            return p + 1;
        }
        default: {
            return p;
        }
    }
}

static int match_class(int c, int cl) {
    int res;
    switch (tolower(cl)) {
        case 'a':
            res = isalpha(c);
            break;
        case 'c':
            res = iscntrl(c);
            break;
        case 'd':
            res = isdigit(c);
            break;
        case 'l':
            res = islower(c);
            break;
        case 'p':
            res = ispunct(c);
            break;
        case 's':
            res = isspace(c);
            break;
        case 'u':
            res = isupper(c);
            break;
        case 'w':
            res = isalnum(c);
            break;
        case 'x':
            res = isxdigit(c);
            break;
        case 'z':
            res = (c == 0);
            break;
        default:
            return (cl == c);
    }
    return (islower(cl) ? res : !res);
}

static int matchbracketclass(int c, const char *p, const char *ec) {
    int sig = 1;
    if (*(p + 1) == '^') {
        sig = 0;
        p++;
    }
    while (++p < ec) {
        if (*p == '%') {
            p++;
            if (match_class(c, uchar(*p)))
                return sig;
        } else if ((*(p + 1) == '-') && (p + 2 < ec)) {
            p += 2;
            if (uchar(*(p - 2)) <= c && c <= uchar(*p))
                return sig;
        } else if (uchar(*p) == c)return sig;
    }
    return !sig;
}

static int singlematch(int c, const char *p, const char *ep) {
    switch (*p) {
        case '.':
            return 1;
        case '%':
            return match_class(c, uchar(*(p + 1)));
        case '[':
            return matchbracketclass(c, p, ep - 1);
        default:
            return (uchar(*p) == c);
    }
}

static const char *match(MatchState *ms, const char *s, const char *p);

static const char *matchbalance(MatchState *ms, const char *s,
                                const char *p) {
    if (*p == 0 || *(p + 1) == 0)
        luaL_error(ms->L, "unbalanced pattern");
    if (*s != *p)return NULL;
    else {
        int b = *p;
        int e = *(p + 1);
        int cont = 1;
        while (++s < ms->src_end) {
            if (*s == e) {
                if (--cont == 0)return s + 1;
            } else if (*s == b)cont++;
        }
    }
    return NULL;
}

static const char *max_expand(MatchState *ms, const char *s,const char *p, const char *ep) {
    ptrdiff_t i = 0;
    while ((s + i) < ms->src_end && singlematch(uchar(*(s + i)), p, ep))
        i++;
    while (i >= 0) {
        const char *res = match(ms, (s + i), ep + 1);
        if (res)return res;
        i--;
    }
    return NULL;
}

static const char *min_expand(MatchState *ms, const char *s,const char *p, const char *ep) {
    for (;;) {
        const char *res = match(ms, s, ep + 1);
        if (res != NULL)
            return res;
        else if (s < ms->src_end && singlematch(uchar(*s), p, ep))
            s++;
        else return NULL;
    }
}

static const char *start_capture(MatchState *ms, const char *s,const char *p, int what) {
    const char *res;
    int level = ms->level;
    if (level >= 32)luaL_error(ms->L, "too many captures");
    ms->capture[level].init = s;
    ms->capture[level].len = what;
    ms->level = level + 1;
    if ((res = match(ms, s, p)) == NULL)
        ms->level--;
    return res;
}

static const char *end_capture(MatchState *ms, const char *s,const char *p) {
    int l = capture_to_close(ms);
    const char *res;
    ms->capture[l].len = s - ms->capture[l].init;
    if ((res = match(ms, s, p)) == NULL)
        ms->capture[l].len = (-1);
    return res;
}

static const char *match_capture(MatchState *ms, const char *s, int l) {
    size_t len;
    l = check_capture(ms, l);
    len = ms->capture[l].len;
    if ((size_t) (ms->src_end - s) >= len &&
        memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    else return NULL;
}


static const char *match(MatchState *ms, const char *s, const char *p) {
    init:
    switch (*p) {
        case '(': {
            if (*(p + 1) == ')')
                return start_capture(ms, s, p + 2, (-2));
            else
                return start_capture(ms, s, p + 1, (-1));
        }
        case ')': {
            return end_capture(ms, s, p + 1);
        }
        case '%': {
            switch (*(p + 1)) {
                case 'b': {
                    s = matchbalance(ms, s, p + 2);
                    if (s == NULL)return NULL;
                    p += 4;
                    goto init;
                }
                case 'f': {
                    const char *ep;
                    char previous;
                    p += 2;
                    if (*p != '[')
                        luaL_error(ms->L, "missing "LUA_QL("[")" after "
                                          LUA_QL("%%f")" in pattern");
                    ep = classend(ms, p);
                    previous = (s == ms->src_init) ? '\0' : *(s - 1);
                    if (matchbracketclass(uchar(previous), p, ep - 1) ||
                        !matchbracketclass(uchar(*s), p, ep - 1))
                        return NULL;
                    p = ep;
                    goto init;
                }
                default: {
                    if (isdigit(uchar(*(p + 1)))) {
                        s = match_capture(ms, s, uchar(*(p + 1)));
                        if (s == NULL)return NULL;
                        p += 2;
                        goto init;
                    }
                    goto dflt;
                }
            }
        }
        case '\0': {
            return s;
        }
        case '$': {
            if (*(p + 1) == '\0')
                return (s == ms->src_end) ? s : NULL;
            else goto dflt;
        }
        default:
        dflt:
        {
            const char *ep = classend(ms, p);
            int m = s < ms->src_end && singlematch(uchar(*s), p, ep);
            switch (*ep) {
                case '?': {
                    const char *res;
                    if (m && ((res = match(ms, s + 1, ep + 1)) != NULL))
                        return res;
                    p = ep + 1;
                    goto init;
                }
                case '*': {
                    return max_expand(ms, s, p, ep);
                }
                case '+': {
                    return (m ? max_expand(ms, s + 1, p, ep) : NULL);
                }
                case '-': {
                    return min_expand(ms, s, p, ep);
                }
                default: {
                    if (!m)return NULL;
                    s++;
                    p = ep;
                    goto init;
                }
            }
        }
    }
}

static const char *lmemfind(const char *s1, size_t l1,
                            const char *s2, size_t l2) {
    if (l2 == 0)return s1;
    else if (l2 > l1)return NULL;
    else {
        const char *init;
        l2--;
        l1 = l1 - l2;
        while (l1 > 0 && (init = (const char *) memchr(s1, *s2, l1)) != NULL) {
            init++;
            if (memcmp(init, s2 + 1, l2) == 0)
                return init - 1;
            else {
                l1 -= init - s1;
                s1 = init;
            }
        }
        return NULL;
    }
}

static void push_onecapture(MatchState *ms, int i, const char *s,
                            const char *e) {
    if (i >= ms->level) {
        if (i == 0)
            lua_pushlstring(ms->L, s, e - s);
        else
            luaL_error(ms->L, "invalid capture index");
    } else {
        ptrdiff_t l = ms->capture[i].len;
        if (l == (-1))luaL_error(ms->L, "unfinished capture");
        if (l == (-2))
            lua_pushinteger(ms->L, ms->capture[i].init - ms->src_init + 1);
        else
            lua_pushlstring(ms->L, ms->capture[i].init, l);
    }
}

static int push_captures(MatchState *ms, const char *s, const char *e) {
    int i;
    int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
    luaL_checkstack(ms->L, nlevels, "too many captures");
    for (i = 0; i < nlevels; i++)
        push_onecapture(ms, i, s, e);
    return nlevels;
}

static int str_find_aux(lua_State *L, int find) {
    size_t l1, l2;
    const char *s = luaL_checklstring(L, 1, &l1);
    const char *p = luaL_checklstring(L, 2, &l2);
    ptrdiff_t init = posrelat(luaL_optinteger(L, 3, 1), l1) - 1;
    if (init < 0)init = 0;
    else if ((size_t) (init) > l1)init = (ptrdiff_t) l1;
    if (find && (lua_toboolean(L, 4) ||
                 strpbrk(p, "^$*+?.([%-") == NULL)) {
        const char *s2 = lmemfind(s + init, l1 - init, p, l2);
        if (s2) {
            lua_pushinteger(L, s2 - s + 1);
            lua_pushinteger(L, s2 - s + l2);
            return 2;
        }
    } else {
        MatchState ms;
        int anchor = (*p == '^') ? (p++, 1) : 0;
        const char *s1 = s + init;
        ms.L = L;
        ms.src_init = s;
        ms.src_end = s + l1;
        do {
            const char *res;
            ms.level = 0;
            if ((res = match(&ms, s1, p)) != NULL) {
                if (find) {
                    lua_pushinteger(L, s1 - s + 1);
                    lua_pushinteger(L, res - s);
                    return push_captures(&ms, NULL, 0) + 2;
                } else
                    return push_captures(&ms, s1, res);
            }
        } while (s1++ < ms.src_end && !anchor);
    }
    lua_pushnil(L);
    return 1;
}

static int str_find(lua_State *L) {
    return str_find_aux(L, 1);
}

static int str_match(lua_State *L) {
    return str_find_aux(L, 0);
}

static int gmatch_aux(lua_State *L) {
    MatchState ms;
    size_t ls;
    const char *s = lua_tolstring(L, lua_upvalueindex(1), &ls);
    const char *p = lua_tostring(L, lua_upvalueindex(2));
    const char *src;
    ms.L = L;
    ms.src_init = s;
    ms.src_end = s + ls;
    for (src = s + (size_t) lua_tointeger(L, lua_upvalueindex(3));
         src <= ms.src_end;
         src++) {
        const char *e;
        ms.level = 0;
        if ((e = match(&ms, src, p)) != NULL) {
            lua_Integer newstart = e - s;
            if (e == src)newstart++;
            lua_pushinteger(L, newstart);
            lua_replace(L, lua_upvalueindex(3));
            return push_captures(&ms, src, e);
        }
    }
    return 0;
}

static int gmatch(lua_State *L) {
    luaL_checkstring(L, 1);
    luaL_checkstring(L, 2);
    lua_setTop(L, 2);
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, gmatch_aux, 3);
    return 1;
}

static void add_s(MatchState *ms, luaL_Buffer *b, const char *s, const char *e) {
    size_t l, i;
    const char *news = lua_tolstring(ms->L, 3, &l);
    for (i = 0; i < l; i++) {
        if (news[i] != '%')
            luaL_addchar(b, news[i]);
        else {
            i++;
            if (!isdigit(uchar(news[i])))
                luaL_addchar(b, news[i]);
            else if (news[i] == '0')
                luaL_addlstring(b, s, e - s);
            else {
                push_onecapture(ms, news[i] - '1', s, e);
                luaL_addvalue(b);
            }
        }
    }
}

static void add_value(MatchState *ms, luaL_Buffer *b, const char *s,
                      const char *e) {
    lua_State *L = ms->L;
    switch (lua_type(L, 3)) {
        case 3:
        case 4: {
            add_s(ms, b, s, e);
            return;
        }
        case 6: {
            int n;
            lua_pushValue(L, 3);
            n = push_captures(ms, s, e);
            lua_call(L, n, 1);
            break;
        }
        case 5: {
            push_onecapture(ms, 0, s, e);
            lua_gettable(L, 3);
            break;
        }
    }
    if (!lua_toboolean(L, -1)) {
        lua_pop(L, 1);
        lua_pushlstring(L, s, e - s);
    } else if (!lua_isstring(L, -1))
        luaL_error(L, "invalid replacement value (a %s)", luaL_typename(L, -1));
    luaL_addvalue(b);
}

static int str_gsub(lua_State *L) {
    size_t srcl;
    const char *src = luaL_checklstring(L, 1, &srcl);
    const char *p = luaL_checkstring(L, 2);
    int tr = lua_type(L, 3);
    int max_s = luaL_optint(L, 4, srcl + 1);
    int anchor = (*p == '^') ? (p++, 1) : 0;
    int n = 0;
    MatchState ms;
    luaL_Buffer b;
    luaL_argcheck(L, tr == 3 || tr == 4 ||
                     tr == 6 || tr == 5, 3,
                  "string/function/table expected");
    luaL_buffinit(L, &b);
    ms.L = L;
    ms.src_init = src;
    ms.src_end = src + srcl;
    while (n < max_s) {
        const char *e;
        ms.level = 0;
        e = match(&ms, src, p);
        if (e) {
            n++;
            add_value(&ms, &b, src, e);
        }
        if (e && e > src)
            src = e;
        else if (src < ms.src_end)
            luaL_addchar(&b, *src++);
        else break;
        if (anchor)break;
    }
    luaL_addlstring(&b, src, ms.src_end - src);
    luaL_pushresult(&b);
    lua_pushinteger(L, n);
    return 2;
}

static void addquoted(lua_State *L, luaL_Buffer *b, int arg) {
    size_t l;
    const char *s = luaL_checklstring(L, arg, &l);
    luaL_addchar(b, '"');
    while (l--) {
        switch (*s) {
            case '"':
            case '\\':
            case '\n': {
                luaL_addchar(b, '\\');
                luaL_addchar(b, *s);
                break;
            }
            case '\r': {
                luaL_addlstring(b, "\\r", 2);
                break;
            }
            case '\0': {
                luaL_addlstring(b, "\\000", 4);
                break;
            }
            default: {
                luaL_addchar(b, *s);
                break;
            }
        }
        s++;
    }
    luaL_addchar(b, '"');
}

static const char *scanformat(lua_State *L, const char *strfrmt, char *form) {
    const char *p = strfrmt;
    while (*p != '\0' && strchr("-+ #0", *p) != NULL)p++;
    if ((size_t) (p - strfrmt) >= sizeof("-+ #0"))
        luaL_error(L, "invalid format (repeated flags)");
    if (isdigit(uchar(*p)))p++;
    if (isdigit(uchar(*p)))p++;
    if (*p == '.') {
        p++;
        if (isdigit(uchar(*p)))p++;
        if (isdigit(uchar(*p)))p++;
    }
    if (isdigit(uchar(*p)))
        luaL_error(L, "invalid format (width or precision too long)");
    *(form++) = '%';
    strncpy(form, strfrmt, p - strfrmt + 1);
    form += p - strfrmt + 1;
    *form = '\0';
    return p;
}

static void addintlen(char *form) {
    size_t l = strlen(form);
    char spec = form[l - 1];
    strcpy(form + l - 1, "l");
    form[l + sizeof("l") - 2] = spec;
    form[l + sizeof("l") - 1] = '\0';
}

static int str_format(lua_State *L) {
    int top = lua_getTop(L);
    int arg = 1;
    size_t sfl;
    const char *strfrmt = luaL_checklstring(L, arg, &sfl);
    const char *strfrmt_end = strfrmt + sfl;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    while (strfrmt < strfrmt_end) {
        if (*strfrmt != '%')
            luaL_addchar(&b, *strfrmt++);
        else if (*++strfrmt == '%')
            luaL_addchar(&b, *strfrmt++);
        else {
            char form[(sizeof("-+ #0") + sizeof("l") + 10)];
            char buff[512];
            if (++arg > top)
                luaL_argerror(L, arg, "no value");
            strfrmt = scanformat(L, strfrmt, form);
            switch (*strfrmt++) {
                case 'c': {
                    sprintf(buff, form, (int) luaL_checknumber(L, arg));
                    break;
                }
                case 'd':
                case 'i': {
                    addintlen(form);
                    sprintf(buff, form, (long) luaL_checknumber(L, arg));
                    break;
                }
                case 'o':
                case 'u':
                case 'x':
                case 'X': {
                    addintlen(form);
                    sprintf(buff, form, (unsigned long) luaL_checknumber(L, arg));
                    break;
                }
                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G': {
                    sprintf(buff, form, (double) luaL_checknumber(L, arg));
                    break;
                }
                case 'q': {
                    addquoted(L, &b, arg);
                    continue;
                }
                case 's': {
                    size_t l;
                    const char *s = luaL_checklstring(L, arg, &l);
                    if (!strchr(form, '.') && l >= 100) {
                        lua_pushValue(L, arg);
                        luaL_addvalue(&b);
                        continue;
                    } else {
                        sprintf(buff, form, s);
                        break;
                    }
                }
                default: {
                    return luaL_error(L, "invalid option "LUA_QL("%%%c")" to "
                                         LUA_QL("format"), *(strfrmt - 1));
                }
            }
            luaL_addlstring(&b, buff, strlen(buff));
        }
    }
    luaL_pushresult(&b);
    return 1;
}



const luaL_Reg strlib[] = {
        {"byte",   str_byte},
        {"char",   str_char},
        {"find",   str_find},
        {"format", str_format},
        {"gmatch", gmatch},
        {"gsub",   str_gsub},
        {"lower",  str_lower},
        {"match",  str_match},
        {"rep",    str_rep},
        {"sub",    str_sub},
        {"upper",  str_upper},
        {NULL, NULL}
};

#define tofilep(L)((FILE**)luaL_checkudata(L,1,"FILE*"))

static int io_type(lua_State *L) {
    void *ud;
    luaL_checkany(L, 1);
    ud = lua_touserdata(L, 1);
    lua_getfield(L, (-10000), "FILE*");
    if (ud == NULL || !lua_getmetatable(L, 1) || !lua_rawequal(L, -2, -1))
        lua_pushnil(L);
    else if (*((FILE **) ud) == NULL)
        lua_pushliteral(L, "closed file");
    else
        lua_pushliteral(L, "file");
    return 1;
}

static FILE *tofile(lua_State *L) {
    FILE **f = tofilep(L);
    if (*f == NULL)
        luaL_error(L, "attempt to use a closed file");
    return *f;
}

static int aux_close(lua_State *L) {
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "__close");
    return (lua_tocfunction(L, -1))(L);
}

static int io_close(lua_State *L) {
    if (lua_isnone(L, 1))
        lua_rawgeti(L, (-10001), 2);
    tofile(L);
    return aux_close(L);
}

static int pushresult(lua_State *L, int i, const char *filename) {
    int en = errno;
    if (i) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        if (filename)
            lua_pushfstring(L, "%s: %s", filename, strerror(en));
        else
            lua_pushfstring(L, "%s", strerror(en));
        lua_pushinteger(L, en);
        return 3;
    }
}

static const char *const fnames[] = {"input", "output"};

static FILE *getiofile(lua_State *L, int findex) {
    FILE *f;
    lua_rawgeti(L, (-10001), findex);
    f = *(FILE **) lua_touserdata(L, -1);
    if (f == NULL)
        luaL_error(L, "standard %s file is closed", fnames[findex - 1]);
    return f;
}

static int io_flush(lua_State *L) {
    return pushresult(L, fflush(getiofile(L, 2)) == 0, NULL);
}

static FILE **newfile(lua_State *L) {
    FILE **pf = (FILE **) lua_newuserdata(L, sizeof(FILE *));
    *pf = NULL;
    luaL_getmetatable(L, "FILE*");
    lua_setmetatable(L, -2);
    return pf;
}

static void fileerror(lua_State *L, int arg, const char *filename) {
    lua_pushfstring(L, "%s: %s", filename, strerror(errno));
    luaL_argerror(L, arg, lua_tostring(L, -1));
}

static int g_iofile(lua_State *L, int f, const char *mode) {
    if (!lua_isnoneornil(L, 1)) {
        const char *filename = lua_tostring(L, 1);
        if (filename) {
            FILE **pf = newfile(L);
            *pf = fopen(filename, mode);
            if (*pf == NULL)
                fileerror(L, 1, filename);
        } else {
            tofile(L);
            lua_pushValue(L, 1);
        }
        lua_rawseti(L, (-10001), f);
    }
    lua_rawgeti(L, (-10001), f);
    return 1;
}

static int io_input(lua_State *L) {
    return g_iofile(L, 1, "r");
}

static int read_line(lua_State *L, FILE *f) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        size_t l;
        char *p = luaL_prepbuffer(&b);
        if (fgets(p, BUFSIZ, f) == NULL) {
            luaL_pushresult(&b);
            return (lua_objlen(L, -1) > 0);
        }
        l = strlen(p);
        if (l == 0 || p[l - 1] != '\n')
            luaL_addsize(&b, l);
        else {
            luaL_addsize(&b, l - 1);
            luaL_pushresult(&b);
            return 1;
        }
    }
}

static int io_readline(lua_State *L) {
    FILE *f = *(FILE **) lua_touserdata(L, lua_upvalueindex(1));
    int sucess;
    if (f == NULL)
        luaL_error(L, "file is already closed");
    sucess = read_line(L, f);
    if (ferror(f))
        return luaL_error(L, "%s", strerror(errno));
    if (sucess)return 1;
    else {
        if (lua_toboolean(L, lua_upvalueindex(2))) {
            lua_setTop(L, 0);
            lua_pushValue(L, lua_upvalueindex(1));
            aux_close(L);
        }
        return 0;
    }
}

static void aux_lines(lua_State *L, int idx, int toclose) {
    lua_pushValue(L, idx);
    lua_pushboolean(L, toclose);
    lua_pushcclosure(L, io_readline, 2);
}

static int f_lines(lua_State *L) {
    tofile(L);
    aux_lines(L, 1, 0);
    return 1;
}

static int io_lines(lua_State *L) {
    if (lua_isnoneornil(L, 1)) {
        lua_rawgeti(L, (-10001), 1);
        return f_lines(L);
    } else {
        const char *filename = luaL_checkstring(L, 1);
        FILE **pf = newfile(L);
        *pf = fopen(filename, "r");
        if (*pf == NULL)
            fileerror(L, 1, filename);
        aux_lines(L, lua_getTop(L), 1);
        return 1;
    }
}

static int io_open(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    const char *mode = luaL_optstring(L, 2, "r");
    FILE **pf = newfile(L);
    *pf = fopen(filename, mode);
    return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}

static int io_output(lua_State *L) {
    return g_iofile(L, 2, "w");
}

static int test_eof(lua_State *L, FILE *f) {
    int c = getc(f);
    ungetc(c, f);
    lua_pushlstring(L, NULL, 0);
    return (c != EOF);
}

static int read_chars(lua_State *L, FILE *f, size_t n) {
    size_t rlen;
    size_t nr;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    rlen = BUFSIZ;
    do {
        char *p = luaL_prepbuffer(&b);
        if (rlen > n)rlen = n;
        nr = fread(p, sizeof(char), rlen, f);
        luaL_addsize(&b, nr);
        n -= nr;
    } while (n > 0 && nr == rlen);
    luaL_pushresult(&b);
    return (n == 0 || lua_objlen(L, -1) > 0);
}

static int read_number(lua_State *L, FILE *f) {
    lua_Number d;
    if (fscanf(f, "%lf", &d) == 1) {
        lua_pushnumber(L, d);
        return 1;
    } else {
        lua_pushnil(L);
        return 0;
    }
}

static int g_read(lua_State *L, FILE *f, int first) {
    int nargs = lua_getTop(L) - 1;
    int success;
    int n;
    clearerr(f);
    if (nargs == 0) {
        success = read_line(L, f);
        n = first + 1;
    } else {
        luaL_checkstack(L, nargs + 20, "too many arguments");
        success = 1;
        for (n = first; nargs-- && success; n++) {
            if (lua_type(L, n) == 3) {
                size_t l = (size_t) lua_tointeger(L, n);
                success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
            } else {
                const char *p = lua_tostring(L, n);
                luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
                switch (p[1]) {
                    case 'n':
                        success = read_number(L, f);
                        break;
                    case 'l':
                        success = read_line(L, f);
                        break;
                    case 'a':
                        read_chars(L, f, ~((size_t) 0));
                        success = 1;
                        break;
                    default:
                        return luaL_argerror(L, n, "invalid format");
                }
            }
        }
    }
    if (ferror(f))
        return pushresult(L, 0, NULL);
    if (!success) {
        lua_pop(L, 1);
        lua_pushnil(L);
    }
    return n - first;
}

static int io_read(lua_State *L) {
    return g_read(L, getiofile(L, 1), 1);
}

static int g_write(lua_State *L, FILE *f, int arg) {
    int nargs = lua_getTop(L) - 1;
    int status = 1;
    for (; nargs--; arg++) {
        if (lua_type(L, arg) == 3) {
            status = status &&
                     fprintf(f, "%.14g", lua_tonumber(L, arg)) > 0;
        } else {
            size_t l;
            const char *s = luaL_checklstring(L, arg, &l);
            status = status && (fwrite(s, sizeof(char), l, f) == l);
        }
    }
    return pushresult(L, status, NULL);
}

static int io_write(lua_State *L) {
    return g_write(L, getiofile(L, 2), 1);
}

const luaL_Reg iolib[] = {
        {"close",  io_close},
        {"flush",  io_flush},
        {"input",  io_input},
        {"lines",  io_lines},
        {"open",   io_open},
        {"output", io_output},
        {"read",   io_read},
        {"type",   io_type},
        {"write",  io_write},
        {NULL, NULL}
};

static int f_write(lua_State *L) {
    return g_write(L, tofile(L), 2);
}

static int f_flush(lua_State *L) {
    return pushresult(L, fflush(tofile(L)) == 0, NULL);
}

static int io_gc(lua_State *L) {
    FILE *f = *tofilep(L);
    if (f != NULL)
        aux_close(L);
    return 0;
}

static int f_read(lua_State *L) {
    return g_read(L, tofile(L), 2);
}

const luaL_Reg flib[] = {
        {"close", io_close},
        {"flush", f_flush},
        {"lines", f_lines},
        {"read",  f_read},
        {"write", f_write},
        {"__gc",  io_gc},
        {NULL, NULL}
};

static int os_pushresult(lua_State *L, int i, const char *filename) {
    int en = errno;
    if (i) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "%s: %s", filename, strerror(en));
        lua_pushinteger(L, en);
        return 3;
    }
}

static int os_remove(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    return os_pushresult(L, remove(filename) == 0, filename);
}

static int os_exit(lua_State *L) {
    exit(luaL_optint(L, 1, EXIT_SUCCESS));
}

static int os_clock(lua_State *L) {
    lua_pushnumber(L, ((lua_Number) clock()) / (lua_Number) CLOCKS_PER_SEC);
    return 1;
}

const luaL_Reg syslib[] = {
        {"exit",   os_exit},
        {"remove", os_remove},
        {"clock",  os_clock},
        {NULL, NULL}
};

static int luaB_assert(lua_State *L) {
    luaL_checkany(L, 1);
    if (!lua_toboolean(L, 1))
        return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
    return lua_getTop(L);
}

static int luaB_error(lua_State *L) {
    int level = luaL_optint(L, 2, 1);
    lua_setTop(L, 1);
    if (lua_isstring(L, 1) && level > 0) {
        luaL_where(L, level);
        lua_pushValue(L, 1);
        lua_concat(L, 2);
    }
    return lua_error(L);
}

static int load_aux(lua_State *L, int status) {
    if (status == 0)
        return 1;
    else {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
}

static int luaB_loadfile(lua_State *L) {
    const char *fname = luaL_optstring(L, 1, NULL);
    return load_aux(L, luaL_loadfile(L, fname));
}

static int luaB_loadstring(lua_State *L) {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    const char *chunkname = luaL_optstring(L, 2, s);
    return load_aux(L, luaL_loadbuffer(L, s, l, chunkname));
}

static int luaB_next(lua_State *L) {
    luaL_checktype(L, 1, 5);
    lua_setTop(L, 2);
    if (lua_next(L, 1))
        return 2;
    else {
        lua_pushnil(L);
        return 1;
    }
}

static int luaB_pcall(lua_State *L) {
    int status;
    luaL_checkany(L, 1);
    status = lua_pcall(L, lua_getTop(L) - 1, (-1), 0);
    lua_pushboolean(L, (status == 0));
    lua_insert(L, 1);
    return lua_getTop(L);
}

static int luaB_rawget(lua_State *L) {
    luaL_checktype(L, 1, 5);
    luaL_checkany(L, 2);
    lua_setTop(L, 2);
    lua_rawget(L, 1);
    return 1;
}

static void getfunc(lua_State *L, int opt) {
    if (lua_isfunction(L, 1))lua_pushValue(L, 1);
    else {
        lua_Debug ar;
        int level = opt ? luaL_optint(L, 1, 1) : luaL_checkint(L, 1);
        luaL_argcheck(L, level >= 0, 1, "level must be non-negative");
        if (lua_getstack(L, level, &ar) == 0)
            luaL_argerror(L, 1, "invalid level");
        lua_getinfo(L, "f", &ar);
        if (lua_isnil(L, -1))
            luaL_error(L, "no function environment for tail call at level %d",
                       level);
    }
}



static int luaB_setfenv(lua_State *L) {
    luaL_checktype(L, 2, 5);
    getfunc(L, 0);
    lua_pushValue(L, 2);
    if (lua_isnumber(L, 1) && lua_tonumber(L, 1) == 0) {
        lua_pushthread(L);
        lua_insert(L, -2);
        lua_setfenv(L, -2);
        return 0;
    } else if (lua_iscfunction(L, -2) || lua_setfenv(L, -2) == 0)
        luaL_error(L,
                   LUA_QL("setfenv")" cannot change environment of given object");
    return 1;
}

static int luaB_setmetatable(lua_State *L) {
    int t = lua_type(L, 2);
    luaL_checktype(L, 1, 5);
    luaL_argcheck(L, t == 0 || t == 5, 2,
                  "nil or table expected");
    if (luaL_getmetafield(L, 1, "__metatable"))
        luaL_error(L, "cannot change a protected metatable");
    lua_setTop(L, 2);
    lua_setmetatable(L, 1);
    return 1;
}

static int luaB_tonumber(lua_State *L) {
    int base = luaL_optint(L, 2, 10);
    if (base == 10) {
        luaL_checkany(L, 1);
        if (lua_isnumber(L, 1)) {
            lua_pushnumber(L, lua_tonumber(L, 1));
            return 1;
        }
    } else {
        const char *s1 = luaL_checkstring(L, 1);
        char *s2;
        unsigned long n;
        luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
        n = strtoul(s1, &s2, base);
        if (s1 != s2) {
            while (isspace((unsigned char) (*s2)))s2++;
            if (*s2 == '\0') {
                lua_pushnumber(L, (lua_Number) n);
                return 1;
            }
        }
    }
    lua_pushnil(L);
    return 1;
}

static int luaB_type(lua_State *L) {
    luaL_checkany(L, 1);
    lua_pushstring(L, luaL_typename(L, 1));
    return 1;
}

static int luaB_unpack(lua_State *L) {
    int i, e, n;
    luaL_checktype(L, 1, 5);
    i = luaL_optint(L, 2, 1);
    e = luaL_opt(L, luaL_checkint, 3, luaL_getn(L, 1));
    if (i > e)return 0;
    n = e - i + 1;
    if (n <= 0 || !lua_checkStack(L, n))
        return luaL_error(L, "too many results to unpack");
    lua_rawgeti(L, 1, i);
    while (i++ < e)
        lua_rawgeti(L, 1, i);
    return n;
}

static const luaL_Reg base_funcs[] = {
        {"assert",       luaB_assert},
        {"error",        luaB_error},
        {"loadfile",     luaB_loadfile},
        {"loadstring",   luaB_loadstring},
        {"next",         luaB_next},
        {"pcall",        luaB_pcall},
        {"rawget",       luaB_rawget},
        {"setfenv",      luaB_setfenv},
        {"setmetatable", luaB_setmetatable},
        {"tonumber",     luaB_tonumber},
        {"type",         luaB_type},
        {"unpack",       luaB_unpack},
        {NULL, NULL}
};

static void auxopen(lua_State *L, const char *name,
                    lua_CFunction f, lua_CFunction u) {
    lua_pushcfunction(L, u);
    lua_pushcclosure(L, f, 1);
    lua_setfield(L, -2, name);
}

static int luaB_ipairs(lua_State *L) {
    luaL_checktype(L, 1, 5);
    lua_pushValue(L, lua_upvalueindex(1));
    lua_pushValue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

static int ipairsaux(lua_State *L) {
    int i = luaL_checkint(L, 2);
    luaL_checktype(L, 1, 5);
    i++;
    lua_pushinteger(L, i);
    lua_rawgeti(L, 1, i);
    return (lua_isnil(L, -1)) ? 0 : 2;
}

static int luaB_pairs(lua_State *L) {
    luaL_checktype(L, 1, 5);
    lua_pushValue(L, lua_upvalueindex(1));
    lua_pushValue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int luaB_newproxy(lua_State *L) {
    lua_setTop(L, 1);
    lua_newuserdata(L, 0);
    if (lua_toboolean(L, 1) == 0)
        return 1;
    else if (lua_isboolean(L, 1)) {
        lua_newtable(L);
        lua_pushValue(L, -1);
        lua_pushboolean(L, 1);
        lua_rawset(L, lua_upvalueindex(1));
    } else {
        int validproxy = 0;
        if (lua_getmetatable(L, 1)) {
            lua_rawget(L, lua_upvalueindex(1));
            validproxy = lua_toboolean(L, -1);
            lua_pop(L, 1);
        }
        luaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
        lua_getmetatable(L, 1);
    }
    lua_setmetatable(L, 2);
    return 1;
}

static void base_open(lua_State *L) {
    lua_pushValue(L, (-10002));
    lua_setglobal(L, "_G");
    luaL_register(L, "_G", base_funcs);
    lua_pushliteral(L, "Lua mini");
    lua_setglobal(L, "_VERSION");
    auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
    auxopen(L, "pairs", luaB_pairs, luaB_next);
    lua_createTable(L, 0, 1);
    lua_pushValue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "kv");
    lua_setfield(L, -2, "__mode");
    lua_pushcclosure(L, luaB_newproxy, 1);
    lua_setglobal(L, "newproxy");
}

static int luaopen_base(lua_State *L) {
    base_open(L);
    return 1;
}


static void addfield(lua_State *L, luaL_Buffer *b, int i) {
    lua_rawgeti(L, 1, i);
    if (!lua_isstring(L, -1))
        luaL_error(L, "invalid value (%s) at index %d in table for "
                      LUA_QL("concat"), luaL_typename(L, -1), i);
    luaL_addvalue(b);
}

static int tconcat(lua_State *L) {
    luaL_Buffer b;
    size_t lsep;
    int i, last;
    const char *sep = luaL_optlstring(L, 2, "", &lsep);
    luaL_checktype(L, 1, 5);
    i = luaL_optint(L, 3, 1);
    last = luaL_opt(L, luaL_checkint, 4, luaL_getn(L, 1));
    luaL_buffinit(L, &b);
    for (; i < last; i++) {
        addfield(L, &b, i);
        luaL_addlstring(&b, sep, lsep);
    }
    if (i == last)
        addfield(L, &b, i);
    luaL_pushresult(&b);
    return 1;
}

#define aux_getn(L, n)(luaL_checktype(L,n,5),luaL_getn(L,n))

static int tinsert(lua_State *L) {
    int e = aux_getn(L, 1) + 1;
    int pos;
    switch (lua_getTop(L)) {
        case 2: {
            pos = e;
            break;
        }
        case 3: {
            int i;
            pos = luaL_checkint(L, 2);
            if (pos > e)e = pos;
            for (i = e; i > pos; i--) {
                lua_rawgeti(L, 1, i - 1);
                lua_rawseti(L, 1, i);
            }
            break;
        }
        default: {
            return luaL_error(L, "wrong number of arguments to "LUA_QL("insert"));
        }
    }
    luaL_setn(L, 1, e);
    lua_rawseti(L, 1, pos);
    return 0;
}

static int tremove(lua_State *L) {
    int e = aux_getn(L, 1);
    int pos = luaL_optint(L, 2, e);
    if (!(1 <= pos && pos <= e))
        return 0;
    luaL_setn(L, 1, e - 1);
    lua_rawgeti(L, 1, pos);
    for (; pos < e; pos++) {
        lua_rawgeti(L, 1, pos + 1);
        lua_rawseti(L, 1, pos);
    }
    lua_pushnil(L);
    lua_rawseti(L, 1, e);
    return 1;
}

static void set2(lua_State *L, int i, int j) {
    lua_rawseti(L, 1, i);
    lua_rawseti(L, 1, j);
}

static int sort_comp(lua_State *L, int a, int b) {
    if (!lua_isnil(L, 2)) {
        int res;
        lua_pushValue(L, 2);
        lua_pushValue(L, a - 1);
        lua_pushValue(L, b - 2);
        lua_call(L, 2, 1);
        res = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return res;
    } else
        return lua_lessthan(L, a, b);
}


static void auxsort(lua_State *L, int l, int u) {
    while (l < u) {
        int i, j;
        lua_rawgeti(L, 1, l);
        lua_rawgeti(L, 1, u);
        if (sort_comp(L, -1, -2))
            set2(L, l, u);
        else
            lua_pop(L, 2);
        if (u - l == 1)break;
        i = (l + u) / 2;
        lua_rawgeti(L, 1, i);
        lua_rawgeti(L, 1, l);
        if (sort_comp(L, -2, -1))
            set2(L, i, l);
        else {
            lua_pop(L, 1);
            lua_rawgeti(L, 1, u);
            if (sort_comp(L, -1, -2))
                set2(L, i, u);
            else
                lua_pop(L, 2);
        }
        if (u - l == 2)break;
        lua_rawgeti(L, 1, i);
        lua_pushValue(L, -1);
        lua_rawgeti(L, 1, u - 1);
        set2(L, i, u - 1);
        i = l;
        j = u - 1;
        for (;;) {
            while (lua_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
                if (i > u)luaL_error(L, "invalid order function for sorting");
                lua_pop(L, 1);
            }
            while (lua_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
                if (j < l)luaL_error(L, "invalid order function for sorting");
                lua_pop(L, 1);
            }
            if (j < i) {
                lua_pop(L, 3);
                break;
            }
            set2(L, i, j);
        }
        lua_rawgeti(L, 1, u - 1);
        lua_rawgeti(L, 1, i);
        set2(L, u - 1, i);
        if (i - l < u - i) {
            j = l;
            i = i - 1;
            l = i + 2;
        } else {
            j = i + 1;
            i = u;
            u = j - 2;
        }
        auxsort(L, j, i);
    }
}

static int sort(lua_State *L) {
    int n = aux_getn(L, 1);
    luaL_checkstack(L, 40, "");
    if (!lua_isnoneornil(L, 2))
        luaL_checktype(L, 2, 6);
    lua_setTop(L, 2);
    auxsort(L, 1, n);
    return 0;
}

static const luaL_Reg tab_funcs[] = {
        {"concat", tconcat},
        {"insert", tinsert},
        {"remove", tremove},
        {"sort",   sort},
        {NULL, NULL}
};

static int luaopen_table(lua_State *L) {
    luaL_register(L, "table", tab_funcs);
    return 1;
}

static void createmeta(lua_State *L) {
    luaL_newmetatable(L, "FILE*");
    lua_pushValue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_register(L, NULL, flib);
}

static void newfenv(lua_State *L, lua_CFunction cls) {
    lua_createTable(L, 0, 1);
    lua_pushcfunction(L, cls);
    lua_setfield(L, -2, "__close");
}

static int io_pclose(lua_State *L) {
    FILE **p = tofilep(L);
    int ok = lua_pclose(L, *p);
    *p = NULL;
    return pushresult(L, ok, NULL);
}

static int io_fclose(lua_State *L) {
    FILE **p = tofilep(L);
    int ok = (fclose(*p) == 0);
    *p = NULL;
    return pushresult(L, ok, NULL);
}

static int io_noclose(lua_State *L) {
    lua_pushnil(L);
    lua_pushliteral(L, "cannot close standard file");
    return 2;
}

static void createstdfile(lua_State *L, FILE *f, int k, const char *fname) {
    *newfile(L) = f;
    if (k > 0) {
        lua_pushValue(L, -1);
        lua_rawseti(L, (-10001), k);
    }
    lua_pushValue(L, -2);
    lua_setfenv(L, -2);
    lua_setfield(L, -3, fname);
}

static int luaopen_io(lua_State *L) {
    createmeta(L);
    newfenv(L, io_fclose);
    lua_replace(L, (-10001));
    luaL_register(L, "io", iolib);
    newfenv(L, io_noclose);
    createstdfile(L, stdin, 1, "stdin");
    createstdfile(L, stdout, 2, "stdout");
    createstdfile(L, stderr, 0, "stderr");
    lua_pop(L, 1);
    lua_getfield(L, -1, "popen");
    newfenv(L, io_pclose);
    lua_setfenv(L, -2);
    lua_pop(L, 1);
    return 1;
}

static int luaopen_os(lua_State *L) {
    luaL_register(L, "os", syslib);
    return 1;
}

static void createmetatable(lua_State *L) {
    lua_createTable(L, 0, 1);
    lua_pushliteral(L, "");
    lua_pushValue(L, -2);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
    lua_pushValue(L, -2);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static int luaopen_string(lua_State *L) {
    luaL_register(L, "string", strlib);
    createmetatable(L);
    return 1;
}

const luaL_Reg luaLibs[] = {
        {"",       luaopen_base},
        {"table",  luaopen_table},
        {"io",     luaopen_io},
        {"os",     luaopen_os},
        {"string", luaopen_string},
        {NULL, NULL}
};