#include "dwm.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>

int
luaopen_dwm_eventemitter(lua_State *L) {
	const luaL_Reg lib[] = {
		{ NULL, NULL }
	};

	luaL_newlib(L, lib);

	(void) luaL_dostring(L, 
		"local M = require 'dwm.eventemitter'\n"
		"local EventEmitter = {}\n"
		"EventEmitter.__index = EventEmitter\n"
		"function EventEmitter.new()\n"
		"  local self = setmetatable({}, EventEmitter)\n"
		"  self._listeners = {}\n"
		"  return self\n"
		"end\n"
		"function EventEmitter:on(event, listener)\n"
		"  local listeners = self._listeners[event]\n"
		"  if not listeners then\n"
		"    self._listeners[event] = {}\n"
		"    listeners = self._listeners[event]"
		"  end\n"
		"  listeners[#listeners + 1] = listener\n"
		"end\n"
		"function EventEmitter:off(event, listener)\n"
		"  local listeners = self._listeners[event]\n"
		"  if not listeners then return end\n"
		"  for i = 1, #listeners do\n"
		"    if listeners[i] == listener then\n"
		"      table.remove(listeners, i)\n"
		"      break\n"
		"    end\n"
		"  end\n"
		"  if #listeners == 0 then self._listeners[event] = nil end\n"
		"end\n"
		"function EventEmitter:emit(event, ...)\n"
		"  local listeners = self._listeners[event]\n"
		"  if listeners then\n"
		"    for i = 1, #listeners do\n"
		"      listeners[i](...)\n"
		"    end\n"
		"  end\n"
		"end\n"
		"M.new = EventEmitter.new\n"
		"\n");

	return 1;
}
