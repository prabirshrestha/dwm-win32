#include "dwm.h"
#include "hotkey.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>

#include <windows.h>

static int f_bind(lua_State *L);

int
luaopen_dwm_hotkey(lua_State *L) {
	const luaL_Reg lib[] = {
		{ "bind", f_bind },
		{ NULL, NULL }
	};

	luaL_newlib(L, lib);

	return 1;
}

int
f_bind(lua_State *L) {
	const char *msg = "TODO";
	MessageBox(NULL, msg, "dwm-win32 log", MB_OK);
	return 0;
}
