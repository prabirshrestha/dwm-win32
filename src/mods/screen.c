#include "screen.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

static const struct  luaL_reg dwmscreenmod[] = {
	{ NULL, NULL }
};

int dwmmod_openscreen(lua_State *L) {
	luaL_register(L, "dwm.screen", dwmscreenmod);
	return 1;
}
