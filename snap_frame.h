#ifndef SNAP_FRAME_H
#define SNAP_FRAME_H

#include <windows.h>
#include "sides.h"

BOOL SnapFrame_CalculateInsets(const RECT * logical_outer, const RECT * physical_outer,
    const RECT * visible, RECT * offsets);
BOOL SnapFrame_GetInsets(HWND hwnd, const RECT * logical_outer, RECT * offsets);
void SnapFrame_SetCoordinateWindow(HWND hwnd);
int SnapFrame_GetSpacing(enum SIDE side);
void SnapFrame_InsetWorkArea(const RECT * work_area, RECT * inset);

#ifdef ALLSNAP_TESTING
typedef struct SNAP_FRAME_TEST_OPS {
    HANDLE (WINAPI *set_context)(HANDLE);
    BOOL (WINAPI *get_window_rect)(HWND, LPRECT);
    HRESULT (WINAPI *get_dwm_bounds)(HWND, DWORD, PVOID, DWORD);
} SNAP_FRAME_TEST_OPS;
void SnapFrame_TestSetOps(const SNAP_FRAME_TEST_OPS * operations);
#endif

#endif
