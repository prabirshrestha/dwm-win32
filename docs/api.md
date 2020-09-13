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
for _, displayid = ipairs(dwmdisplay.displays()) do
   print(displayid)
end
```

## Get display information

```lua
local display = dwmdisplay.display(displayid)
print(display.id)
print(display.x)
print(display.y)
print(display.width)
print(display.height)
```

# client mod

```lua
local dwmclient = require 'dwm.client'
```

## Get all clients

```lua
for _, clientid = ipairs(dwmclient.clients()) do
    print(clientid)
end
```

## Get client information

```lua
local client = dwmclient.client(clientid)
print(client.visible)
print(client.title)
print(client.classname)
print(client.parent)
print(client.owner)
print(client.cloaked)
print(client.pid) -- process id
print(client.winstyle)
print(client.winexstyle)
```

## Set client focus

```lua
dwmclient.focus(clientid)
```

## Hide client

```lua
dwmclient.hide(clientid)
```

## Show client

```lua
dwmclient.show(clientid)
```

## Close client

```lua
dwmclient.close(clientid)
```

## Maximize client

```lua
dwmclient.maximize(clientid)
```

## Minimize client

```lua
dwmclient.minimize(clientid)
```

## Set border

```lua
dwmclient.border(clientid, true)  -- show border
dwmclient.border(clientid, false) -- hide border
```

## Set position

```lua
dwmclient.position(clientid, {
    x = 10,
    y = 20,
    width = 200,
    height = 300,
})
```
