/* See LICENSE file for copyright and license details.
 *
 * This is a port of the popular X11 window manager dwm to Microsoft Windows.
 * It was originally started by Marc Andre Tanner <mat at brain-dump dot org>
 *
 * Each child of the root window is called a client. Clients are organized 
 * in a global linked client list, the focus history is remembered through 
 * a global stack list. Each client contains a bit array to indicate the 
 * tags of a client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading WinMain().
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT            0x0600

#if _MSC_VER
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dwmapi.lib")
#endif

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <compat-5.3.h>
#ifndef LUAJIT
#include "../extern/luabitop/bit.c"
#endif

#include <windows.h>
#include <dwmapi.h>
#include <winuser.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shellapi.h>
#include <stdbool.h>
#include <time.h>

#include "mods/dwm.h"
#include "mods/eventemitter.h"
#include "mods/display.h"
#include "mods/client.h"
#include "mods/hotkey.h"

#define NAME                    L"dwm-win32"     /* Used for window name/class */

#define ISVISIBLE(x)            ((x)->tags & tagset[seltags])
#define ISFOCUSABLE(x)            (!(x)->isminimized && ISVISIBLE(x) && IsWindowVisible((x)->hwnd))
#define LENGTH(x)               (sizeof x / sizeof x[0])
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAXTAGLEN               16
#define WIDTH(x)                ((x)->w + 2 * (x)->bw)
#define HEIGHT(x)               ((x)->h + 2 * (x)->bw)
#define TAGMASK                 ((int)((1LL << LENGTH(tags)) - 1))
#define TEXTW(x)                (textnw(x, wcslen(x)))

#ifdef NDEBUG
#define debug(...) do { } while (false)
#else
#define debug(...) eprint(false, __VA_ARGS__)
#endif

#define die(...) if (TRUE) { eprint(true, __VA_ARGS__); eprint(true, L"Win32 Last Error: %d", GetLastError()); cleanup(NULL); exit(EXIT_FAILURE); }

#define EVENT_OBJECT_CLOAKED 0x8017
#define EVENT_OBJECT_UNCLOAKED 0x8018

HHOOK g_KeybdHook = NULL;

enum { CurNormal, CurResize, CurMove, CurLast };        /* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };            /* color */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle };    /* clicks */

typedef struct {
    int x, y, w, h;
    unsigned long norm[ColLast];
    unsigned long sel[ColLast];
    HDC hdc;
} DC; /* draw context */

DC dc;

typedef union {
    int i;
    unsigned int ui;
    float f;
    void *v;
} Arg;

typedef struct {
    unsigned int click;
    unsigned int button;
    unsigned int key;
    void (*func)(const Arg *arg);
    const Arg arg;
} Button;

typedef struct Client Client;
struct Client {
    HWND hwnd;
    HWND parent;
    HWND root;
    int x, y, w, h;
    int bw; // XXX: useless?
    unsigned int tags;
    bool isminimized;
    bool isfloating;
    bool isalive;
    bool ignore;
    bool ignoreborder;
    bool border;
    bool wasvisible;
    bool isfixed, isurgent; // XXX: useless?
    bool iscloaked; // WinStore apps
    Client *next;
    Client *snext;
};

