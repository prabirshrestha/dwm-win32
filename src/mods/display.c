#include "display.h"

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>

#include <windows.h>

static int f_displays(lua_State *L);
static int f_display(lua_State *L);

int
luaopen_dwm_display(lua_State *L) {
	struct luaL_Reg lib[] = {
		{ "displays", f_displays },
		{ "display", f_display },
		{ NULL, NULL }
	};

	luaL_newlib(L, lib);

	return 1;
}

static int f_displays(lua_State *L) {
	lua_newtable(L);

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);

	DWORD devicenum = 0;

	uint32_t index = 0;

	while(EnumDisplayDevices(NULL, devicenum, &dd, 0)) {
		if (!(dd.StateFlags && DISPLAY_DEVICE_ACTIVE)) continue;
		DISPLAY_DEVICE newdd = {0};
		newdd.cb = sizeof(DISPLAY_DEVICE);
		DWORD monitornum = 0;
		while(EnumDisplayDevices(dd.DeviceName, monitornum, &newdd, 0)) {
			lua_pushnumber(L, ++index);
			lua_pushstring(L, newdd.DeviceKey);
			lua_settable(L, -3);
			++monitornum;
		}
		++devicenum;
	}

	return 1; /* number of results */
}

static int f_display(lua_State *L) {
	uint32_t argc = lua_gettop(L);
	if (argc != 1)
		return luaL_error(L, "expecting exactly 1 argument");

	if (!lua_isstring(L, 1))
		return luaL_error(L, "expecting first argument to be of type string");

	const char *devicekey = lua_tostring(L, 1); /* first arg */

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);

	DWORD devicenum = 0;

	DEVMODE dm;

	BOOL found = FALSE;

	while(EnumDisplayDevices(NULL, devicenum, &dd, 0)) {
		if (!(dd.StateFlags && DISPLAY_DEVICE_ACTIVE)) continue;
		DISPLAY_DEVICE newdd = {0};
		newdd.cb = sizeof(DISPLAY_DEVICE);
		DWORD monitornum = 0;
		while(EnumDisplayDevices(dd.DeviceName, monitornum, &newdd, 0)) {
			if (strcmp(newdd.DeviceKey, devicekey) != 0)
				continue;

			lua_newtable(L);
			found = TRUE;

			lua_pushstring(L, "id");
			lua_pushstring(L, newdd.DeviceKey);
			lua_settable(L, -3);

			dm.dmSize = sizeof(dm);
			dm.dmScale = sizeof(dm);
			dm.dmDriverExtra = 0;

			if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
				lua_pushstring(L, "x");
				lua_pushnumber(L, dm.dmPosition.x);
				lua_settable(L, -3);

				lua_pushstring(L, "y");
				lua_pushnumber(L, dm.dmPosition.y);
				lua_settable(L, -3);

				lua_pushstring(L, "width");
				lua_pushnumber(L, dm.dmPelsWidth);
				lua_settable(L, -3);

				lua_pushstring(L, "height");
				lua_pushnumber(L, dm.dmPelsHeight);
				lua_settable(L, -3);
			}

			++monitornum;
		}
		++devicenum;
	}

	if (!found)
		lua_pushnil(L);

	return 1; /* number of results */
}
