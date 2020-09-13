# dwm-win32 lua api

# dwm module

```lua
local dwm = require 'dwm'
```

## dwm.log

```lua
local dwm = require 'dwm'
dwm.log('Hello World!')
```

## dwm.VERSION

```lua
local dwm = require 'dwm'
dwm.log(dwm.VERSION) -- ex: 1.0.0
```

## dwm.PLATFORM

```lua
local dwm = require 'dwm'
dwm.log(dwm.PLATFORM) -- ex: Windows
```

## dwm.EXEFILE

```lua
local dwm = require 'dwm'
dwm.log(dwm.EXEFILE) -- ex: c:\apps\dwm-win32.exe
```

# display mod

```lua
local display = require 'dwm.display'
```

## Get all available displays

```lua
local moddisplay = require 'dwm.display'

local displays = moddisplay.getDisplays()

local display1 = displays[1]
local x = display1.x
local y = display.y
local width = display.width
local height = display.height
```
