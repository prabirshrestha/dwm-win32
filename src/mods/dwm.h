#ifndef DWM_MOD_DWM
#define DWM_MOD_DWM

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <windows.h>

typedef struct {
    HINSTANCE hInstance;
} DwmState;

int luaopen_dwm(lua_State *L);

void dwm_setstate(lua_State *L, DwmState *state);
DwmState *dwm_getstate(lua_State *L);

#endif