typedef struct {
    unsigned int mod;
    unsigned int key;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

typedef struct {
    const wchar_t *symbol;
    void (*arrange)(void);
} Layout;

typedef struct {
    const wchar_t *class;
    const wchar_t *title;
    const wchar_t *processname;
    unsigned int tags;
    bool isfloating;
    bool ignoreborder;
} Rule;

/* function declarations */
static void applyrules(Client *c);
static void arrange(void);
static void attach(Client *c);
static void attachstack(Client *c);
static void cleanup(lua_State *L);
static void clearurgent(Client *c);
static void detach(Client *c);
static void detachstack(Client *c);
static void drawbar(void);
static void drawsquare(bool filled, bool empty, bool invert, unsigned long col[ColLast]);
static void drawtext(const wchar_t *text, unsigned long col[ColLast], bool invert);
void drawborder(Client *c, COLORREF color);
void eprint(bool premortem, const wchar_t *errstr, ...);
static void focus(Client *c);
static void focusstack(const Arg *arg);
static void movestack(const Arg *arg);
static void forcearrange(const Arg *arg);
static Client *getclient(HWND hwnd);
LPWSTR getclientclassname(HWND hwnd);
LPWSTR getclienttitle(HWND hwnd);
LPWSTR getclientprocessname(HWND hwnd);
HWND getroot(HWND hwnd);
static void grabkeys(HWND hwnd);
static void killclient(const Arg *arg);
static Client *manage(HWND hwnd);
static void monocle(void);
static Client *nextchild(Client *p, Client *c);
static Client *nexttiled(Client *c);
static void quit(const Arg *arg);
static void resize(Client *c, int x, int y, int w, int h);
static void restack(void);
static BOOL CALLBACK scan(HWND hwnd, LPARAM lParam);
static void setborder(Client *c, bool border);
static void setvisibility(HWND hwnd, bool visibility);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(lua_State *L, HINSTANCE hInstance);
static void setupbar(HINSTANCE hInstance);
static void showclientinfo(const Arg *arg); 
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static int textnw(const wchar_t *text, unsigned int len);
static void tile(void);
static void togglebar(const Arg *arg);
static void toggleborder(const Arg *arg);
static void toggleexplorer(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void writelog(const Arg *arg);
static void unmanage(Client *c);
static void updatebar(void);
static void updategeom(void);
static void view(const Arg *arg);
static void zoom(const Arg *arg);
static bool iscloaked(HWND hwnd);

typedef BOOL (*RegisterShellHookWindowProc) (HWND);

static HWND dwmhwnd, barhwnd;
static HWINEVENTHOOK wineventhook;
static HFONT font;
static wchar_t stext[256];
static int sx, sy, sw, sh; /* X display screen geometry x, y, width, height */ 
static int bx, by, bh, blw;    /* bar geometry x, y, height and layout symbol width */
static int wx, wy, ww, wh; /* window area geometry x, y, width, height, bar excluded */
static unsigned int seltags, sellt;

static Client *clients = NULL;
static Client *sel = NULL;
static Client *stack = NULL;
static Layout *lt[] = { NULL, NULL };
static UINT shellhookid;    /* Window Message id */

/* configuration, allows nested code to access above variables */
#include "config.h"

static int curtag = 1, prevtag = 1;
static Layout *lts[LENGTH(tags) + 1];
static double mfacts[LENGTH(tags) + 1];
static bool showbars[LENGTH(tags) + 1];

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { wchar_t limitexceeded[sizeof(unsigned int) * 8 < LENGTH(tags) ? -1 : 1]; };

/* elements of the window whose color should be set to the values in the array below */
static int colorwinelements[] = { COLOR_ACTIVEBORDER, COLOR_INACTIVEBORDER };
static COLORREF colors[2][LENGTH(colorwinelements)] = { 
    { 0, 0 }, /* used to save the values before dwm started */
    { selbordercolor, normbordercolor },
};


void
applyrules(Client *c) {
    unsigned int i;
    Rule *r;

    /* rule matching */
    for (i = 0; i < LENGTH(rules); i++) {
        r = &rules[i];
        if ((!r->title || wcsstr(getclienttitle(c->hwnd), r->title))
        && (!r->class || wcsstr(getclientclassname(c->hwnd), r->class))
        && (!r->processname || wcsstr(getclientprocessname(c->hwnd), r->processname))) {
            c->isfloating = r->isfloating;
            c->ignoreborder = r->ignoreborder;
            c->tags |= r->tags & TAGMASK ? r->tags & TAGMASK : tagset[seltags]; 
        }
    }
    if (!c->tags)
        c->tags = tagset[seltags];
}

void
arrange(void) {
    showhide(stack);
    focus(NULL);
    if (lt[sellt]->arrange)
        lt[sellt]->arrange();
    restack();
}

void
attach(Client *c) {
    c->next = clients;
    clients = c;
}

void
attachstack(Client *c) {
    c->snext = stack;
    stack = c;
}

void
buttonpress(unsigned int button, POINTS *point) {
    unsigned int i, x, click;
    Arg arg = {0};

    /* XXX: hack */
    dc.hdc = GetWindowDC(barhwnd);

    i = x = 0;

    do { x += TEXTW(tags[i]); } while (point->x >= x && ++i < LENGTH(tags));
    if (i < LENGTH(tags)) {
        click = ClkTagBar;
        arg.ui = 1 << i;
    }
    else if (point->x < x + blw)
        click = ClkLtSymbol;
    else if (point->x > wx + ww - TEXTW(stext))
        click = ClkStatusText;
    else
        click = ClkWinTitle;

    if (GetKeyState(VK_SHIFT) < 0)
        return;

    for (i = 0; i < LENGTH(buttons); i++) {
        if (click == buttons[i].click && buttons[i].func && buttons[i].button == button
            && (!buttons[i].key || GetKeyState(buttons[i].key) < 0)) {
            buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
            break;
        }
    }
}

void
cleanup(lua_State *L) {
    int i;
    Arg a = {.ui = ~0};
    Layout foo = { L"", NULL };

    if (barhwnd)
        KillTimer(barhwnd, 1);
    
    for (i = 0; i < LENGTH(keys); i++) {
        UnregisterHotKey(dwmhwnd, i);
    }

    DeregisterShellHookWindow(dwmhwnd);

    if (wineventhook != NULL)
        UnhookWinEvent(wineventhook);

    view(&a);
    lt[sellt] = &foo;
    while (stack)
        unmanage(stack);

    SetSysColors(LENGTH(colorwinelements), colorwinelements, colors[0]); 

    DestroyWindow(dwmhwnd);

    HWND hwnd;
    hwnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hwnd)
        setvisibility(hwnd, TRUE);

    if (font)
        DeleteObject(font);

	if (L) {
		lua_close(L);
		L = NULL;
	}
}

void
clearurgent(Client *c) {
    c->isurgent = false;
}

void
detach(Client *c) {
    Client **tc;

    for (tc = &clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
}

void
detachstack(Client *c) {
    Client **tc;

    for (tc = &stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;
}

void
drawbar(void) {
    dc.hdc = GetWindowDC(barhwnd);

    dc.h = bh;

    int x;
    unsigned int i, occ = 0, urg = 0;
    unsigned long *col;
    Client *c;
    time_t timer;
    struct tm date;
    wchar_t timestr[256];
    wchar_t localtimestr[256];
    wchar_t utctimestr[256];
    wchar_t batterystr[256];
    wchar_t out[256];


    for (c = clients; c; c = c->next) {
        occ |= c->tags;
        if (c->isurgent)
            urg |= c->tags;
    }

    dc.x = 0;
    for (i = 0; i < LENGTH(tags); i++) {
        dc.w = TEXTW(tags[i]);
        col = tagset[seltags] & 1 << i ? dc.sel : dc.norm;
        drawtext(tags[i], col, urg & 1 << i);
        drawsquare(sel && sel->tags & 1 << i, occ & 1 << i, urg & 1 << i, col);
        dc.x += dc.w;
    }
    if (blw > 0) {
        dc.w = blw;
        drawtext(lt[sellt]->symbol, dc.norm, false);
        x = dc.x + dc.w;
    }
    else
        x = dc.x;
    dc.w = TEXTW(stext);
    dc.x = ww - dc.w;
    if (dc.x < x) {
        dc.x = x;
        dc.w = ww - x;
    }
    drawtext(stext, dc.norm, false);

    if (showclock) {
        /* Draw Date Time */
        timer = time(NULL);
        localtime_s(&date, &timer);
        wcsftime(localtimestr, 255, clockfmt, &date);

        if (showutcclock) {
            timer = time(NULL);
            gmtime_s(&date, &timer);
            wcsftime(utctimestr, 255, clockfmt, &date);

            swprintf(timestr, sizeof(timestr), L"%ls | UTC: %ls", localtimestr, utctimestr);
        } else {
            swprintf(timestr, sizeof(localtimestr), L"%ls", localtimestr);
        }
    }
    // Battery status
    if (showBattery) {
        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status)) {
            unsigned char battery = status.BatteryLifePercent;
            /* battery := 0..100 or 255 if unknown */
            if (battery != 255) {
                if (status.ACLineStatus == 1) {
                    swprintf(batterystr, sizeof(batterystr), L" | C %u%%", battery);
                } else {
                    swprintf(batterystr, sizeof(batterystr), L" | D %u%%", battery);
                }
            }
        }
    }
  
    // concatenate all the parts to create the final output string
    wcscpy(out, timestr);
    wcscat(out, batterystr);
    dc.w = TEXTW(out);
    dc.x = ww - dc.w;
    drawtext(out, dc.norm, false);

    if ((dc.w = dc.x - x) > bh) {
        dc.x = x;
        if (sel) {
            drawtext(getclienttitle(sel->hwnd), dc.sel, false);
            drawsquare(sel->isfixed, sel->isfloating, false, dc.sel);
        }
        else
            drawtext(NULL, dc.norm, false);
    }

    ReleaseDC(barhwnd, dc.hdc);
}

void
drawsquare(bool filled, bool empty, bool invert, unsigned long col[ColLast]) {
    static int size = 5;
    RECT r = { .left = dc.x + 1, .top = dc.y + 1, .right = dc.x + size, .bottom = dc.y + size };

    HBRUSH brush = CreateSolidBrush(col[invert ? ColBG : ColFG]);
    SelectObject(dc.hdc, brush);

    if (filled) {
        FillRect(dc.hdc, &r, brush);
    } else if (empty) {
        FillRect(dc.hdc, &r, brush);
    }
    DeleteObject(brush);
}

void
drawtext(const wchar_t *text, unsigned long col[ColLast], bool invert) {
    RECT r = { .left = dc.x, .top = dc.y, .right = dc.x + dc.w, .bottom = dc.y + dc.h };

    HPEN pen = CreatePen(PS_SOLID, borderpx, selbordercolor);
    HBRUSH brush = CreateSolidBrush(col[invert ? ColFG : ColBG]);
    SelectObject(dc.hdc, pen);
    SelectObject(dc.hdc, brush);
    FillRect(dc.hdc, &r, brush);

    DeleteObject(brush);
    DeleteObject(pen);

    SetBkMode(dc.hdc, TRANSPARENT);
    SetTextColor(dc.hdc, col[invert ? ColBG : ColFG]);

    if (!font) {
        font = CreateFontW(fontsize, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, fontname);
        if (!font)
            font = (HFONT)GetStockObject(SYSTEM_FONT);
        }
    SelectObject(dc.hdc, font);

    DrawTextW(dc.hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void
eprint(bool premortem, const wchar_t *errstr, ...) {
    va_list ap;
    int num_of_chars;
    wchar_t* buffer = NULL;
    size_t buffer_num_of_chars;
    wchar_t program_name[] = L"dwm-win32: ";

    va_start(ap, errstr);

    num_of_chars = _vscwprintf(errstr, ap);
    if (num_of_chars == -1) {
        OutputDebugStringW(L"_vscwprintf failed in eprint");
        goto cleanup;
    }

    buffer_num_of_chars = wcslen(program_name) + num_of_chars + 1;
    buffer = (wchar_t*)calloc(buffer_num_of_chars, sizeof(wchar_t));
    if (buffer == NULL) {
        OutputDebugStringW(L"calloc failed in eprint");
        goto cleanup;
    }

    if (wcscpy_s(buffer, buffer_num_of_chars, program_name) != 0) {
        OutputDebugStringW(L"wcscpy_s failed in eprint");
        goto cleanup;
    }

    if (vswprintf(buffer + wcslen(program_name), num_of_chars + 1, errstr, ap) < 0) {
        OutputDebugStringW(L"vswprintf failed in eprint");
        goto cleanup;
    }

    OutputDebugStringW(buffer);

    if (premortem)
        MessageBoxW(NULL, buffer, L"dwm-win32 has encountered an error", MB_ICONERROR | MB_SETFOREGROUND | MB_OK);

cleanup:
    if (buffer != NULL)
        free(buffer);
    
    va_end(ap);
}

void
setselected(Client *c) {
    if (!c || !ISVISIBLE(c))
        for (c = stack; c && !ISVISIBLE(c); c = c->snext);
    if (sel && sel != c)
        drawborder(sel, normbordercolor);
    if (c) {
        if (c->isurgent)
            clearurgent(c);
        detachstack(c);
        attachstack(c);
        drawborder(c, selbordercolor);
    }
    sel = c;
    drawbar();
}

void
focus(Client *c) {
    setselected(c);
    if (sel){
    	   //Artificially presses VK_F13 key, won't change focus if we dont do that (better performance) bert1337/dwm-win32/commit/6e331064f5db68ff3b7a2b2a38cdd3810e94d38a
        keybd_event(VK_F13, 0, 0, 0);
        keybd_event(VK_F13, 0, 0x0002, 0);
        SetForegroundWindow(sel->hwnd);
    }
}

void
focusstack(const Arg *arg) {
    Client *c = NULL, *i;

    if (!sel)
        return;
    if (arg->i > 0) {
        for (c = sel->next; c && !ISFOCUSABLE(c); c = c->next);
        if (!c)
            for (c = clients; c && !ISFOCUSABLE(c); c = c->next);
    }
    else {
        for (i = clients; i != sel; i = i->next)
            if (ISFOCUSABLE(i))
                c = i;
        if (!c)
            for (; i; i = i->next)
                if (ISFOCUSABLE(i))
                    c = i;
    }
    if (c) {
        focus(c);
        restack();
    }
}

Client *
managechildwindows(Client *p) {
    Client *c, *t;
    EnumChildWindows(p->hwnd, scan, 0);
    /* remove all child windows which were not part
     * of the enumeration above.
     */
    for (c = clients; c; ) {
        if (c->parent == p->hwnd) {
            /* XXX: ismanageable isn't that reliable or some
             *      windows change over time which means they
             *      were once reported as manageable but not
             *      this time so we also check if they are
             *      currently visible and if that's the case
             *      we keep them in our client list.
             */
            if (!c->isalive && !IsWindowVisible(c->hwnd)) {
                t = c->next;
                unmanage(c);
                c = t;
                continue;
            }

            /* reset flag for next check */
            c->isalive = false;
        }
        c = c->next;
    }

    return nextchild(p, clients);
}

Client *
getclient(HWND hwnd) {
    Client *c;

    for (c = clients; c; c = c->next)
        if (c->hwnd == hwnd)
            return c;
    return NULL;
}

LPWSTR
getclientclassname(HWND hwnd) {
    static wchar_t buf[500];
    GetClassNameW(hwnd, buf, sizeof buf);
    return buf;
}

LPWSTR
getclienttitle(HWND hwnd) {
    static wchar_t buf[500];
    GetWindowTextW(hwnd, buf, sizeof buf);
    return buf;
}

LPWSTR
getclientprocessname(HWND hwnd) {
    DWORD processid = 0;
    DWORD buf_size = MAX_PATH;
    static wchar_t buf[MAX_PATH];
    GetWindowThreadProcessId(hwnd, &processid);
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processid);
    if (hProc) {
        if (QueryFullProcessImageNameW(hProc, 0, buf, &buf_size)) {
            CloseHandle(hProc);
            return buf;
        } else {
            CloseHandle(hProc);
        }
    }
    return NULL;
}


HWND
getroot(HWND hwnd) {
    HWND parent, deskwnd = GetDesktopWindow();

    while ((parent = GetWindow(hwnd, GW_OWNER)) != NULL && deskwnd != parent)
        hwnd = parent;

    return hwnd;
}

void
grabkeys(HWND hwnd) {
    int i;
    for (i = 0; i < LENGTH(keys); i++) {
        RegisterHotKey(hwnd, i, keys[i].mod, keys[i].key);
    }
}

bool
iscloaked(HWND hwnd) {
    int cloaked_val;
    HRESULT h_res = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked_val, sizeof(cloaked_val));

    if (h_res != S_OK)
        cloaked_val = 0;

    return cloaked_val ? true : false;
}

bool
ismanageable(HWND hwnd) {
    if (hwnd == 0)
        return false;

    if (getclient(hwnd))
        return true;

    HWND parent = GetParent(hwnd);    
    int style = GetWindowLong(hwnd, GWL_STYLE);
    int exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    bool pok = (parent != 0 && ismanageable(parent));
    bool istool = exstyle & WS_EX_TOOLWINDOW;
    bool isapp = exstyle & WS_EX_APPWINDOW;
    bool noactiviate = exstyle & WS_EX_NOACTIVATE;
    const wchar_t *classname = getclientclassname(hwnd);
    const wchar_t *title = getclienttitle(hwnd);

    if (pok && !getclient(parent))
        manage(parent);

    debug(L"ismanageable: %s\n", getclienttitle(hwnd));
    debug(L"    hwnd: %d\n", hwnd);
    debug(L"  window: %d\n", IsWindow(hwnd));
    debug(L" visible: %d\n", IsWindowVisible(hwnd));
    debug(L"  parent: %d\n", parent);
    debug(L"parentok: %d\n", pok);
    debug(L"   owner: %d\n", GetWindow(hwnd, GW_OWNER));
    debug(L" toolwin: %d\n", istool);
    debug(L"  appwin: %d\n", isapp);
    debug(L"noactivate: %d\n", noactiviate);

    /* XXX: should we do this? */
    if (GetWindowTextLength(hwnd) == 0) {
        debug(L"   title: NULL\n");
        debug(L"  manage: false\n");
        return false;
    }

    if (style & WS_DISABLED) {
        debug(L"disabled: true\n");
        debug(L"  manage: false\n");
        return false;
    }

    if (noactiviate)
        return false;

    /* This is to avoid managing inactive suspended windows 10 modern apps */
    if (iscloaked(hwnd))
        return false;

    if (wcsstr(classname, L"Windows.UI.Core.CoreWindow") && (
        wcsstr(title, L"Windows Shell Experience Host") ||
        wcsstr(title, L"Microsoft Text Input Application") ||
        wcsstr(title, L"Action center") ||
        wcsstr(title, L"New Notification") ||
        wcsstr(title, L"Date and Time Information") ||
        wcsstr(title, L"Volume Control") ||
        wcsstr(title, L"Network Connections") ||
        wcsstr(title, L"Cortana") ||
        wcsstr(title, L"Start") ||
        wcsstr(title, L"Windows Default Lock Screen") ||
        wcsstr(title, L"Search"))) {
        return false;
    }

    if (wcsstr(classname, L"ForegroundStaging") ||
        wcsstr(classname, L"ApplicationManager_DesktopShellWindow") ||
        wcsstr(classname, L"Static") ||
        wcsstr(classname, L"Scrollbar") ||
        wcsstr(classname, L"Progman")) {
        return false;
    }

    /*
     *    WS_EX_APPWINDOW
     *        Forces a top-level window onto the taskbar when 
     *        the window is visible.
     *
     *    WS_EX_TOOLWINDOW
     *        Creates a tool window; that is, a window intended 
     *        to be used as a floating toolbar. A tool window 
     *        has a title bar that is shorter than a normal 
     *        title bar, and the window title is drawn using 
     *        a smaller font. A tool window does not appear in 
     *        the taskbar or in the dialog that appears when 
     *        the user presses ALT+TAB. If a tool window has 
     *        a system menu, its icon is not displayed on the 
     *        title bar. However, you can display the system 
     *        menu by right-clicking or by typing ALT+SPACE.
     */

    if ((parent == 0 && IsWindowVisible(hwnd)) || pok) {
        if ((!istool && parent == 0) || (istool && pok)) {
            debug(L"  manage: true\n");
            return true;
        }
        if (isapp && parent != 0) {
            debug(L"  manage: true\n");
            return true;
        }
    }
    debug(L"  manage: false\n");
    return false;
}

void
killclient(const Arg *arg) {
    if (!sel)
        return;
    PostMessage(sel->hwnd, WM_CLOSE, 0, 0);
}

Client*
manage(HWND hwnd) {    
    Client *c = getclient(hwnd);

    if (c)
        return c;

    debug(L" manage %s\n", getclienttitle(hwnd));

    WINDOWINFO wi = {
        .cbSize = sizeof(WINDOWINFO),
    };

    if (!GetWindowInfo(hwnd, &wi))
        return NULL;

    if (!(c = calloc(1, sizeof(Client))))
        die(L"fatal: could not calloc() %u bytes for new client\n", sizeof(Client));

    c->hwnd = hwnd;
    c->parent = GetParent(hwnd);
    c->root = getroot(hwnd);
    c->isalive = true;
    c->iscloaked = iscloaked(hwnd);
    c->isminimized = IsIconic(hwnd);

    static WINDOWPLACEMENT wp = {
        .length = sizeof(WINDOWPLACEMENT),
        .showCmd = SW_RESTORE,
    };

    if (IsWindowVisible(hwnd) && !c->isminimized)
        SetWindowPlacement(hwnd, &wp);
    
    c->isfloating = (!(wi.dwStyle & WS_MINIMIZEBOX) && !(wi.dwStyle & WS_MAXIMIZEBOX));

    c->ignoreborder = iscloaked(hwnd);

    debug(L" window style: %d\n", wi.dwStyle);
    debug(L"     minimize: %d\n", wi.dwStyle & WS_MINIMIZEBOX);
    debug(L"     maximize: %d\n", wi.dwStyle & WS_MAXIMIZEBOX);
    debug(L"        popup: %d\n", wi.dwStyle & WS_POPUP);
    debug(L"   isfloating: %d\n", c->isfloating);

    applyrules(c);

    if (!c->isfloating)
        setborder(c, false);
        

    if (c->isfloating && IsWindowVisible(hwnd) && !c->isminimized) {
        debug(L" new floating window: x: %d y: %d w: %d h: %d\n", wi.rcWindow.left, wi.rcWindow.top, wi.rcWindow.right - wi.rcWindow.left, wi.rcWindow.bottom - wi.rcWindow.top);
        resize(c, wi.rcWindow.left, wi.rcWindow.top, wi.rcWindow.right - wi.rcWindow.left, wi.rcWindow.bottom - wi.rcWindow.top);
    }

    attach(c);
    attachstack(c);
    return c;
}

void
monocle(void) {
    Client *c;

    for (c = nexttiled(clients); c; c = nexttiled(c->next)) {
        resize(c, wx, wy, ww - 2 * c->bw, wh - 2 * c->bw);
    }
}

Client *
nextchild(Client *p, Client *c) {
    for (; c && c->parent != p->hwnd; c = c->next);
    return c;
}

Client *
nexttiled(Client *c) {
    for (; c && (c->isfloating || c->isminimized || !ISVISIBLE(c)); c = c->next);
    return c;
}

void
quit(const Arg *arg) {
    PostMessage(dwmhwnd, WM_CLOSE, 0, 0);
}

void
resize(Client *c, int x, int y, int w, int h) {
    if (w <= 0 && h <= 0) {
        setvisibility(c->hwnd, false);
        return;
    }
    if (x > sx + sw)
        x = sw - WIDTH(c);
    if (y > sy + sh)
        y = sh - HEIGHT(c);
    if (x + w + 2 * c->bw < sx)
        x = sx;
    if (y + h + 2 * c->bw < sy)
        y = sy;
    if (h < bh)
        h = bh;
    if (w < bh)
        w = bh;
    if (c->x != x || c->y != y || c->w != w || c->h != h) {
        c->x = x;
        c->y = y;
        c->w = w;
        c->h = h;
        debug(L" resize %d: %s: x: %d y: %d w: %d h: %d\n", c->hwnd, getclienttitle(c->hwnd), x, y, w, h);
        SetWindowPos(c->hwnd, HWND_TOP, c->x - 7, c->y, c->w + 14, c->h + 7, SWP_NOACTIVATE);
    }
}

void
restack(void) {

}

LRESULT CALLBACK barhandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            updatebar();
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            drawbar();
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            buttonpress(msg, &MAKEPOINTS(lParam));
            break;
        case WM_TIMER:
            drawbar();
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam); 
    }

    return 0;
}


