#include "stdafx.h"
#include <dwmapi.h>
#include "snap_frame.h"
#include "snap_lib_internal.h"

typedef HANDLE (WINAPI *SET_THREAD_DPI_CONTEXT)(HANDLE);
static int g_logical_width = 1, g_physical_width = 1;
static int g_logical_height = 1, g_physical_height = 1;

#ifdef ALLSNAP_TESTING
static SNAP_FRAME_TEST_OPS g_frame_test_ops = {0};
void SnapFrame_TestSetOps(const SNAP_FRAME_TEST_OPS * operations){
	if (operations){
		g_frame_test_ops = *operations;
	}else{
		ZeroMemory(&g_frame_test_ops,sizeof(g_frame_test_ops));
	}
}
#endif

static SET_THREAD_DPI_CONTEXT GetContextOperation(void){
#ifdef ALLSNAP_TESTING
	if (g_frame_test_ops.set_context) return g_frame_test_ops.set_context;
#endif
	return (SET_THREAD_DPI_CONTEXT)GetProcAddress(
		GetModuleHandle(_T("user32.dll")), "SetThreadDpiAwarenessContext");
}

static BOOL FrameWindowRect(HWND hwnd, RECT * rectangle){
#ifdef ALLSNAP_TESTING
	if (g_frame_test_ops.get_window_rect) return g_frame_test_ops.get_window_rect(hwnd,rectangle);
#endif
	return GetWindowRect(hwnd,rectangle);
}

static HRESULT FrameDwmBounds(HWND hwnd, RECT * rectangle){
#ifdef ALLSNAP_TESTING
	if (g_frame_test_ops.get_dwm_bounds)
		return g_frame_test_ops.get_dwm_bounds(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,rectangle,sizeof(*rectangle));
#endif
	return DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,rectangle,sizeof(*rectangle));
}

/* Measure the outer rectangle independently of optional DWM frame information. */
static BOOL GetPhysicalOuterBounds(HWND hwnd, RECT * outer){
	SET_THREAD_DPI_CONTEXT set_context = GetContextOperation();
	HANDLE previous;
	BOOL result, restored;
	if (!set_context){
		return FALSE;
	}
	previous = set_context((HANDLE)-4); /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
	if (!previous){
		return FALSE;
	}
	result = FrameWindowRect(hwnd,outer);
	/* Restore even when the measurement fails; never report success after a failed restore. */
	restored = set_context(previous) != NULL;
	return result && restored;
}

BOOL SnapFrame_CalculateInsets(const RECT * logical_outer, const RECT * physical_outer,
	const RECT * visible, RECT * offsets){
	int physical_width = rectWidth(*physical_outer), physical_height = rectHeight(*physical_outer);
	int logical_width = rectWidth(*logical_outer), logical_height = rectHeight(*logical_outer);
	SetRectEmpty(offsets);
	if (physical_width <= 0 || physical_height <= 0 || logical_width <= 0 || logical_height <= 0
		|| visible->left < physical_outer->left || visible->top < physical_outer->top
		|| visible->right > physical_outer->right || visible->bottom > physical_outer->bottom
		|| IsRectEmpty(visible)){
		return FALSE;
	}
	offsets->left = MulDiv(visible->left - physical_outer->left,logical_width,physical_width);
	offsets->right = MulDiv(visible->right - physical_outer->right,logical_width,physical_width);
	offsets->top = MulDiv(visible->top - physical_outer->top,logical_height,physical_height);
	offsets->bottom = MulDiv(visible->bottom - physical_outer->bottom,logical_height,physical_height);
	return TRUE;
}

BOOL SnapFrame_GetInsets(HWND hwnd, const RECT * logical_outer, RECT * offsets){
	RECT physical_outer, visible;
	SetRectEmpty(offsets);
	return GetPhysicalOuterBounds(hwnd,&physical_outer)
		&& SUCCEEDED(FrameDwmBounds(hwnd,&visible))
		&& SnapFrame_CalculateInsets(logical_outer,&physical_outer,&visible,offsets);
}

void SnapFrame_SetCoordinateWindow(HWND hwnd){
	RECT logical_outer, physical_outer;
	g_logical_width = g_physical_width = g_logical_height = g_physical_height = 1;
	if (hwnd && FrameWindowRect(hwnd,&logical_outer) && GetPhysicalOuterBounds(hwnd,&physical_outer)
		&& rectWidth(physical_outer) > 0 && rectHeight(physical_outer) > 0
		&& rectWidth(logical_outer) > 0 && rectHeight(logical_outer) > 0){
		g_logical_width = rectWidth(logical_outer);
		g_physical_width = rectWidth(physical_outer);
		g_logical_height = rectHeight(logical_outer);
		g_physical_height = rectHeight(physical_outer);
	}
}

int SnapFrame_GetSpacing(enum SIDE side){
	return isVerticalSide(side)
		? MulDiv(getWindowSpacing(),g_logical_height,g_physical_height)
		: MulDiv(getWindowSpacing(),g_logical_width,g_physical_width);
}

void SnapFrame_InsetWorkArea(const RECT * work_area, RECT * inset){
	int x = min(SnapFrame_GetSpacing(SIDE_LEFT),max(0,(rectWidth(*work_area)-1)/2));
	int y = min(SnapFrame_GetSpacing(SIDE_TOP),max(0,(rectHeight(*work_area)-1)/2));
	*inset = *work_area;
	inset->left += x;
	inset->right -= x;
	inset->top += y;
	inset->bottom -= y;
}
