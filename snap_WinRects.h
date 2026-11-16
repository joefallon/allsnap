#ifndef SNAP_WINRECTS_INCLUDE

#define SNAP_WINRECTS_INCLUDE

#include "snap_testers.h"
#include "sides.h"
#include "snap_CornerInfo.h"

#define IS_MDI_CHILD(hwnd)((GetWindowLong((hwnd),GWL_EXSTYLE) & WS_EX_MDICHILD) != 0)

void WinRects_Refresh(DWORD get_which, HWND hwnd);
void WinRects_getRects(PTEST_RECTS);
void WinRects_RefreshStartScreen(void);

int		WinRects_GetNumScreens(void);
RECT *	WinRects_GetScreens(void);
/* Classifies a candidate from the current WinRects_getRects snapshot. */
BOOL WinRects_IsWorkArea(const RECT * candidate);
BOOL WinRects_IsOldPosition(const RECT * candidate);

BOOL WinRects_GetWindowRect(HWND hWnd, LPRECT pRect);
BOOL WinRects_GetRgnBox(HWND hWnd, LPRECT pRgnBox);

void WinRects_getCurrentScreen(RECT * pRect);
void WinRects_getScreenFromPt(POINT pt, RECT * pRect);

#ifdef ALLSNAP_TESTING
void WinRects_TestSetScreens(const RECT * screens, int count, const RECT * current_screen);
void WinRects_TestSetWindows(const RECT * windows, int count);
void WinRects_TestRefreshScreens(const RECT * screens, int count, DWORD get_which, const RECT * window);
#endif

#endif
