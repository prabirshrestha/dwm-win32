/* See LICENSE file for copyright and license details. */
#define NDEBUG 1

static const wchar_t *fontname         = L"Fira Code";
static const unsigned int fontsize  = 20;

/* appearance, colors are specified in the form 0x00bbggrr or with the RGB(r, g, b) macro */
#define normbordercolor 0x00444444
#define normbgcolor     0x00222222
#define normfgcolor     0x00bbbbbb
#define selbordercolor  0x00775500
#define selbgcolor      0x00775500
#define selfgcolor      0x00eeeeee

static const unsigned int borderpx    = 0;        /* border pixel of windows */
static const unsigned int textmargin  = 15;       /* margin for the text displayed on the bar */
static bool showbar                   = true;     /* false means no bar */
static bool topbar                    = true;     /* false means bottom bar */
static bool showclock                 = true;     /* false means no clock */
static bool showutcclock              = true;     /* false means no utc clock */
static bool showexploreronstart       = false;    /* false means do not show explorer/task bar on start */

/* tagging */
static const wchar_t tags[][MAXTAGLEN] = { L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9" };
static unsigned int tagset[] = {1, 1}; /* after start, first tag is selected */

static Rule rules[] = {
    /* class                                title                                   tags mask   isfloating      ignoreborder */
    { L"MultitaskingViewFrame",              NULL,                                   0,          true,           true },
    { L"MSCTFIME UI",                        NULL,                                   0,          true,           true },
    { L"Microsoft-Windows-SnipperToolbar",   L"Snipping Tool",                        0,          true,           true },
    { L"Microsoft Text Input Application",   NULL,                                   0,          true,           true },
    { L"MSO_BORDEREFFECT_WINDOW_CLASS",      NULL,                                   0,          true,           true },
    { L"CASCADIA_HOSTING_WINDOW_CLASS",      NULL,                                   0,          false,          true },
    { L"ThumbnailDeviceHelperWnd",           NULL,                                   0,          true,           true },
    { L"EdgeUiInputTopWndClass",             NULL,                                   0,          true,           true },
    { L"CabinetWClass",                      NULL,                                   0,          false,          true }, /* file explorer */
    { L"XLMAIN",                             NULL,                                   0,          false,          true }, /* Excel */
    { NULL,                                  L"MSO_BORDEREFFECT_WINDOW_CLASS",       0,          false,          true }, /* Excel */
    { L"PPTFrameClass",                      NULL,                                   0,          false,          true }, /* PowerPoint */
    { L"OpusApp",                            NULL,                                   0,          false,          true }, /* Word */
    { NULL,                                  L"OneNote",                             0,          false,          true }, /* OneNote */
    { NULL,                                 L"Snip & Sketch",                        0,          true,           true },
    { L"Chrome_WidgetWin_1",                 L"Google Chrome",                        0,          false,          true },
    { NULL,                                 L"vimrun.exe",                           0,          true,           true },
};

/* layout(s) */
static float mfact      = 0.55; /* factor of master area size [0.05..0.95] */

#include "bstack.c"
#include "grid.c"
#include "gaplessgrid.c"
#include "fibonacci.c"

static Layout layouts[] = {
    /* symbol     arrange function */
    { L"[]=",      tile },    /* first entry is default */
    { L"><>",      NULL },    /* no layout function means floating behavior */
    { L"[M]",      monocle },
    { L"TTT",      bstack },
    { L"###",      gaplessgrid },
    { L"+++",      grid },
    { L"(@)",      spiral },
    { L"[\\]",     dwindle },
};

/* key definitions */
#define MODKEY           (MOD_ALT)
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|MOD_CONTROL,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|MOD_SHIFT,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|MOD_CONTROL|MOD_SHIFT, KEY,      toggletag,      {.ui = 1 << TAG} },

static wchar_t clockfmt[] = L"%m/%d/%Y %a %H:%M";
static int clock_interval = 15000;

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const wchar_t*[]){ L"/bin/sh", L"-c", cmd, NULL } }

/* commands */
static const wchar_t *termcmd[]  = { L"wt.exe", NULL };

static Key keys[] = {
    /* modifier                     key        function             argument */
    { 0 }, // ??? dummy empty
    { MODKEY|MOD_SHIFT,             VK_RETURN, spawn,               {.v = termcmd } },
    { MODKEY|MOD_CONTROL,           'B',       togglebar,           {0} },
    { MODKEY,                       'J',       focusstack,          {.i = +1 } },
    { MODKEY,                       'K',       focusstack,          {.i = -1 } },
    { MODKEY,                       'H',       setmfact,            {.f = -0.05} },
    { MODKEY,                       'L',       setmfact,            {.f = +0.05} },
    { MODKEY,                       'I',       showclientinfo,      {0} },
    { MODKEY|MOD_CONTROL,           VK_RETURN, zoom,                {0} },
    { MODKEY,                       VK_TAB,    view,                {0} },
    { MODKEY|MOD_SHIFT,             'C',       killclient,          {0} },
    { MODKEY,                       'T',       setlayout,           {.v = &layouts[0]} },
    { MODKEY,                       'F',       setlayout,           {.v = &layouts[1]} },
    { MODKEY,                       'M',       setlayout,           {.v = &layouts[2]} },
    { MODKEY,                       'B',       setlayout,           {.v = &layouts[3]} },
    { MODKEY,                       'G',       setlayout,           {.v = &layouts[4]} },
    { MODKEY|MOD_SHIFT,             'G',       setlayout,           {.v = &layouts[5]} },
    { MODKEY,                       'D',       setlayout,           {.v = &layouts[6]} },
    { MODKEY|MOD_SHIFT,             'D',       setlayout,           {.v = &layouts[7]} },
    { MODKEY|MOD_CONTROL,           VK_SPACE,  setlayout,           {0} },
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
    { MODKEY|MOD_CONTROL,           'Q',       quit,                {0} },
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