int i = 0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            break;
        case WM_CLOSE:
            cleanup(NULL);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_HOTKEY:
            if (wParam > 0 && wParam < LENGTH(keys)) {
                keys[wParam].func(&(keys[wParam ].arg));
            }
            break;
        case WM_DISPLAYCHANGE:
            updategeom();
            updatebar();
            arrange();
            break;
        default:
            if (msg == shellhookid) { /* Handle the shell hook message */
                Client *c = getclient((HWND)lParam);
                switch (wParam & 0x7fff) {
                    /* The first two events are also trigger if windows
                     * are being hidden or shown because of a tag
                     * switch, therefore we ignore them in this case.
                     */
                    case HSHELL_WINDOWCREATED:
                        debug(L"window created: %s\n", getclienttitle((HWND)lParam));
                        if (!c && ismanageable((HWND)lParam)) {
                            c = manage((HWND)lParam);
                            managechildwindows(c);
                            arrange();
                        }
                        break;
                    case HSHELL_WINDOWDESTROYED:
                        if (c) {
                            debug(L" window %s: %s\n", c->ignore ? L"hidden" : L"destroyed", getclienttitle(c->hwnd));
                            if (!c->ignore)
                                unmanage(c);
                            else
                                c->ignore = false;
                        } else {
                            debug(L" unmanaged window destroyed\n");
                        }
                        break;
                    case HSHELL_WINDOWACTIVATED:
                        debug(L" window activated: %s || %d\n", c ? getclienttitle(c->hwnd) : L"unknown", (HWND)lParam);
                        if (c) {
                            Client *t = sel;
                            managechildwindows(c);
                            setselected(c);
                            /* check if the previously selected 
                             * window got minimized
                             */
                            if (t && (t->isminimized = IsIconic(t->hwnd))) {
                                debug(L" active window got minimized: %s\n", getclienttitle(t->hwnd));
                                arrange();
                            }
                            /* the newly focused window was minimized */
                            if (sel && sel->isminimized) {
                                debug(L" newly active window was minimized: %s\n", getclienttitle(sel->hwnd));
                                sel->isminimized = false;                                
                                zoom(NULL);
                            }
                        } else  {
                            /* Some window don't seem to generate 
                             * HSHELL_WINDOWCREATED messages therefore 
                              * we check here whether we should manage
                              * the window or not.
                              */
                            if (ismanageable((HWND)lParam)) {
                                c = manage((HWND)lParam);
                                managechildwindows(c);
                                setselected(c);
                                arrange();
                            }
                        }
                        break;
                }
            } else
                return DefWindowProc(hwnd, msg, wParam, lParam); 
    }

    return 0;
}

