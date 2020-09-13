#include "client.h"

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <windows.h>

typedef struct EnumWindowsState {
	lua_State *L;
	size_t index;
} EnumWindowsState;

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);

static int modclient_getClients(lua_State *L);

static const struct  luaL_reg dwmclientmod[] = {
	{ "getClients", modclient_getClients },
	{ NULL, NULL }
};

int dwmmod_openclient(lua_State *L) {
	luaL_register(L, "dwm.client", dwmclientmod);
	return 1;
}

static int modclient_getClients(lua_State *L) {
	lua_newtable(L);

	EnumWindowsState state;
	state.L = L;
	state.index = 0;

	EnumWindows(EnumWindowsCallback, (LPARAM)&state);

	return 1; /* number of results */
}


BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
	EnumWindowsState *state = (EnumWindowsState*)lParam;

	state->index += 1;
	lua_pushnumber(state->L, state->index);
	lua_pushnumber(state->L, (uint32_t)hwnd);
	lua_settable(state->L, -3);

	return TRUE;
}
