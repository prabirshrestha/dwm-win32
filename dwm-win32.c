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
 * To understand everything else, start reading main().
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT		0x0500

#include <windows.h>
#include <winuser.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>
#include <stdbool.h>

#define NAME			"dwm-win32" 	/* Used for window name/class */

/* macros */
#define ISVISIBLE(x)            (x->tags & tagset[seltags])
#define LENGTH(x)               (sizeof x / sizeof x[0])
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAXTAGLEN               16
#define WIDTH(x)                ((x)->w + 2 * (x)->bw)
#define HEIGHT(x)               ((x)->h + 2 * (x)->bw)
#define TAGMASK                 ((int)((1LL << LENGTH(tags)) - 1))
#define TEXTW(x)                (textnw(x, strlen(x)))
#ifdef NDEBUG
# define debug(format, args...)
#else
# define debug eprint
#endif

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast };		/* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };			/* color */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle };	/* clicks */

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
	int x, y, w, h;
	int bw; // XXX: useless?
	unsigned int tags;
	bool isminimized, isfixed, isfloating, isurgent, ignore, border;
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
	const char *symbol;
	void (*arrange)(void);
} Layout;

typedef struct {
	const char *class;
	const char *title;
	unsigned int tags;
	bool isfloating;
} Rule;

/* function declarations */
static void applyrules(Client *c);
static void arrange(void);
static void attach(Client *c);
static void attachstack(Client *c);
static void cleanup();
static void clearurgent(Client *c);
static void detach(Client *c);
static void detachstack(Client *c);
static void die(const char *errstr, ...);
static void drawbar(void);
static void drawsquare(bool filled, bool empty, bool invert, unsigned long col[ColLast]);
static void drawtext(const char *text, unsigned long col[ColLast], bool invert);
void drawborder(Client *c, COLORREF color);
void eprint(const char *errstr, ...);
static void focus(Client *c);
static void focusstack(const Arg *arg);
static Client *getclient(HWND hwnd);
LPSTR getclientclassname(HWND hwnd);
LPSTR getclienttitle(HWND hwnd);
static void grabkeys(HWND hwnd);
static void killclient(const Arg *arg);
static void monocle(void);
static Client *nexttiled(Client *c);
static void quit(const Arg *arg);
static void resize(Client *c, int x, int y, int w, int h);
static void restack(void);
static BOOL CALLBACK scan(HWND hwnd, LPARAM lParam);
static void setborder(Client *c, bool border);
static void setvisibility(HWND hwnd, bool visibility);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(HINSTANCE hInstance);
static void setupbar(HINSTANCE hInstance);
static void showclientclassname(const Arg *arg); 
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static int textnw(const char *text, unsigned int len);
static void tile(void);
static void togglebar(const Arg *arg);
static void toggleborder(const Arg *arg);
static void toggleexplorer(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unmanage(Client *c);
static void updatebar(void);
static void updategeom(void);
static void updatestatus(void);
static void view(const Arg *arg);
static void zoom(const Arg *arg);

/* Shell Hook stuff */
typedef BOOL (*RegisterShellHookWindowProc) (HWND);
RegisterShellHookWindowProc RegisterShellHookWindow;

/* XXX: should be in a system header, no? */
typedef struct {
    HWND    hwnd;
    RECT    rc;
} SHELLHOOKINFO, *LPSHELLHOOKINFO;

/* variables */
static HWND dwmhwnd, barhwnd;
static char stext[256];
static int sx, sy, sw, sh; /* X display screen geometry x, y, width, height */ 
static int by, bh, blw;    /* bar geometry y, height and layout symbol width */
static int wx, wy, ww, wh; /* window area geometry x, y, width, height, bar excluded */
static unsigned int seltags = 0, sellt = 0;

static Client *clients = NULL;
static Client *sel = NULL;
static Client *stack = NULL;
static Layout *lt[] = { NULL, NULL };
static UINT shellhookid;	/* Window Message id */

/* configuration, allows nested code to access above variables */
#include "config.h"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[sizeof(unsigned int) * 8 < LENGTH(tags) ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c) {
	unsigned int i;
	Rule *r;

	/* rule matching */
	for(i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if((!r->title || strstr(getclienttitle(c->hwnd), r->title))
		&& (!r->class || strstr(getclientclassname(c->hwnd), r->class))) {
			c->isfloating = r->isfloating;
			c->tags |= r->tags & TAGMASK ? r->tags & TAGMASK : tagset[seltags]; 
		}
	}
	if(!c->tags)
		c->tags = tagset[seltags];
}

