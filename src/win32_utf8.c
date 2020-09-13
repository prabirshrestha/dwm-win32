/* https://github.com/tnodir/luasys/blob/9a916b6104f8f990cac2f336990fec068be1a604/src/win32/win32_utf8.c */
/* Lua System: Win33 specifics: UTF-8 */
#include <windows.h>

/*
 * Convert UTF-8 to microsoft unicode.
 */
void *
utf8_to_utf16 (const char *s)
{
  WCHAR *ws;
  int n;

  n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  ws = malloc(n * sizeof(WCHAR));
  if (!ws) return NULL;

  n = MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, n);
  if (!n) {
    free(ws);
    ws = NULL;
  }
  return ws;
}

/*
 * Convert microsoft unicode to UTF-8.
 */
void *
utf16_to_utf8 (const WCHAR *ws)
{
  char *s;
  int n;

  n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, 0);
  s = malloc(n);
  if (!s) return NULL;

  n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, n, NULL, 0);
  if (!n) {
    free(s);
    s = NULL;
  }
  return s;
}

/*
 * Convert an ansi string to microsoft unicode, based on the
 * current codepage settings for file apis.
 */
void *
mbcs_to_utf16 (const char *mbcs)
{
  WCHAR *ws;
  const int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  int n;

  n = MultiByteToWideChar(codepage, 0, mbcs, -1, NULL, 0);
  ws = malloc(n * sizeof(WCHAR));
  if (!ws) return NULL;

  n = MultiByteToWideChar(codepage, 0, mbcs, -1, ws, n);
  if (!n) {
    free(ws);
    ws = NULL;
  }
  return ws;
}

/*
 * Convert microsoft unicode to multibyte character string, based on the
 * user's Ansi codepage.
 */
void *
utf16_to_mbcs (const WCHAR *ws)
{
  char *mbcs;
  const int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  int n;

  n = WideCharToMultiByte(codepage, 0, ws, -1, NULL, 0, NULL, 0);
  mbcs = malloc(n);
  if (!mbcs) return NULL;

  n = WideCharToMultiByte(codepage, 0, ws, -1, mbcs, n, NULL, 0);
  if (!n) {
    free(mbcs);
    mbcs = NULL;
  }
  return mbcs;
}

/*
 * Convert multibyte character string to UTF-8.
 */
void *
mbcs_to_utf8 (const char *mbcs)
{
  char *s;
  WCHAR *ws;

  ws = mbcs_to_utf16(mbcs);
  if (!ws) return NULL;

  s = utf16_to_utf8(ws);
  free(ws);
  return s;
}

/*
 * Convert UTF-8 to multibyte character string.
 */
void *
utf8_to_mbcs (const char *s)
{
  char *mbcs;
  WCHAR *ws;

  ws = utf8_to_utf16(s);
  if (!ws) return NULL;

  mbcs = utf16_to_mbcs(ws);
  free(ws);
  return mbcs;
}

/*
 * Convert UTF-8 to OS filename.
 */
void *
utf8_to_filename (const char *s)
{
  return _WIN32_WINNT ? utf8_to_utf16(s) : utf8_to_mbcs(s);
}

/*
 * Convert OS filename to UTF-8.
 */
char *
filename_to_utf8 (const void *s)
{
  return _WIN32_WINNT ? utf16_to_utf8(s) : mbcs_to_utf8(s);
}