void CALLBACK
wineventproc(HWINEVENTHOOK heventhook, DWORD event, HWND hwnd, LONG object, LONG child, DWORD eventthread, DWORD eventtime_ms) {
    if (object != OBJID_WINDOW || child != CHILDID_SELF || hwnd == NULL)
        return;

    Client *c = getclient(hwnd);

    switch (event)
    {
    case EVENT_OBJECT_UNCLOAKED:
        if (!c && ismanageable(hwnd)) {
            c = manage(hwnd);
            managechildwindows(c);
            setselected(c);
            arrange();
        }

        break;

    case EVENT_OBJECT_CLOAKED:
        if (c)
            unmanage(c);

        break;
    }
}

BOOL CALLBACK 
scan(HWND hwnd, LPARAM lParam) {
    Client *c = getclient(hwnd);
    if (c)
        c->isalive = true;
    else if (ismanageable(hwnd))
        manage(hwnd);

    return TRUE;
}

void
drawborder(Client *c, COLORREF color) {
#if 0
    HDC hdc = GetWindowDC(c->hwnd);

#if 0
    /* this would be another way, but it uses standard sytem colors */
    RECT area = { .left = 0, .top = 0, .right = c->w, .bottom = c->h };
    DrawEdge(hdc, &area, BDR_RAISEDOUTER | BDR_SUNKENINNER, BF_RECT);
#else
    HPEN pen = CreatePen(PS_SOLID, borderpx, color);
    SelectObject(hdc, pen);
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, c->w, 0);
    LineTo(hdc, c->w, c->h);
    LineTo(hdc, 0, c->h);
    LineTo(hdc, 0, 0);
    DeleteObject(pen);
