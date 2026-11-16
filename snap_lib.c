#include "stdafx.h"
#include <windowsx.h>
#include <Commctrl.h>
#define SNAPLIBAPI __declspec(dllexport)
#include "snap_lib.h"
#include "snapper.h"
#include "snap_Winrects.h"
#include "wdjsub.h"
#include "mydebug.h"
#include "snap_MultiSz.h"
#include "snap_mywm_msg.h"
#include "snap_lifecycle_test.h"

#ifdef BEEPING
#define BEEP {MessageBeep(-1);}
#else
#define BEEP
#endif

#ifdef _WIN64
#define ALLSNAP_SUBCLASS_ID (477524932)
#define WM_TO_SEND (WM_USER + 13)
#define WM_TO_GET (WM_USER + 12)
#define SHARENAME "Shared64"
#define SHARECMD "/section:Shared64,rws"
#else
#define SHARENAME "Shared32"
#define SHARECMD "/section:Shared32,rws"
#define ALLSNAP_SUBCLASS_ID (477524964)
#define WM_TO_SEND (WM_USER + 12)
#define WM_TO_GET  (WM_USER + 13)
#endif


#define IS_SIZING_SPOT(x) ( ((x) >= HTSIZEFIRST) && ((x) <= HTSIZELAST) \
							|| ((x) == HTSIZE))

//Instruct compiler to put the g_thresh and g_enabled data 
// in its own data Section called Shared. We then instruct the
//linker that we want to share the data in this section
//with all instances of this aplication.?

#pragma data_seg(SHARENAME)
HWND	g_hWnd_app		= NULL;
UINT	g_sounds_thread_id		= 0;

gridsnap_settings_t g_gridsnap = {
	{0,0,1},
	{0,0,1}
};

int		g_win_thresh			= 10;
int		g_screen_thresh			= 10;
UINT	g_toggle_key			= VK_MENU;
UINT    g_center_key            = VK_CONTROL;
UINT    g_equal_key             = VK_SHIFT;
UINT	g_crop_top				= 3;
BOOL	g_is_cropping_top		= FALSE;
BOOL	g_enabled				= TRUE;
UINT	g_snap_type				= SNAPT_OTHERS | SNAPT_DESKTOP;
BOOL	g_is_noisy				= FALSE;
BOOL	g_snap_mdi				= FALSE;
BOOL	g_is_disable_toggled	= TRUE;
BOOL	g_snap_insides 			= TRUE;
BOOL	g_kept_to_screen		= FALSE;
BOOL	g_cropping_rgn			= TRUE;
OSVERSIONINFO		g_os;

volatile LONG g_active_subclasses = 0;
volatile LONG g_hook_stopping = FALSE;
int g_window_spacing = 0;

TCHAR	g_skinned_classes[MAX_CLASSNAME_LENGTH * MAX_NUM_CLASS_NAMES] =
  _T("icoCTactTrilly;Winamp v1.x;ConsoleWindowClass;Winamp PE;Winamp EQ;Winamp MB;BaseWindow_RootWnd");	
TCHAR	g_ignored_classes[MAX_CLASSNAME_LENGTH * MAX_NUM_CLASS_NAMES] =
  _T("Progman;IDEOwner;NeilShadow");
#pragma data_seg()

HHOOK g_hhook = NULL;
volatile LONG g_local_registration = 0;
DWORD g_subclass_thread = 0;
SUBCLASSPROC	g_subclass_proc = NULL;
HWND	g_subclassed_window = NULL;
WNDPROC g_original_proc = NULL; 


// Instruct the linker to make the Shared section
// readable
#pragma comment(linker, SHARECMD)

 
//Nonshared variables
HINSTANCE	g_hinstDll	= NULL;
//WNDPROC 	g_wpOrigWndProc;


BOOL		g_moved_or_sized =FALSE;
BOOL		g_was_zoomed=FALSE;

BOOL		g_was_aerosnapped = FALSE;
int			g_last_height = 0;
int			g_last_width=0;

