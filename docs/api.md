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
local dwmdisplay = require 'dwm.display'
```

## Get all displays

```lua
local dwmdisplay = require 'dwm.display'

local displays = dwmdisplay.getDisplays()

local display1 = displays[1]
local x = display1.x
local y = display.y
local width = display.width
local height = display.height
```

# client mod

```lua
local dwmclient = require 'dwm.client'
```

## Get all clients

```lua
for _, clientid = ipairs(dwmclient.getClients()) do
    -- clientid is a number
end
```

## Get client information

```lua
local client = dwmclient.getClient(clientid)
```

## Set client visiblity

```lua
dwmclient.setVisibility(clientid, false) -- false to hide client
dwmclient.setVisibility(clientid, true)  -- true to show client
```
