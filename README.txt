dwm-win32 is a port of the well known X11 window manager dwm to the Microsoft Windows platform.

TODO
 - focus handling / stealling / floating windows (ex. WinSCP)
 - fix redraw problems which happen from time to time
 - fullscreen windows? Screensaver?
 - window border currently it's drawn into the client area which is bad
 - status text via stdin or a separate tool
 - crash handler which makes all windows visible restores borders etc
 - code cleanups all over the place
 - multi head support?

 [ - introduce a ShellProc function and register it with SetWindowsHookEx
     to handle window events instead of current mechanism in WndProc which
     is based on the shellhookid but seems to be bugy in some cases?.
     Unfortunately the SetWindowsHookEx thingy seems to require a separate 
     dll which sucks. ]