#define NUM_MSG_TO_SKIP (25)
unsigned int		g_num_msgs_so_far;


//Forward References
LRESULT WINAPI CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI CallWndProcNT(int nCode, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI CALLBACK MouseProc(int nCode,WPARAM wParam,LPARAM lParam);
#ifdef _WIN64
LRESULT APIENTRY SubclassProc64(
	HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
);
#else
LRESULT APIENTRY SubclassProc32(
	HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
);
#endif


void UnSubclass(HWND hwnd);
void Subclass(HWND hwnd);
//void ForceDebugBreak(){
//	__try { DebugBreak();}
//	__except(UnhandledExceptionFilter(GetExceptionInformation())){}
//}

BOOL is_64wnd(HWND hwnd){
    BOOL is_64 = FALSE;
 	BOOL res = FALSE;
	DWORD thisthread = GetCurrentThreadId();
	HANDLE phandle = GetCurrentProcess();//(PROCESS_ALL_ACCESS,FALSE,pid);
//	res = IsWow64Process(phandle,&is_64);
//	
//	DWORD pid = GetWindowThreadProcessId(hwnd,NULL);
	
//	DBG_MSG_PTR(g_hWnd_app,DBGMSG_WNDTHREAD,pid);
//	DBG_MSG_PTR(g_hWnd_app,DBGMSG_THISTHREAD,thisthread);
//	DBG_MSG_PTR(g_hWnd_app,DBGMSG_PROCESS,phandle);
//DBG_MSG_PTR(g_hWnd_app,DBGMSG_IS64,is_64);
	is_64 = !IsWow64Message();
	DBG_MSG_PTR(g_hWnd_app,DBGMSG_IS64,is_64);
	
	CloseHandle(phandle);
	return is_64;
}

BOOL is_matching_platform(HWND hwnd){
	return TRUE;
#ifdef _WIN64
	return TRUE;
	//	return !IsWow64Message();
//	return is_64wnd(hwnd);
#else
	return TRUE;
//	return !is_64wnd(hwnd);
#endif
	

}

#ifdef _WIN64
#define SNAP_SUBCLASS_PROC SubclassProc64
#else
#define SNAP_SUBCLASS_PROC SubclassProc32
#endif

#ifdef ALLSNAP_TESTING
/* A test must explicitly supply a hook operation; never hook the user's desktop. */
static HHOOK WINAPI TestHookNotSupported(int type, HOOKPROC proc, HINSTANCE instance, DWORD thread)
{
    (void)type; (void)proc; (void)instance; (void)thread;
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}
static BOOL WINAPI TestUnhookNotSupported(HHOOK hook)
{
    (void)hook;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
static const SNAP_LIFECYCLE_TEST_OPS g_default_lifecycle_ops = {
    TestHookNotSupported, TestUnhookNotSupported, SetWindowSubclass, RemoveWindowSubclass
};
static SNAP_LIFECYCLE_TEST_OPS g_lifecycle_ops = {
    TestHookNotSupported, TestUnhookNotSupported, SetWindowSubclass, RemoveWindowSubclass
};
#define LifecycleInstallHook g_lifecycle_ops.install_hook
#define LifecycleRemoveHook g_lifecycle_ops.remove_hook
#define LifecycleInstallSubclass g_lifecycle_ops.install_subclass
#define LifecycleRemoveSubclass g_lifecycle_ops.remove_subclass
#else
#define LifecycleInstallHook SetWindowsHookEx
#define LifecycleRemoveHook UnhookWindowsHookEx
#define LifecycleInstallSubclass SetWindowSubclass
#define LifecycleRemoveSubclass RemoveWindowSubclass
#endif

static DWORD LifecycleError(void)
{
    DWORD error = GetLastError();
    return error ? error : ERROR_GEN_FAILURE;
}

static void ReportLifecycleFailure(enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error)
{
    if (g_hWnd_app != NULL)
        PostMessage(g_hWnd_app, MYWM_DIAGNOSTIC, (WPARAM)stage, (LPARAM)error);
    SetLastError(error);
}

/* Safe under loader lock: no User32/Comctl calls and no waits for other threads. */
static void ReleaseLocalSubclass(void)
{
    /* Keep the local slot unavailable until all old ownership fields are cleared. */
    if (InterlockedCompareExchange(&g_local_registration, -1, 1) != 1)
        return;
    g_original_proc = NULL;
    g_subclassed_window = NULL;
    g_subclass_proc = NULL;
    g_subclass_thread = 0;
    g_moved_or_sized = FALSE;
    g_num_msgs_so_far = 0;
    InterlockedDecrement(&g_active_subclasses);
    InterlockedExchange(&g_local_registration, 0);
}

static void SubclassWindowDestroyed(HWND hwnd)
{
    if (hwnd == g_subclassed_window && GetCurrentThreadId() == g_subclass_thread)
        ReleaseLocalSubclass();
}

#ifndef ALLSNAP_TESTING
BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, PVOID reserved)
{
    (void)reserved;
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hinstDll = hinstDll;
    else if (fdwReason == DLL_PROCESS_DETACH)
        ReleaseLocalSubclass();
    return TRUE;
}
#endif

BOOL WINAPI SnapHookAll(HWND hwnd, UINT thread_id, OSVERSIONINFO os)
{
    HHOOK hook;
    if (g_hhook != NULL)
        return TRUE;
    SetLastError(ERROR_SUCCESS);
    hook = LifecycleInstallHook(WH_CALLWNDPROC, (HOOKPROC)CallWndProc, g_hinstDll, 0);
    if (hook == NULL) {
        DWORD error = LifecycleError();
        SetLastError(error);
        return FALSE;
    }
    g_hhook = hook;
    g_hWnd_app = hwnd;
    g_sounds_thread_id = thread_id;
    g_os = os;
    InterlockedExchange(&g_hook_stopping, FALSE);
    return TRUE;
}

BOOL WINAPI SnapUnHookAll(void)
{
    DWORD error;
    if (g_hhook == NULL)
        return TRUE;
    InterlockedExchange(&g_hook_stopping, TRUE);
    if (InterlockedCompareExchange(&g_active_subclasses, 0, 0) != 0) {
        InterlockedExchange(&g_hook_stopping, FALSE);
        SetLastError(ERROR_BUSY);
        return FALSE;
    }
    SetLastError(ERROR_SUCCESS);
    if (!LifecycleRemoveHook(g_hhook)) {
        error = LifecycleError();
        InterlockedExchange(&g_hook_stopping, FALSE);
        ReportLifecycleFailure(SNAP_DIAG_HOOK_REMOVE, error);
        return FALSE;
    }
    g_hhook = NULL;
    return TRUE;
}

BOOL WINAPI SnapCanUnHook(void)
{
    return InterlockedCompareExchange(&g_active_subclasses, 0, 0) == 0;
}

static BOOL TryUnSubclass(HWND hwnd)
{
    DWORD error;
    if (InterlockedCompareExchange(&g_local_registration, 0, 0) == 0)
        return TRUE;
    if (hwnd != g_subclassed_window) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    if (GetCurrentThreadId() != g_subclass_thread) {
        SetLastError(ERROR_INVALID_THREAD_ID);
        return FALSE;
    }
    if (!IsWindow(hwnd)) {
        ReleaseLocalSubclass();
        return TRUE;
    }
    SetLastError(ERROR_SUCCESS);
    if (!LifecycleRemoveSubclass(hwnd, (SUBCLASSPROC)SNAP_SUBCLASS_PROC, ALLSNAP_SUBCLASS_ID)) {
        error = LifecycleError();
        ReportLifecycleFailure(SNAP_DIAG_SUBCLASS_REMOVE, error);
        return FALSE;
    }
    ReleaseLocalSubclass();
    return TRUE;
}

void UnSubclass(HWND hwnd)
{
    TryUnSubclass(hwnd);
}

static BOOL TrySubclass(HWND hwnd)
{
    RECT winrect;
    DWORD error;
    if (hwnd == NULL || !GetWindowRect(hwnd, &winrect)) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    if (InterlockedCompareExchange(&g_hook_stopping, 0, 0)) {
        SetLastError(ERROR_BUSY);
        return FALSE;
    }
    if (InterlockedCompareExchange(&g_local_registration, 1, 0) != 0) {
        if (hwnd == g_subclassed_window && GetCurrentThreadId() == g_subclass_thread)
            return TRUE;
        SetLastError(ERROR_BUSY);
        return FALSE;
    }
    /* Reserve before installing, so shutdown cannot pass an in-flight registration. */
    InterlockedIncrement(&g_active_subclasses);
    if (InterlockedCompareExchange(&g_hook_stopping, 0, 0)) {
        ReleaseLocalSubclass();
        SetLastError(ERROR_BUSY);
        return FALSE;
    }
    SetLastError(ERROR_SUCCESS);
    if (!LifecycleInstallSubclass(hwnd, (SUBCLASSPROC)SNAP_SUBCLASS_PROC, ALLSNAP_SUBCLASS_ID, 0)) {
        error = LifecycleError();
        ReleaseLocalSubclass();
        ReportLifecycleFailure(SNAP_DIAG_SUBCLASS_INSTALL, error);
        return FALSE;
    }
    g_subclassed_window = hwnd;
    g_subclass_thread = GetCurrentThreadId();
    g_subclass_proc = (SUBCLASSPROC)SNAP_SUBCLASS_PROC;
    g_last_height = rectHeight(winrect);
    g_last_width = rectWidth(winrect);
    g_num_msgs_so_far = 0;
    g_moved_or_sized = FALSE;
    snapper_OnEnterSizeMove(hwnd);
    return TRUE;
}

void Subclass(HWND hwnd)
{
    TrySubclass(hwnd);
}

LRESULT WINAPI CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    /* A negative hook code must be forwarded without inspecting its payload. */
    if (nCode == HC_ACTION && lParam != 0) {
        PCWPSTRUCT message = (PCWPSTRUCT)lParam;
        HWND hwnd = message->hwnd;
        if (message->message == WM_ENTERSIZEMOVE) {
            if ((isSnapMdi() || !IS_MDI_CHILD(hwnd)) && isEnabled() && !IsZoomed(hwnd))
                Subclass(hwnd);
        } else if (message->message == WM_EXITSIZEMOVE) {
            UnSubclass(hwnd);
        }
    }
    /* The handle parameter is ignored; injected processes do not own the host hook. */
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

#ifdef ALLSNAP_TESTING
void SnapTest_LifecycleReset(void)
{
    ReleaseLocalSubclass();
    InterlockedExchange(&g_active_subclasses, 0);
    InterlockedExchange(&g_hook_stopping, FALSE);
    g_hhook = NULL;
    g_hWnd_app = NULL;
    g_lifecycle_ops = g_default_lifecycle_ops;
}
void SnapTest_SetLifecycleOps(const SNAP_LIFECYCLE_TEST_OPS * operations)
{
    g_lifecycle_ops = g_default_lifecycle_ops;
    if (operations == NULL) return;
    if (operations->install_hook) g_lifecycle_ops.install_hook = operations->install_hook;
    if (operations->remove_hook) g_lifecycle_ops.remove_hook = operations->remove_hook;
    if (operations->install_subclass) g_lifecycle_ops.install_subclass = operations->install_subclass;
    if (operations->remove_subclass) g_lifecycle_ops.remove_subclass = operations->remove_subclass;
}
BOOL SnapTest_Subclass(HWND hwnd) { return TrySubclass(hwnd); }
BOOL SnapTest_UnSubclass(HWND hwnd) { return TryUnSubclass(hwnd); }
void SnapTest_ProcessDetach(void) { ReleaseLocalSubclass(); }
void SnapTest_WindowDestroyed(HWND hwnd) { SubclassWindowDestroyed(hwnd); }
void SnapTest_SetExternalSubclassCount(LONG count)
{
    InterlockedExchange(&g_active_subclasses, count + (g_local_registration ? 1 : 0));
}
LONG SnapTest_ActiveSubclassCount(void) { return InterlockedCompareExchange(&g_active_subclasses, 0, 0); }
HWND SnapTest_SubclassWindow(void) { return g_subclassed_window; }
HHOOK SnapTest_HookHandle(void) { return g_hhook; }
LRESULT SnapTest_CallHook(int code, WPARAM wparam, LPARAM lparam) { return CallWndProc(code, wparam, lparam); }
#endif
BOOL window_size_changed(HWND hwnd){
	RECT win_rect;
	int new_width,new_height;
	BOOL res = FALSE;

	GetWindowRect(hwnd,&win_rect);
	new_width = rectWidth(win_rect);
	new_height = rectHeight(win_rect);

	res = ( (new_width != g_last_width) || (new_height!=g_last_height));

	g_last_width = new_width;
	g_last_height = new_height;

	return res;
}

BOOL avoid_aero_snap(HWND hwnd){
	if (IsZoomed(hwnd)){
		g_was_zoomed = TRUE;
		return TRUE;
	}
	else{
		if(g_was_zoomed){
			g_was_zoomed=FALSE;
			return TRUE;
		}
	}
	if(g_num_msgs_so_far++<NUM_MSG_TO_SKIP){
		return TRUE;
	}
	return FALSE;
}
// Subclass procedure 
#ifdef _WIN64
LRESULT APIENTRY SubclassProc64(
#else
LRESULT APIENTRY SubclassProc32(
#endif
	HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
)
{  

	switch(uMsg){

        case WM_NCDESTROY:
            SubclassWindowDestroyed(hwnd);
            break;

		//case WM_ENTERSIZEMOVE:
		//	g_moved_or_sized = TRUE;
#ifndef DBG_NO_MOVING
		case WM_MOVING:
			if (!window_size_changed(hwnd)){
			
				snapper_OnMoving(hwnd,(LPRECT)(lParam));
				if (!g_moved_or_sized){
					g_moved_or_sized= TRUE;
				}
				break;
			}
			break;
#endif
#ifndef DBG_NO_SIZING
		case WM_SIZING:
			if (wParam !=9){	
				snapper_OnSizing(hwnd,wParam,(LPRECT)(lParam));
				if (!g_moved_or_sized){
					g_moved_or_sized= TRUE;
				}
			}
			break;
#endif
	}

	if (	(	
				( (uMsg == WM_ACTIVATE)		&& (wParam == WA_INACTIVE))
			||	
				( (uMsg == WM_ACTIVATEAPP)	&& (wParam == FALSE)) //  x == FALSE or just !x?
			||  
				( uMsg == WM_EXITSIZEMOVE)
			) 
			
	//	&&	(g_subclassed_window != NULL)  //should this be an assertion?
	){	DBG_MSG(g_hWnd_app,DBGMSG_EXITSIZEMOVE);
		UnSubclass(hwnd);
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
} 

void WINAPI getGridSnap(gridsnap_settings_t  *p_grid_settings)
{
	*p_grid_settings = g_gridsnap;
}

void WINAPI setGridSnap(gridsnap_settings_t const * p_grid_settings)
{
	gridsnap_settings_t validated;
	gridsnap_setting_t * dimensions[2];
	int i;
	if (!p_grid_settings){
		return;
	}
	validated = *p_grid_settings;
	dimensions[0] = &validated.h;
	dimensions[1] = &validated.v;
	for (i = 0; i < 2; ++i){
		gridsnap_setting_t * setting = dimensions[i];
		if (setting->type < GRIDTYPE_EVEN || setting->type > GRIDTYPE_SINGLE){
			setting->type = GRIDTYPE_EVEN;
			setting->val = 0;
			setting->enabled = FALSE;
		}else if (setting->val < 0 || (setting->type == GRIDTYPE_EVEN && setting->val > 100)){
			setting->val = 0;
			setting->enabled = FALSE;
		}
	}
	g_gridsnap = validated;
}

BOOL WINAPI isEnabled(void){
	return	g_enabled;
}

void WINAPI setEnabled(BOOL enabled){
	g_enabled = enabled;

	/*if (enabled){
		SnapHookAll(g_hWnd_app,g_thread_id);
	}
	else{
		UnHookAll();
	}*/
}

BOOL WINAPI isNoisy(void){
	return	g_is_noisy;
}

void WINAPI setNoisy(BOOL is_noisy){
	g_is_noisy = is_noisy;
}

BOOL WINAPI isCroppingTop(void){
	return	g_is_cropping_top;
}

void WINAPI setCroppingTop(BOOL is_cropping_top){
	g_is_cropping_top = is_cropping_top;
}

BOOL WINAPI isSnapMdi(void){
	return	g_snap_mdi;
}
void WINAPI setSnapMdi(BOOL snap_mdi){
	g_snap_mdi = snap_mdi;
}


BOOL WINAPI isDisableToggle(void){
	return g_is_disable_toggled;
}
void WINAPI setDisableToggle(BOOL is_disable_toggle){
	g_is_disable_toggled = is_disable_toggle;
}



void WINAPI setWinThresh(int thresh){
	g_win_thresh = thresh;
}

void WINAPI setWindowSpacing(int spacing){
    g_window_spacing = spacing < 0 ? 0 : (spacing > 1000 ? 1000 : spacing);
}

int WINAPI getWindowSpacing(void){
    return g_window_spacing;
}

void WINAPI setScreenThresh(int thresh){
	g_screen_thresh = thresh;
}

int  WINAPI getWinThresh(void){
	return g_win_thresh;
}
int  WINAPI getScreenThresh(void){
	return g_screen_thresh;
}


void WINAPI setCropTop(int crop_top){
	g_crop_top = crop_top;
}

int  WINAPI getCropTop(void){
	return g_crop_top;
}

UINT WINAPI getSnapType(void){
	return g_snap_type;
}
void WINAPI setSnapType(UINT snap_type){
	g_snap_type = snap_type;
}

UINT WINAPI getToggleKey(void){
	return g_toggle_key;
}
void WINAPI setToggleKey(UINT toggle_key){
	g_toggle_key = toggle_key;
}
UINT WINAPI getCenterKey(void){
	return g_center_key;
}
void WINAPI setCenterKey(UINT center_key){
	g_center_key = center_key;
}
UINT WINAPI getEqualKey(void){
	return g_equal_key;
}
void WINAPI setEqualKey(UINT equal_key){
	g_equal_key = equal_key;
}
BOOL WINAPI isSnappingInsides(void){
	return	g_snap_insides;
}
void WINAPI setSnappingInsides(BOOL snap_insides){
	g_snap_insides = snap_insides;
}

BOOL WINAPI isKeptToScreen(void){
	return	g_kept_to_screen;
}

void WINAPI setKeptToScreen(BOOL kept_to_screen){
	g_kept_to_screen = kept_to_screen;
}

void WINAPI setSkinnedClasses(TCHAR * sz,int len){
	if (len > MAX_CLASSNAME_LIST_LENGTH){
		return;
	}
	lstrcpyn(g_skinned_classes,sz,MAX_CLASSNAME_LIST_LENGTH);
}

TCHAR *  getSkinnedClasses(void){
	return g_skinned_classes;
}

void WINAPI setIgnoredClasses(TCHAR * sz,int len){	
	if (len > MAX_CLASSNAME_LIST_LENGTH){
		return;
	}

	lstrcpyn(g_ignored_classes,sz,MAX_CLASSNAME_LIST_LENGTH);
}

TCHAR *  getIgnoredClasses(void){
	return g_ignored_classes;
}

void WINAPI setCroppingRgn(BOOL cropping_rgn){
	g_cropping_rgn = cropping_rgn;
}

BOOL WINAPI isCroppingRgn(void){
	return	g_cropping_rgn;
}