void
arrange(void) {
	showhide(stack);
	focus(NULL);
	if(lt[sellt]->arrange)
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
	debug(" buttonpress: x:%d y:%d\n", point->x, point->y);
	do { x += TEXTW(tags[i]);
		debug("  %d: %d > %d\n", i, point->x, x);
	} while(point->x >= x && ++i < LENGTH(tags));
	if(i < LENGTH(tags)) {
		click = ClkTagBar;
		arg.ui = 1 << i;
	}
	else if(point->x < x + blw)
		click = ClkLtSymbol;
	else if(point->x > wx + ww - TEXTW(stext))
		click = ClkStatusText;
	else
		click = ClkWinTitle;

	if (GetKeyState(VK_SHIFT) < 0)
		return;

	for(i = 0; i < LENGTH(buttons); i++)
		// XXX: is GetKeyState really working?
		if(click == buttons[i].click && buttons[i].func && buttons[i].button == button
		   && (!buttons[i].key || GetKeyState(buttons[i].key) < 0))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
cleanup() {
	int i;
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	
	for(i = 0; i < LENGTH(keys); i++) {
		UnregisterHotKey(dwmhwnd, i);
	}

	DeregisterShellHookWindow(dwmhwnd);

	view(&a);
	lt[sellt] = &foo;
	while(stack)
		unmanage(stack);

	DestroyWindow(dwmhwnd);
}

void
clearurgent(Client *c) {

}

void
detach(Client *c) {
	Client **tc;

	for(tc = &clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c) {
	Client **tc;

	for(tc = &stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;
}

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	cleanup();
	exit(EXIT_FAILURE);
}

void
drawbar(void) {
	dc.hdc = GetWindowDC(barhwnd);

	dc.h = bh;

	int x;
	unsigned int i, occ = 0, urg = 0;
	unsigned long *col;
	Client *c;

	for(c = clients; c; c = c->next) {
		occ |= c->tags;
		if(c->isurgent)
			urg |= c->tags;
	}

	dc.x = 0;
	for(i = 0; i < LENGTH(tags); i++) {
		dc.w = TEXTW(tags[i]);
		col = tagset[seltags] & 1 << i ? dc.sel : dc.norm;
		drawtext(tags[i], col, urg & 1 << i);
		drawsquare(sel && sel->tags & 1 << i, occ & 1 << i, urg & 1 << i, col);
		dc.x += dc.w;
	}
	if(blw > 0) {
		dc.w = blw;
		drawtext(lt[sellt]->symbol, dc.norm, false);
		x = dc.x + dc.w;
	}
	else
		x = dc.x;
	dc.w = TEXTW(stext);
	dc.x = ww - dc.w;
	if(dc.x < x) {
		dc.x = x;
		dc.w = ww - x;
	}
	drawtext(stext, dc.norm, false);

	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		if(sel) {
			drawtext(getclienttitle(sel->hwnd), dc.sel, false);
			drawsquare(sel->isfixed, sel->isfloating, false, dc.sel);
		}
		else
			drawtext(NULL, dc.norm, false);
	}

	ReleaseDC(barhwnd, dc.hdc);
}

void
drawsquare(bool filled, bool empty, bool invert, COLORREF col[ColLast]) {
	static int size = 5;
	RECT r = { .left = dc.x + 1, .top = dc.y + 1, .right = dc.x + size, .bottom = dc.y + size };

	HBRUSH brush = CreateSolidBrush(col[invert ? ColBG : ColFG]);
	SelectObject(dc.hdc, brush);

	if(filled) {
		FillRect(dc.hdc, &r, brush);
	} else if(empty) {
		FillRect(dc.hdc, &r, brush);
	}
	DeleteObject(brush);
}

void
drawtext(const char *text, COLORREF col[ColLast], bool invert) {
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

	HFONT font = (HFONT)GetStockObject(SYSTEM_FONT); 
	SelectObject(dc.hdc, font);

	DrawText(dc.hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void
eprint(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	fflush(stderr);
	va_end(ap);
}

void
focus(Client *c) {
	if(!c || !ISVISIBLE(c))
		for(c = stack; c && !ISVISIBLE(c); c = c->snext);
	if(sel && sel != c)
		drawborder(sel, normbordercolor);
	if(c) {
		if(c->isurgent)
			clearurgent(c);
		detachstack(c);
		attachstack(c);
		drawborder(c, selbordercolor);
		SetForegroundWindow(c->hwnd);
	}
	sel = c;
	drawbar();
}

void
focusstack(const Arg *arg) {
	Client *c = NULL, *i;

	if(!sel)
		return;
	if (arg->i > 0) {
		for(c = sel->next; c && !ISVISIBLE(c); c = c->next);
		if(!c)
			for(c = clients; c && !ISVISIBLE(c); c = c->next);
	}
	else {
		for(i = clients; i != sel; i = i->next)
			if(ISVISIBLE(i))
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i))
					c = i;
	}
	if(c) {
		focus(c);
		restack();
	}
}

Client *
getclient(HWND hwnd) {
	Client *c;

	for(c = clients; c && c->hwnd != hwnd; c = c->next);
	return c;
}

LPSTR
getclientclassname(HWND hwnd) {
	static TCHAR buf[128];
	GetClassName(hwnd, buf, sizeof buf);
	return buf;
}

LPSTR
getclienttitle(HWND hwnd) {
	static TCHAR buf[128];
	GetWindowText(hwnd, buf, sizeof buf);
	return buf;
}

void
grabkeys(HWND hwnd) {
	int i;
	for (i = 0; i < LENGTH(keys); i++) {
		RegisterHotKey(hwnd, i, keys[i].mod, keys[i].key);
	}
}

bool
ismanageable(HWND hwnd){
	debug("ismanageable: %d %s\n", hwnd, getclienttitle(hwnd));
	debug(" visible: %d\n", IsWindowVisible(hwnd));
	debug(" parent : %d\n", GetParent(hwnd));

	/* Some criteria for windows to be tiled */
	if (IsWindowVisible(hwnd) && GetParent(hwnd) == 0) {
		int exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		HWND owner = GetWindow(hwnd, GW_OWNER);
		debug(" owner:      %d\n", owner);
		debug(" toolwindow: %d\n", exstyle & WS_EX_TOOLWINDOW);
		debug(" appwindow:  %d\n", exstyle & WS_EX_APPWINDOW);
		if (((exstyle & WS_EX_TOOLWINDOW) == 0 && owner == 0) || ((exstyle & WS_EX_APPWINDOW) && owner != 0)) {
			return true;
		}
	}
	return false;
}

void
killclient(const Arg *arg) {
	if(!sel)
		return;
	PostMessage(sel->hwnd, WM_CLOSE, 0, 0);
}

Client*
manage(HWND hwnd) {
	debug(" manage %s\n", getclienttitle(hwnd));
	static Client cz;
	Client *c;

	WINDOWINFO wi = {
		.cbSize = sizeof(WINDOWINFO),
	};


	if (!GetWindowInfo(hwnd, &wi))
		return NULL;

	if(!(c = malloc(sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));

	*c = cz;
	c->hwnd = hwnd;

	static WINDOWPLACEMENT wp = {
		.length = sizeof(WINDOWPLACEMENT),
		.showCmd = SW_RESTORE,
	};

	SetWindowPlacement(hwnd, &wp);
	
	/* maybe we could also filter based on 
	 * WS_MINIMIZEBOX and WS_MAXIMIZEBOX
	 */
	c->isfloating = wi.dwStyle & WS_POPUP;

	debug(" window style: %d\n", wi.dwStyle);
	debug("     minimize: %d\n", wi.dwStyle & WS_MINIMIZEBOX);
	debug("     maximize: %d\n", wi.dwStyle & WS_MAXIMIZEBOX);
	debug("        popup: %d\n", wi.dwStyle & WS_POPUP);
	debug("   isfloating: %d\n", c->isfloating);

	applyrules(c);

	setborder(c, c->isfloating);

	if (c->isfloating) {
		debug(" new floating window: x: %d y: %d w: %d h: %d\n", wi.rcWindow.left, wi.rcWindow.top, wi.rcWindow.right - wi.rcWindow.left, wi.rcWindow.bottom - wi.rcWindow.top);
		resize(c, wi.rcWindow.left, wi.rcWindow.top, wi.rcWindow.right - wi.rcWindow.left, wi.rcWindow.bottom - wi.rcWindow.top);
	}

	attach(c);
	attachstack(c);
	return c;
}

void
monocle(void) {
	Client *c;

	for(c = nexttiled(clients); c; c = nexttiled(c->next)) {
		resize(c, wx, wy, ww - 2 * c->bw, wh - 2 * c->bw);
	}
}

Client *
nexttiled(Client *c) {
	for(; c && (c->isfloating || c->isminimized || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
quit(const Arg *arg) {
	PostMessage(dwmhwnd, WM_CLOSE, 0, 0);
}

void
resize(Client *c, int x, int y, int w, int h) {
#if 0
	if(w <= 0 || h <= 0)
		return;
#endif
	if(x > sx + sw)
		x = sw - WIDTH(c);
	if(y > sy + sh)
		y = sh - HEIGHT(c);
	if(x + w + 2 * c->bw < sx)
		x = sx;
	if(y + h + 2 * c->bw < sy)
		y = sy;
	if(h < bh)
		h = bh;
	if(w < bh)
		w = bh;
	if(c->x != x || c->y != y || c->w != w || c->h != h) {
		c->x = x;
		c->y = y;
		c->w = w;
		c->h = h;
		debug(" resize %s: x: %d y: %d w: %d h: %d\n", getclienttitle(c->hwnd), x, y, w, h);
		SetWindowPos(c->hwnd, HWND_TOP, c->x, c->y, c->w, c->h, SWP_NOACTIVATE);
	}
}

void
restack(void) {
#if 0
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar();
	if(!sel)
		return;
	if(sel->isfloating || !lt[sellt]->arrange)
		XRaiseWindow(dpy, sel->win);
	if(lt[sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = barwin;
		for(c = stack; c; c = c->snext)
			if(!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, false);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
#endif
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
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam); 
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
			break;
		case WM_CLOSE:
			cleanup();
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_HOTKEY:
			if (wParam > 0 && wParam < LENGTH(keys)) {
				keys[wParam].func(&(keys[wParam ].arg));
			}
			break;
		default:
			if (msg == shellhookid) { /* Handle the shell hook message */
				debug(" SHELLHOOK: %d == %d\n", wParam, HSHELL_WINDOWACTIVATED);
				Client *c = getclient((HWND)lParam);
				switch (wParam) {
					/* The first two events are also trigger if windows
					 * are being hidden or shown because of a tag
					 * switch, therefore we ignore them in this case.
					 */
					case HSHELL_WINDOWCREATED:
						debug("window created: %s\n", getclienttitle((HWND)lParam));
						if (!c && ismanageable((HWND)lParam)) {
							manage((HWND)lParam);
							arrange();
						}
						break;
					case HSHELL_WINDOWDESTROYED:
						if (c)
							if (!c->ignore)
								unmanage(c);
							else
								c->ignore = false;
						else 
							debug(" unmanaged window destroyed");
						break;
					case HSHELL_WINDOWACTIVATED:
						debug(" window activated: %s || %d\n", c ? getclienttitle(c->hwnd) : "unknown", (HWND)lParam);
						if (c)
							focus(c);
						else  {
							/* Some window don't seem to generate 
							 * HSHELL_WINDOWCREATED messages therefore 
						 	 * we check here whether we should manage
						 	 * the window or not.
						 	 */
							if (ismanageable((HWND)lParam)) {
								c = manage((HWND)lParam);
								focus(c);
								arrange();
							} else
								SetForegroundWindow((HWND)lParam);
						}
						break;
					case HSHELL_GETMINRECT:
						debug("min/max event\n");
						c = getclient(((SHELLHOOKINFO*)lParam)->hwnd);
						if (c) {
							WINDOWPLACEMENT winplace;
							winplace.length = sizeof(WINDOWPLACEMENT);
							if (GetWindowPlacement(c->hwnd, &winplace)) 
								c->isminimized = (winplace.showCmd == SW_SHOWMINIMIZED);
							debug(" window state changed: %s\n", c->isminimized ? "minimized" : "maximized");
							arrange();						
						}
						break;
				}
			} else
				return DefWindowProc(hwnd, msg, wParam, lParam); 
	}

	return 0;
}

BOOL CALLBACK 
scan(HWND hwnd, LPARAM lParam) {
	if (ismanageable(hwnd))
		manage(hwnd);
	return TRUE;
}

void
drawborder(Client *c, COLORREF color) {
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
}

void
setborder(Client *c, bool border) {
	if (border) {
		SetWindowLong(c->hwnd, GWL_STYLE, (GetWindowLong(c->hwnd, GWL_STYLE) | (WS_CAPTION | WS_SIZEBOX)));
		SetWindowPos(c->hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER); /* XXX: needed */
	} else {
		SetWindowLong(c->hwnd, GWL_STYLE, (GetWindowLong(c->hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_SIZEBOX)));
	}
	c->border = border;
}


void
setvisibility(HWND hwnd, bool visibility) {
	SetWindowPos(hwnd, 0, 0, 0, 0, 0, (visibility ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void
setlayout(const Arg *arg) {
	if(!arg || !arg->v || arg->v != lt[sellt])
		sellt ^= 1;
	if(arg && arg->v)
		lt[sellt] = (Layout *)arg->v;
	if(sel)
		arrange();
	else
		drawbar();
}

/* arg > 1.0 will set mfact absolutly */
void
setmfact(const Arg *arg) {
	float f;

	if(!arg || !lt[sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + mfact : arg->f - 1.0;
	if(f < 0.1 || f > 0.9)
		return;
	mfact = f;
	arrange();
}

void
setup(HINSTANCE hInstance) {

	lt[0] = &layouts[0];
	lt[1] = &layouts[1 % LENGTH(layouts)];

	/* init appearance */

	dc.norm[ColBorder] = normbordercolor;
	dc.norm[ColBG] = normbgcolor;
	dc.norm[ColFG] = normfgcolor;
	dc.sel[ColBorder] = selbordercolor;
	dc.sel[ColBG] = selbgcolor;
	dc.sel[ColFG] = selfgcolor;

	updategeom();

	WNDCLASSEX winClass;

	winClass.cbSize = sizeof(WNDCLASSEX);
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

	if (!RegisterClassEx(&winClass))
		die("Error registering window class");

	dwmhwnd = CreateWindowEx(0, NAME, NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

	if (!dwmhwnd)
		die("Error Creating Window");

	grabkeys(dwmhwnd);

	EnumWindows(scan, 0);

	setupbar(hInstance);

	arrange();
	
	/* Get function pointer for RegisterShellHookWindow */
	RegisterShellHookWindow = (RegisterShellHookWindowProc)GetProcAddress(GetModuleHandle("USER32.DLL"), "RegisterShellHookWindow");
	if (!RegisterShellHookWindow)
		die("Could not find RegisterShellHookWindow");
	RegisterShellHookWindow(dwmhwnd);
	/* Grab a dynamic id for the SHELLHOOK message to be used later */
	shellhookid = RegisterWindowMessage("SHELLHOOK");
}

void
setupbar(HINSTANCE hInstance) {

	unsigned int i, w = 0;

	WNDCLASS winClass;
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
	winClass.lpszClassName = "dwm-bar";

	if (!RegisterClass(&winClass))
		die("Error registering window class");

	barhwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		"dwm-bar",
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

	for(blw = i = 0; LENGTH(layouts) > 1 && i < LENGTH(layouts); i++) {
 		w = TEXTW(layouts[i].symbol);
		blw = MAX(blw, w);
	}

	ReleaseDC(barhwnd, dc.hdc);

	PostMessage(barhwnd, WM_PAINT, 0, 0);
	updatebar();
}

void
showclientclassname(const Arg *arg) {
	MessageBox(NULL, getclientclassname(GetForegroundWindow()), "Window class", MB_OK);
}

void
showhide(Client *c) {
	if(!c)
		return;
	if (!ISVISIBLE(c))
		c->ignore = true;
	/* XXX: is the order of showing / hidding important?
	 *      if so then use recursion like dwm does
	 */
	setvisibility(c->hwnd, ISVISIBLE(c));
	showhide(c->snext);
}

void
spawn(const Arg *arg) {
	ShellExecute(NULL, NULL, ((char **)arg->v)[0], ((char **)arg->v)[1], NULL, SW_SHOWDEFAULT);
}

void
tag(const Arg *arg) {
	if(sel && arg->ui & TAGMASK) {
		sel->tags = arg->ui & TAGMASK;
		arrange();
	}
}

int
textnw(const char *text, unsigned int len) {
	SIZE size;
	GetTextExtentPoint(dc.hdc, text, len, &size);
	if (size.cx > 0)
		size.cx += textmargin;
	return size.cx;  
}


void
tile(void) {
	int x, y, h, w, mw;
	unsigned int i, n;
	Client *c;

	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), n++);
	if(n == 0)
		return;

	/* master */
	c = nexttiled(clients);
	mw = mfact * ww;
	resize(c, wx, wy, (n == 1 ? ww : mw) - 2 * c->bw, wh - 2 * c->bw);

	if(--n == 0)
		return;

	/* tile stack */
	x = (wx + mw > c->x + c->w) ? c->x + c->w + 2 * c->bw : wx + mw;
	y = wy;
	w = (wx + mw > c->x + c->w) ? wx + ww - x : ww - mw;
	h = wh / n;
	if(h < bh)
		h = wh;

	for(i = 0, c = nexttiled(c->next); c; c = nexttiled(c->next), i++) {
		resize(c, x, y, w - 2 * c->bw, /* remainder */ ((i + 1 == n)
		       ? wy + wh - y - 2 * c->bw : h - 2 * c->bw));
		if(h != wh)
			y = c->y + HEIGHT(c);
	}
}

void
togglebar(const Arg *arg) {
	showbar = !showbar;
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
	HWND hwnd = FindWindow("Progman", "Program Manager");
	if (hwnd)
		setvisibility(hwnd, !IsWindowVisible(hwnd));

	hwnd = FindWindow("Shell_TrayWnd", NULL);
	if (hwnd)
		setvisibility(hwnd, !IsWindowVisible(hwnd));

	
	updategeom();
	updatebar();
	arrange();		
}


void
togglefloating(const Arg *arg) {
	if(!sel)
		return;
	sel->isfloating = !sel->isfloating || sel->isfixed;
	setborder(sel, sel->isfloating);
	if(sel->isfloating)
		resize(sel, sel->x, sel->y, sel->w, sel->h);
	arrange();
}

void
toggletag(const Arg *arg) {
	unsigned int mask;

	if (!sel)
		return;
	
	mask = sel->tags ^ (arg->ui & TAGMASK);
	if(mask) {
		sel->tags = mask;
		arrange();
	}
}

void
toggleview(const Arg *arg) {
	unsigned int mask = tagset[seltags] ^ (arg->ui & TAGMASK);

	if(mask) {
		tagset[seltags] = mask;
		arrange();
	}
}

void
unmanage(Client *c) {
	debug(" unmanage %s\n", getclienttitle(c->hwnd));
	setborder(c, true);
	detach(c);
	detachstack(c);
	if(sel == c)
		focus(NULL);
	free(c);
	arrange();
}

void
updatebar(void) {
	//debug("updatebar: %s  x: %d y: %d w: %d h: %d\n", showbar ? "show" : "hide" , 0, by, ww, bh);
	SetWindowPos(barhwnd, showbar ? HWND_TOPMOST : HWND_NOTOPMOST, 0, by, ww, bh, (showbar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	
}

void
updategeom(void) {
	RECT wa;
	HWND hwnd = FindWindow("Shell_TrayWnd", NULL);
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

	bh = 20; /* XXX: fixed value */

	/* window area geometry */
	wx = sx;
	wy = showbar && topbar ? sy + bh : sy;
	ww = sw;
	wh = showbar ? sh - bh : sh;
	/* bar position */
	by = showbar ? (topbar ? wy - bh : wy + wh) : -bh;
	debug("updategeom: %d x %d\n", ww, wh);
}

void
updatestatus() {
	drawbar();
}

void
view(const Arg *arg) {
	if((arg->ui & TAGMASK) == tagset[seltags])
		return;
	seltags ^= 1; /* toggle sel tagset */
	if(arg->ui & TAGMASK)
		tagset[seltags] = arg->ui & TAGMASK;
	arrange();
}

void
zoom(const Arg *arg) {
	Client *c = sel;

	if(!lt[sellt]->arrange || lt[sellt]->arrange == monocle || (sel && sel->isfloating))
		return;
	if(c == nexttiled(clients))
		if(!c || !(c = nexttiled(c->next)))
			return;
	detach(c);
	attach(c);
	focus(c);
	arrange();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {	
	MSG msg;

	setup(hInstance);

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	cleanup();

	return msg.wParam;
}
