/* See LICENSE file for copyright and license details. */
#define NDEBUG 1

/* appearance, colors are specified in the form 0x00bbggrr or with the RGB(r, g, b) macro */
#define normbordercolor 0x00444444
#define normbgcolor     0x00333333
#define normfgcolor     0x00bbbbbb
#define selbordercolor  0x00005577
#define selbgcolor      0x00333333
#define selfgcolor      0x003300ff

static const unsigned int borderpx    = 0;        /* border pixel of windows */
static const unsigned int textmargin  = 5;        /* margin for the text displayed on the bar */
static bool showbar                   = true;     /* false means no bar */
static bool topbar                    = true;     /* false means bottom bar */
static bool showclock                 = true;     /* false means no clock */
static bool showexploreronstart       = false;    /* false means do not show explorer/task bar on start */

/* tagging */
static const char tags[][MAXTAGLEN] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static unsigned int tagset[] = {1, 1}; /* after start, first tag is selected */

static Rule rules[] = {
	/* class                   title                      tags mask     isfloating */
	{ "MozillaUIWindowClass",  "- Mozilla Firefox",       1 << 1,       false },
	{ "Chrome_WidgetWin_1",    "- Google Chrome",         1 << 1,       false },
};

/* layout(s) */
static float mfact      = 0.55; /* factor of master area size [0.05..0.95] */

static Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY 		(MOD_CONTROL | MOD_ALT)
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|MOD_CONTROL,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|MOD_SHIFT,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|MOD_CONTROL|MOD_SHIFT, KEY,      toggletag,      {.ui = 1 << TAG} },

static char clockfmt[] = "%Y/%m/%d(%a) %H:%M";
static int clock_interval = 15000;

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *termcmd[]  = { "cmd.exe", NULL };

static Key keys[] = {
	/* modifier                     key        function             argument */
	{ MODKEY|MOD_SHIFT,             VK_RETURN, spawn,               {.v = termcmd } },
	{ MODKEY,                       'B',       togglebar,           {0} },
	{ MODKEY,                       'J',       focusstack,          {.i = +1 } },
	{ MODKEY,                       'K',       focusstack,          {.i = -1 } },
	{ MODKEY,                       'H',       setmfact,            {.f = -0.05} },
	{ MODKEY,                       'L',       setmfact,            {.f = +0.05} },
	{ MODKEY,                       'I',       showclientclassname, {0} },
	{ MODKEY,                       VK_RETURN, zoom,                {0} },
	{ MODKEY,                       VK_TAB,    view,                {0} },
	{ MODKEY|MOD_SHIFT,             'C',       killclient,          {0} },
	{ MODKEY,                       'T',       setlayout,           {.v = &layouts[0]} },
	{ MODKEY,                       'F',       setlayout,           {.v = &layouts[1]} },
	{ MODKEY,                       'M',       setlayout,           {.v = &layouts[2]} },
	{ MODKEY,                       VK_SPACE,  setlayout,           {0} },
	{ MODKEY|MOD_SHIFT,             VK_SPACE,  togglefloating,      {0} },
	{ MODKEY,                       'N',       toggleborder,        {0} },
	{ MODKEY,                       'E',       toggleexplorer,      {0} },
	{ MODKEY,                       '0',       view,                {.ui = ~0 } },
	{ MODKEY|MOD_SHIFT,             '0',       tag,                 {.ui = ~0 } },
	TAGKEYS(                        '1',                            0)
	TAGKEYS(                        '2',                            1)
	TAGKEYS(                        '3',                            2)
	TAGKEYS(                        '4',                            3)
	TAGKEYS(                        '5',                            4)
	TAGKEYS(                        '6',                            5)
	TAGKEYS(                        '7',                            6)
	TAGKEYS(                        '8',                            7)
	TAGKEYS(                        '9',                            8)
	{ MODKEY,                       'Q',       quit,                {0} },
};


/* button definitions */
/* click can be a tag number (starting at 0), ClkLtSymbol, ClkStatusText or ClkWinTitle */
static Button buttons[] = {
	/* click                button event type     modifier keys    function        argument */
	{ ClkLtSymbol,          WM_LBUTTONDOWN,       0,               setlayout,      {0} },
	{ ClkLtSymbol,          WM_RBUTTONDOWN,       0,               setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          WM_MBUTTONDOWN,       0,               zoom,           {0} },
	{ ClkStatusText,        WM_MBUTTONDOWN,       0,               spawn,          {.v = termcmd } },
#if 0
	{ ClkClientWin,         WM_MBUTTONDOWN,       MODKEY,          togglefloating, {0} },
#endif
	{ ClkTagBar,            WM_LBUTTONDOWN,       VK_MENU,         tag,            {0} },
	{ ClkTagBar,            WM_RBUTTONDOWN,       VK_MENU,         toggletag,      {0} },
	{ ClkTagBar,            WM_LBUTTONDOWN,       0,               view,           {0} },
	{ ClkTagBar,            WM_RBUTTONDOWN,       0,               toggleview,     {0} },
};