#endif

    ReleaseDC(c->hwnd, hdc);
#endif
}

void
setborder(Client *c, bool border) {
    if (!c->ignoreborder) {
        if (border) {
            SetWindowLong(c->hwnd, GWL_STYLE, (GetWindowLong(c->hwnd, GWL_STYLE) | (WS_CAPTION | WS_SIZEBOX)));
        } else {
            /* XXX: ideally i would like to use the standard window border facilities and just modify the 
             *      color with SetSysColor but this only seems to work if we leave WS_SIZEBOX enabled which
             *      is not optimal.
             */
            SetWindowLong(c->hwnd, GWL_STYLE, (GetWindowLong(c->hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_SIZEBOX)) | WS_BORDER | WS_THICKFRAME);
            SetWindowLong(c->hwnd, GWL_EXSTYLE, (GetWindowLong(c->hwnd, GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE)));
        }
        SetWindowPos(c->hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
        c->border = border;
    }
}

void
setvisibility(HWND hwnd, bool visibility) {
    SetWindowPos(hwnd, 0, 0, 0, 0, 0, (visibility ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void
setlayout(const Arg *arg) {
    if (!arg || !arg->v || arg->v != lt[sellt])
        sellt ^= 1;
    if (arg && arg->v)
	lt[sellt] = lts[curtag] = (Layout *)arg->v;
    if (sel)
        arrange();
    else
        drawbar();
}

/* arg > 1.0 will set mfact absolutly */
void
setmfact(const Arg *arg) {
    float f;

    if (!arg || !lt[sellt]->arrange)
        return;
    f = arg->f < 1.0 ? arg->f + mfact : arg->f - 1.0;
    if (f < 0.1 || f > 0.9)
        return;
    mfact = mfacts[curtag] = f;
    arrange();
}

int
luaopen_dwmlibs(lua_State *L, DwmState* state) {
	luaL_openlibs(L);

#ifndef LUAJIT
	luaopen_bit(L);
#endif

	luaL_requiref(L, "dwm", luaopen_dwm, 0);
	dwm_setstate(L, state);
	luaL_requiref(L, "dwm.eventemitter", luaopen_dwm_eventemitter, 0);
	luaL_requiref(L, "dwm.display", luaopen_dwm_display, 0);
	luaL_requiref(L, "dwm.client", luaopen_dwm_client, 0);
	luaL_requiref(L, "dwm.hotkey", luaopen_dwm_hotkey, 0);

	return 1;
}

void load_user_script(lua_State *L) {
	(void) luaL_dostring(L,
		"local dwm = require 'dwm'\n"
		"local dwm_config_dir = (os.getenv('XDG_CONFIG_HOME') or os.getenv('HOME') or os.getenv('USERPROFILE')) .. '/.config/dwm'\n"
		"local init_script = dwm_config_dir .. '/init.lua'\n"
		"local f = io.open(init_script, 'r')\n"
		"if f then\n"
		"  f:close()\n"
		"  dofile(init_script)\n"
		"end\n"
		"\n");
}

static int lua_panic_handler(lua_State *L) {
	void *ud = NULL;
	lua_getallocf(L, &ud);
	if (ud) {
		const char *msg = NULL;
		if (lua_type(L, -1) == LUA_TSTRING)
			msg = lua_tostring(L, -1);
        die(msg);
	}
	return 0;
}

void
setup(lua_State *L, HINSTANCE hInstance) {
	lua_atpanic(L, &lua_panic_handler);
	DwmState *state = (DwmState*)malloc(sizeof(DwmState));
	state->hInstance = hInstance;
	luaopen_dwmlibs(L, state);
	load_user_script(L);

    unsigned int i;

    lt[0] = &layouts[0];
    lt[1] = &layouts[1 % LENGTH(layouts)];

    /* init appearance */

    dc.norm[ColBorder] = normbordercolor;
    dc.norm[ColBG] = normbgcolor;
    dc.norm[ColFG] = normfgcolor;
    dc.sel[ColBorder] = selbordercolor;
    dc.sel[ColBG] = selbgcolor;
    dc.sel[ColFG] = selfgcolor;

    /* save colors so we can restore them in cleanup */
    for (i = 0; i < LENGTH(colorwinelements); i++)
        colors[0][i] = GetSysColor(colorwinelements[i]);
    
    SetSysColors(LENGTH(colorwinelements), colorwinelements, colors[1]); 
    
    HWND hwnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hwnd)
        setvisibility(hwnd, showexploreronstart);

    WNDCLASSEXW winClass;

    winClass.cbSize = sizeof(WNDCLASSEXW);
    winClass.style = 0;
    winClass.lpfnWndProc = WndProc;
    winClass.cbClsExtra = 0;
    winClass.cbWndExtra = 0;
    winClass.hInstance = hInstance;
    winClass.hIcon = NULL;
    winClass.hIconSm = NULL;
    winClass.hCursor = NULL;
    winClass.hbrBackground = NULL;
    winClass.lpszMenuName = NULL;
    winClass.lpszClassName = NAME;

    if (!RegisterClassExW(&winClass))
        die(L"Error registering window class");

    dwmhwnd = CreateWindowExW(0, NAME, NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    if (!dwmhwnd)
        die(L"Error creating window");

    updategeom();

    EnumWindows(scan, 0);

    /* init mfacts */
    for(i=0; i < LENGTH(tags) + 1 ; i++) {
	    mfacts[i] = mfact;
    }

    /* init layouts */
    for(i=0; i < LENGTH(tags) + 1; i++) {
	    lts[i] = &layouts[0];
    }

    setupbar(hInstance);

    for(i=0; i < LENGTH(tags) + 1; i++) {
	    showbars[i] = showbar;
    }

    grabkeys(dwmhwnd);

    arrange();
    
    if (!RegisterShellHookWindow(dwmhwnd))
        die(L"Could not RegisterShellHookWindow");

    /* Grab a dynamic id for the SHELLHOOK message to be used later */
    shellhookid = RegisterWindowMessageW(L"SHELLHOOK");

    wineventhook = SetWinEventHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED, NULL, wineventproc, 0, 0, WINEVENT_OUTOFCONTEXT);

    if (!wineventhook)
        die(L"Could not SetWinEventHook");

    updatebar();

    focus(NULL);
}

void
setupbar(HINSTANCE hInstance) {

    unsigned int i, w = 0;

    WNDCLASSW winClass;
    memset(&winClass, 0, sizeof winClass);

    winClass.style = 0;
    winClass.lpfnWndProc = barhandler;
    winClass.cbClsExtra = 0;
    winClass.cbWndExtra = 0;
    winClass.hInstance = hInstance;
    winClass.hIcon = NULL;
    winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = NULL;
    winClass.lpszMenuName = NULL;
    winClass.lpszClassName = L"dwm-bar";

    if (!RegisterClassW(&winClass))
        die(L"Error registering window class");

    barhwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"dwm-bar",
        NULL, /* window title */
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
        0, 0, 0, 0, 
        NULL, /* parent window */
        NULL, /* menu */
        hInstance,
        NULL
    );

    /* calculate width of the largest layout symbol */
    dc.hdc = GetWindowDC(barhwnd);
    HFONT font = (HFONT)GetStockObject(SYSTEM_FONT); 
    SelectObject(dc.hdc, font);

    for (blw = i = 0; LENGTH(layouts) > 1 && i < LENGTH(layouts); i++) {
         w = TEXTW(layouts[i].symbol);
        blw = MAX(blw, w);
    }

    ReleaseDC(barhwnd, dc.hdc);

    PostMessage(barhwnd, WM_PAINT, 0, 0);

    SetTimer(barhwnd, 1, clock_interval, NULL);

    updatebar();
}

void
showclientinfo(const Arg *arg) {
    HWND hwnd = GetForegroundWindow();
    wchar_t buffer[5000];
    swprintf(buffer, sizeof(buffer), L"ClassName:  %s\nTitle:  %s\nProcessName: %s", getclientclassname(hwnd), getclienttitle(hwnd), getclientprocessname(hwnd));
    MessageBoxW(NULL, buffer, L"dwm-win32 debug", MB_OK);
}

void
showhide(Client *c) {
    if (!c)
        return;
    /* XXX: is the order of showing / hidding important? */
    if (!ISVISIBLE(c)) {
        if (IsWindowVisible(c->hwnd)) {
            c->ignore = true;
            c->wasvisible = true;
            setvisibility(c->hwnd, false);
        }
    } else {
        if (c->wasvisible) {
            setvisibility(c->hwnd, true);
        }
    }
    showhide(c->snext);
}

void
spawn(const Arg *arg) {
    ShellExecuteW(NULL, NULL, ((wchar_t **)arg->v)[0], ((wchar_t **)arg->v)[1], NULL, SW_SHOWDEFAULT);
}

void
tag(const Arg *arg) {
    Client *c;

    if (sel && arg->ui & TAGMASK) {
        sel->tags = arg->ui & TAGMASK;
        debug(L"window tagged: %d %s\n", sel->hwnd, getclienttitle(sel->hwnd));
        for (c = managechildwindows(sel); c; c = nextchild(sel, c->next)) {
            debug(L" child window which is %s tagged: %s\n", c->isfloating ? L"floating" : L"normal", getclienttitle(c->hwnd));
            if (c->isfloating)
                c->tags = arg->ui & TAGMASK;
        }
        debug(L"window tagged finished\n");
        arrange();
    }
}

int
textnw(const wchar_t *text, unsigned int len) {
    SIZE size;
    GetTextExtentPoint32W(dc.hdc, text, len, &size);
    if (size.cx > 0)
        size.cx += textmargin;
    return size.cx;  
}


void
tile(void) {
    int x, y, h, w, mw;
    unsigned int i, n;
    Client *c;

    for (n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), n++);
    if (n == 0)
        return;

    /* master */
    c = nexttiled(clients);
    mw = mfact * ww;
    resize(c, wx, wy, (n == 1 ? ww : mw) - 2 * c->bw, wh - 2 * c->bw);

    if (--n == 0)
        return;

    /* tile stack */
    x = (wx + mw > c->x + c->w) ? c->x + c->w + 2 * c->bw : wx + mw;
    y = wy;
    w = (wx + mw > c->x + c->w) ? wx + ww - x : ww - mw;
    h = wh / n;
    if (h < bh)
        h = wh;

    for (i = 0, c = nexttiled(c->next); c; c = nexttiled(c->next), i++) {
        resize(c, x, y, w - 2 * c->bw, /* remainder */ ((i + 1 == n)
               ? wy + wh - y - 2 * c->bw : h - 2 * c->bw));
        if (h != wh)
            y = c->y + HEIGHT(c);
    }
}

void
togglebar(const Arg *arg) {
    showbar = showbars[curtag] = !showbar;
    updategeom();
    updatebar();
    arrange();
}

void
toggleborder(const Arg *arg) {
    if (!sel)
        return;
    setborder(sel, !sel->border);
}

void
toggleexplorer(const Arg *arg) {
    HWND hwnd = FindWindowW(L"Progman", L"Program Manager");
    if (hwnd)
        setvisibility(hwnd, !IsWindowVisible(hwnd));

    hwnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hwnd)
        setvisibility(hwnd, !IsWindowVisible(hwnd));

    updategeom();
    updatebar();
    arrange();        
}


