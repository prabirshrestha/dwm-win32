#include "dwm.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>

#include <windows.h>

static int f_log(lua_State *L);

static void get_exe_filename(char *buf, int size);

int
luaopen_dwm(lua_State *L) {
	const luaL_Reg lib[] = {
		{ "log", f_log },
		{ NULL, NULL }
	};

	luaL_newlib(L, lib);

	lua_pushstring(L, "VERSION");
	lua_pushstring(L, "0.2.0"); // TODO: use PROJECT_VER
	lua_settable (L, -3);

	lua_pushstring(L, "PLATFORM");
	lua_pushstring(L, "Windows");
	lua_settable (L, -3);

	char exename[2048];
	get_exe_filename(exename, sizeof(exename));
	lua_pushstring(L, "EXEFILE");
	lua_pushstring(L, exename);
	lua_settable (L, -3);

	return 1;
}

int
f_log(lua_State *L) {
	// TODO: support utf-8
	const char *msg = luaL_checkstring(L, 1);
	MessageBox(NULL, msg, "dwm-win32 log", MB_OK);
	return 0;
}

void
get_exe_filename(char *buf, int size) {
	int len = GetModuleFileName(NULL, buf, size - 1);
	buf[len] = '\0';
}
