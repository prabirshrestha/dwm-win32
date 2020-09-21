#include "client.h"

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>

#include <windows.h>
#include <dwmapi.h>

#include "../win32_utf8.h"

typedef struct EnumWindowsState {
	lua_State *L;
	size_t index;
} EnumWindowsState;

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);

static char *getclienttitle(HWND hwnd);
static char *getclientclassname(HWND hwnd);
static BOOL iscloaked(HWND hwnd);

static int f_clients(lua_State *L);
static int f_client(lua_State *L);
static int f_show(lua_State *L);
static int f_hide(lua_State *L);
static int f_border(lua_State *L);
static int f_close(lua_State *L);
static int f_focus(lua_State *L);
static int f_maximize(lua_State *L);
static int f_minimize(lua_State *L);
static int f_position(lua_State *L);

int
luaopen_dwm_client(lua_State *L) {
	const luaL_Reg lib[] = {
		{ "clients", f_clients },
		{ "client", f_client },
		{ "show", f_show },
		{ "hide", f_hide },
		{ "border", f_border },
		{ "close", f_close },
		{ "focus", f_focus },
		{ "maximize", f_maximize },
		{ "minimize", f_minimize },
		{ "position", f_position },
		{ NULL, NULL }
	};

	luaL_newlib(L, lib);

	return 1;
}

static int f_clients(lua_State *L) {
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

static int f_client(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	if (!IsWindow(hwnd)) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	lua_pushstring(L, "id");
	lua_pushnumber(L, id);
	lua_settable(L, -3);

	lua_pushstring(L, "visible");
	lua_pushboolean(L, IsWindowVisible(hwnd) ? 1 : 0);
	lua_settable(L, -3);

	char *clienttitle = getclienttitle(hwnd);
	if (clienttitle) {
		lua_pushstring(L, "title");
		lua_pushstring(L, getclienttitle(hwnd));
		lua_settable(L, -3);
		free(clienttitle);
	}

	char *classname = getclientclassname(hwnd);
	if (classname) {
		lua_pushstring(L, "classname");
		lua_pushstring(L, getclientclassname(hwnd));
		lua_settable(L, -3);
		free(classname);
	}

	lua_pushstring(L, "parent");
	HWND parent = GetParent(hwnd);
	if (parent)
		lua_pushnumber(L, (uint32_t)parent);
	else
		lua_pushnil(L);
	lua_settable(L, -3);

	lua_pushstring(L, "owner");
	HWND owner = GetWindow(hwnd, GW_OWNER);
	if (owner)
		lua_pushnumber(L, (uint32_t)owner);
	else
		lua_pushnil(L);
	lua_settable(L, -3);

	lua_pushstring(L, "cloaked");
	lua_pushboolean(L, iscloaked(hwnd) ? 1 : 0);
	lua_settable(L, -3);

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	lua_pushstring(L, "pid");
	lua_pushnumber(L, (uint32_t)pid);
	lua_settable(L, -3);

	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

	lua_pushstring(L, "winstyle");
	lua_pushnumber(L, style);
	lua_settable(L, -3);

	lua_pushstring(L, "winexstyle");
	lua_pushnumber(L, exstyle);
	lua_settable(L, -3);

	return 1;
}

int
f_show(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	ShowWindow(hwnd, SW_SHOW);

	return 0;
}

int
f_hide(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	ShowWindow(hwnd, SW_HIDE);

	return 0;
}

int
f_border(lua_State *L) {
	uint32_t argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "expecting exactly 2 arguments");

	if (!lua_isnumber(L, 1))
		return luaL_error(L, "expecting first argument to be of type number");

	if (!lua_isboolean(L, 2))
		return luaL_error(L, "expecting second argument to be of type boolean");

	uint32_t id = (uint32_t)lua_tonumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	uint32_t border = (uint32_t)lua_toboolean(L, 2); /* second args*/

	if (border > 0)
		SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) | (WS_CAPTION | WS_SIZEBOX)));
	else {
		SetWindowLong(hwnd, GWL_STYLE, (GetWindowLong(hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_SIZEBOX)) | WS_BORDER | WS_THICKFRAME);
		SetWindowLong(hwnd, GWL_EXSTYLE, (GetWindowLong(hwnd, GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE)));
	}

	return 0;
}

int
f_close(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	SendMessageA(hwnd, WM_CLOSE, 0, 0);

	return 0;
}

int
f_focus(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;
	SetForegroundWindow(hwnd);
	return 0;
}

int
f_maximize(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;
	SendMessageA(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	return 0;
}

int
f_minimize(lua_State *L) {
	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;
	SendMessageA(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return 0;
}

int
f_position(lua_State *L) {
	uint32_t argc = lua_gettop(L);
	if (argc != 2)
		return luaL_error(L, "expecting exactly 2 arguments");

	if (!lua_isnumber(L, 1))
		return luaL_error(L, "expecting first argument to be of type number");

	if (!lua_istable(L, 2))
		return luaL_error(L, "expecting second argument to be of type table");

	uint32_t id = (uint32_t)luaL_checknumber(L, 1); /* first arg */
	HWND hwnd = (HWND)id;

	lua_getfield(L, 2, "x");
	if (!lua_isnumber(L, -1))
		return luaL_error(L, "expecting x to be number");
	uint32_t x = lua_tonumber(L, -1);

	lua_getfield(L, 2, "y");
	if (!lua_isnumber(L, -1))
		return luaL_error(L, "expecting y to be number");
	uint32_t y = lua_tonumber(L, -1);

	lua_getfield(L, 2, "width");
	if (!lua_isnumber(L, -1))
		return luaL_error(L, "expecting width to be number");
	uint32_t width = lua_tonumber(L, -1);

	lua_getfield(L, 2, "height");
	if (!lua_isnumber(L, -1))
		return luaL_error(L, "expecting height to be number");
	uint32_t height = lua_tonumber(L, -1);

	SetWindowPos(hwnd, NULL, x, y, width, height, 0);

	return 0;
}

char
*getclienttitle(HWND hwnd) {
    static wchar_t buf[500];
    GetWindowTextW(hwnd, buf, sizeof buf);
	return (char*)utf16_to_utf8(buf);
}

char
*getclientclassname(HWND hwnd) {
    static wchar_t buf[500];
    GetClassNameW(hwnd, buf, sizeof buf);
	return (char*)utf16_to_utf8(buf);
}

BOOL
iscloaked(HWND hwnd) {
    int cloaked_val;
    HRESULT h_res = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked_val, sizeof(cloaked_val));

    if (h_res != S_OK)
        cloaked_val = 0;

    return cloaked_val ? TRUE : FALSE;
}