void
togglefloating(const Arg *arg) {
    if (!sel)
        return;
    sel->isfloating = !sel->isfloating || sel->isfixed;
    setborder(sel, sel->isfloating);
    if (sel->isfloating)
        resize(sel, sel->x, sel->y, sel->w, sel->h);
    arrange();
}

void
toggletag(const Arg *arg) {
    unsigned int mask;

    if (!sel)
        return;
    
    mask = sel->tags ^ (arg->ui & TAGMASK);
    if (mask) {
        sel->tags = mask;
        arrange();
    }
}

void
toggleview(const Arg *arg) {
    unsigned int mask = tagset[seltags] ^ (arg->ui & TAGMASK);

    if (mask) {
	    if(mask == ~0) {
		    prevtag = curtag;
		    curtag = 0;
	    }
	    if(!(mask & 1 << (curtag - 1))) {
		    prevtag = curtag;
		    for (i=0; !(mask & 1 << i); i++);
		    curtag = i + 1;
	    }
        tagset[seltags] = mask;
	lt[sellt] = lts[curtag];
	mfact = mfacts[curtag];
	if (showbar != showbars[curtag])
		togglebar(NULL);
        arrange();
    }
}

void
writelog(const Arg *arg) {
    Client *c;
    FILE *fout;
    fout = fopen("dwm-win32.log", "w");
    if (fout == NULL) return;

    fprintf(fout, "hwnd, parent, tag, visible, classname, title\n");

    for (c = clients; c; c = c->next) {
        fprintf(fout, "%d,", c->hwnd);
        fprintf(fout, "%d,", c->parent == NULL ? 0 : c->parent);
        fprintf(fout, "%d,", c->tags);
        fprintf(fout, "%d,", IsWindowVisible(c->hwnd));
        fwprintf(fout, getclientclassname(c->hwnd));
        fprintf(fout, ",");
        fwprintf(fout, getclienttitle(c->hwnd));
        fprintf(fout, "\n");
    }

    fclose(fout);
}

