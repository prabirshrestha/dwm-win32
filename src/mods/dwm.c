#include "dwm.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <windows.h>

static void get_exe_filename(char *buf, int size);

static int moddwm_f_log(lua_State *L);

static const luaL_Reg dwmmod[] = {
	{ "log", moddwm_f_log },
	{ NULL, NULL }
};

int
dwmmod_opendwm(lua_State *L) {
	luaL_register(L, "dwm", dwmmod);

	luaL_openlib(L, "dwm", dwmmod, 0);

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

void
get_exe_filename(char *buf, int size) {
	int len = GetModuleFileName(NULL, buf, size - 1);
	buf[len] = '\0';
}

int
moddwm_f_log(lua_State *L) {
	// TODO: support utf-8
	const char *msg = luaL_checkstring(L, 1);
	MessageBox(NULL, msg, "dwm-win32 log", MB_OK);
	return 0;
}
