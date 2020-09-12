#include "screen.h"

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <windows.h>

static int modscreens_getScreens(lua_State *L);

static const struct  luaL_reg dwmscreenmod[] = {
	{ "getScreens", modscreens_getScreens },
	{ NULL, NULL }
};

int dwmmod_openscreen(lua_State *L) {
	luaL_register(L, "dwm.screen", dwmscreenmod);
	return 1;
}

static int modscreens_getScreens(lua_State *L) {
	lua_newtable(L);

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);

	DWORD devicenum = 0;

	uint32_t index = 0;

	while(EnumDisplayDevices(NULL, devicenum, &dd, 0)) {
		DISPLAY_DEVICE newdd = {0};
		newdd.cb = sizeof(DISPLAY_DEVICE);
		DWORD monitornum = 0;
		while(EnumDisplayDevices(dd.DeviceName, monitornum, &newdd, 0)) {
			lua_pushnumber(L, ++index);				/* push key. lua array starts with 1 */
			lua_pushstring(L, newdd.DeviceName);	/* push value*/
			lua_settable(L, -3);					/* add to table */
			++monitornum;
		}
		++devicenum;
	}

	return 1; /* number of results */
}