void
unmanage(Client *c) {
    debug(L" unmanage %s\n", getclienttitle(c->hwnd));
    if (c->wasvisible)
        setvisibility(c->hwnd, true);
    if (!c->isfloating)
        setborder(c, true);
    detach(c);
    detachstack(c);
    if (sel == c)
        focus(NULL);
    free(c);
    arrange();
}

void
updatebar(void) {
    SetWindowPos(barhwnd, showbar ? HWND_TOPMOST : HWND_NOTOPMOST, bx, by, ww, bh, (showbar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

void
updategeom(void) {
    RECT wa;
    HWND hwnd = FindWindowW(L"Shell_TrayWnd", NULL);
    /* check if the windows taskbar is visible and adjust
     * the workspace accordingly.
     */
    if (hwnd && IsWindowVisible(hwnd)) {    
        SystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0);
        sx = wa.left;
        sy = wa.top;
        sw = wa.right - wa.left;
        sh = wa.bottom - wa.top;
    } else {
        sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }

    bx = sx;
    bh = 20; /* XXX: fixed value */

    /* window area geometry */
    wx = sx;
    wy = showbar && topbar ? sy + bh : sy;
    ww = sw;
    wh = showbar ? sh - bh : sh;
    /* bar position */
    by = showbar ? (topbar ? wy - bh : wy + wh) : -bh;
    debug(L"updategeom: %d x %d\n", ww, wh);
}

void
view(const Arg *arg) {
    unsigned int i;
    if ((arg->ui & TAGMASK) == tagset[seltags])
        return;
    seltags ^= 1; /* toggle sel tagset */
    if(arg->ui & TAGMASK) {
	    tagset[seltags] = arg->ui & TAGMASK;
	    prevtag = curtag;
	    if(arg->ui == ~0)
		    curtag = 0;
	    else {
		    for (i=0; !(arg->ui & 1 << i); i++);
		    curtag = i + 1;
	    }
    } else {
	    prevtag= curtag ^ prevtag;
	    curtag^= prevtag;
	    prevtag= curtag ^ prevtag;
    }
    lt[sellt]= lts[curtag];
    mfact = mfacts[curtag];
    if(showbar != showbars[curtag])
	    togglebar(NULL);
    arrange();
}

void
zoom(const Arg *arg) {
    Client *c = sel;

    if (!lt[sellt]->arrange || lt[sellt]->arrange == monocle || (sel && sel->isfloating))
        return;
    if (c == nexttiled(clients))
        if (!c || !(c = nexttiled(c->next)))
            return;
    detach(c);
    attach(c);
    focus(c);
    arrange();
}

void
movestack(const Arg *arg) {
	Client *c = NULL, *p = NULL, *pc = NULL, *i;

	if(arg->i > 0) {
		/* find the client after selmon->sel */
		for(c = sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
		if(!c)
			for(c = clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);

	}
	else {
		/* find the client before sel */
		for(i = clients; i != sel; i = i->next)
			if(ISVISIBLE(i) && !i->isfloating)
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i) && !i->isfloating)
					c = i;
	}
	/* find the client before sel and c */
	for(i = clients; i && (!p || !pc); i = i->next) {
		if(i->next == sel)
			p = i;
		if(i->next == c)
			pc = i;
	}

	/* swap c and sel clients in the clients list */
	if(c && c != sel) {
		Client *temp = sel->next==c?sel:sel->next;
		sel->next = c->next==sel?c:c->next;
		c->next = temp;

		if(p && p != c)
			p->next = c;
		if(pc && pc != sel)
			pc->next = sel;

		if(sel == clients)
			clients = c;
		else if(c == clients)
			clients = sel;

		arrange();
	}
}

void
forcearrange(const Arg *arg) {
    Client *c;
    for (c = clients; c; c = c->next) {
        c->x = 0;
        c->y = 0;
        c->h = 0;
        c->w = 0;
    }
    arrange();
}

// variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.
HHOOK _hook;

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

// This is the callback function. Consider it the event that is raised when, in this case,
// a key is pressed.
static bool ctrlPressed = false;
static bool ctrlAPressed = false;
char str[256];

LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{

    LPMSG msg = (LPMSG)lParam;

    if (nCode >= 0)
    {
        // the action is valid: HC_ACTION.
        if (wParam == WM_KEYDOWN)
        {
            // lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
            kbdStruct = *((KBDLLHOOKSTRUCT *)lParam);
            // a key (non-system) is pressed.
            //0x45 = Virtual Key della lettera E

            /* 
            *TODO: 
            * - Vedere qual'è o quali sono i mod da config.h (bisogna importarla)
            * - ciclare poi tutte le keys (guardando solo alle VK e non alle MOD)
            * - Dentro il for fare l'if che c'è qui sotto lanciando anche la funzione
            * - PROFIT ???
            */
            int i;
            //4294934529
            bool win = GetAsyncKeyState(VK_LWIN) != 0;
            bool ctrl = GetAsyncKeyState(VK_LCONTROL) != 0;
            bool alt = GetAsyncKeyState(VK_LMENU) != 0;
            bool shift = GetAsyncKeyState(VK_LSHIFT) != 0;

            for (i = 0; i < LENGTH(keys); i++)
            {
                //RegisterHotKey(hwnd, i, keys[i].mod, keys[i].key);
                if (kbdStruct.vkCode == (int)keys[i].key)
                {
                    int k = 0;
                    int a = ((keys[i].mod & MOD_ALT) & (1 << k)) >> k; //Controllo che MOD_ALT Sia hotkey
                    k = 1;
                    int c = ((keys[i].mod & MOD_CONTROL) & (1 << k)) >> k; //Controllo che MOD_CONTROL Sia hotkey
                    k = 2;
                    int s = ((keys[i].mod & MOD_SHIFT) & (1 << k)) >> k; //Controllo che MOD_SHIFT Sia hotkey
                    k = 3;
                    int w = ((keys[i].mod & MOD_WIN) & (1 << k)) >> k; //Controllo che MOD_WIN Sia hotkey

                    if ((w == win) &&
                        (a == alt) &&
                        (c == ctrl) &&
                        (s == shift))
                    {
                        keys[i].func(&(keys[i].arg));
                        return 1;
                    }
                }
            }
        }
    }
    // call the next hook in the hook chain. This is nessecary or your hook chain will break and the hook stops
    return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
    // Set the hook and set it to use the callback function above
    // WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
    // The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
    // function that sets and releases the hook. If you create a hack you will not need the callback function
    // in another place then your own code file anyway. Read more about it at MSDN.
    //if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        MessageBox(NULL, "Failed to install hook!", "Error", MB_ICONERROR);
    }
}

void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
	SetProcessDPIAware();

	SetHook();
	
    MSG msg;

    HANDLE mutex = CreateMutexW(NULL, TRUE, NAME);
    if (mutex == NULL)
        die(L"Failed to create dwm-win32 mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        die(L"dwm-win32 already running");

	lua_State *L = luaL_newstate();
    setup(L, hInstance);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanup(L);

    return msg.wParam;
}
