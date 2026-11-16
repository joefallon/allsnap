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

#define OWNER_SLOT_COUNT 64
#define OWNER_GENERATION_MASK 0x3fffffffULL
#define OWNER_READY (1ULL << 62)
#define OWNER_RELEASING (2ULL << 62)
#define OWNER_PHASE_MASK (3ULL << 62)
typedef struct OWNER_IDENTITY {
    DWORD process_id;
    ULONGLONG process_birth;
    ULONGLONG thread_birth;
} OWNER_IDENTITY;
typedef struct OWNER_SLOT {
    __declspec(align(8)) volatile LONGLONG claim;
    OWNER_IDENTITY identity;
} OWNER_SLOT;

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

OWNER_SLOT g_registration_owners[OWNER_SLOT_COUNT] = {0};
volatile LONG g_hook_stopping = FALSE;
int g_window_spacing = 0;

TCHAR	g_skinned_classes[MAX_CLASSNAME_LENGTH * MAX_NUM_CLASS_NAMES] =
  _T("icoCTactTrilly;Winamp v1.x;ConsoleWindowClass;Winamp PE;Winamp EQ;Winamp MB;BaseWindow_RootWnd");	
TCHAR	g_ignored_classes[MAX_CLASSNAME_LENGTH * MAX_NUM_CLASS_NAMES] =
  _T("Progman;IDEOwner;NeilShadow");
#pragma data_seg()

HHOOK g_hhook = NULL;
__declspec(align(8)) volatile LONGLONG g_local_claim = 0;
int g_local_owner_slot = -1;
LONGLONG g_local_owner_token = 0;
OWNER_IDENTITY g_local_identity = {0};
volatile LONG g_module_pinned = FALSE;
DWORD g_subclass_thread = 0;
BOOL g_subclass_installed = FALSE;
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

static BOOL WINAPI PinLifecycleModule(void);
static int WINAPI ProbeOwnerNative(DWORD, DWORD, ULONGLONG, ULONGLONG, BOOL);

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
static BOOL WINAPI TestPinSucceeded(void) { return TRUE; }
#ifdef ALLSNAP_PROCESS_TESTING
#define DefaultPinModule PinLifecycleModule
#else
#define DefaultPinModule TestPinSucceeded
#endif
static LONG g_test_external_count = 0;
static const SNAP_LIFECYCLE_TEST_OPS g_default_lifecycle_ops = {
    TestHookNotSupported, TestUnhookNotSupported, SetWindowSubclass, RemoveWindowSubclass,
    DefaultPinModule, ProbeOwnerNative
};
static SNAP_LIFECYCLE_TEST_OPS g_lifecycle_ops = {
    TestHookNotSupported, TestUnhookNotSupported, SetWindowSubclass, RemoveWindowSubclass,
    DefaultPinModule, ProbeOwnerNative
};
#define LifecycleInstallHook g_lifecycle_ops.install_hook
#define LifecycleRemoveHook g_lifecycle_ops.remove_hook
#define LifecycleInstallSubclass g_lifecycle_ops.install_subclass
#define LifecycleRemoveSubclass g_lifecycle_ops.remove_subclass
#define LifecyclePinModule g_lifecycle_ops.pin_module
#define LifecycleProbeOwner g_lifecycle_ops.probe_owner
#else
#define LifecycleInstallHook SetWindowsHookEx
#define LifecycleRemoveHook UnhookWindowsHookEx
#define LifecycleInstallSubclass SetWindowSubclass
#define LifecycleRemoveSubclass RemoveWindowSubclass
#define LifecyclePinModule PinLifecycleModule
#define LifecycleProbeOwner ProbeOwnerNative
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

static LONGLONG ReadClaim(volatile LONGLONG * claim)
{
    return InterlockedCompareExchange64(claim,0,0);
}
static DWORD ClaimThread(LONGLONG claim) { return (DWORD)(ULONGLONG)claim; }
static LONGLONG FreeClaim(LONGLONG claim)
{
    return (LONGLONG)((ULONGLONG)claim & (OWNER_GENERATION_MASK << 32));
}
static LONGLONG NextClaim(LONGLONG previous, DWORD thread_id)
{
    ULONGLONG generation = ((ULONGLONG)previous >> 32) & OWNER_GENERATION_MASK;
    if (generation == OWNER_GENERATION_MASK) return 0; /* Never permit generation ABA. */
    return (LONGLONG)(((generation + 1) << 32) | thread_id);
}
static BOOL IsPublished(LONGLONG claim)
{
    return ((ULONGLONG)claim & OWNER_PHASE_MASK) == OWNER_READY;
}
static ULONGLONG BirthValue(FILETIME time)
{
    return ((ULONGLONG)time.dwHighDateTime << 32) | time.dwLowDateTime;
}
static BOOL CurrentIdentity(OWNER_IDENTITY * identity)
{
    FILETIME process_birth,thread_birth,exit_time,kernel,user;
    if (!GetProcessTimes(GetCurrentProcess(),&process_birth,&exit_time,&kernel,&user)
        || !GetThreadTimes(GetCurrentThread(),&thread_birth,&exit_time,&kernel,&user)) return FALSE;
    identity->process_id = GetCurrentProcessId();
    identity->process_birth = BirthValue(process_birth);
    identity->thread_birth = BirthValue(thread_birth);
    return TRUE;
}

static BOOL WINAPI PinLifecycleModule(void)
{
    HMODULE module;
    return GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        (LPCTSTR)(ULONG_PTR)&CallWndProc,&module);
}
static BOOL EnsureLifecyclePinned(void)
{
    if (InterlockedCompareExchange(&g_module_pinned,0,0)) return TRUE;
    if (!LifecyclePinModule()) return FALSE;
    InterlockedExchange(&g_module_pinned,TRUE);
    return TRUE;
}

/* Unknown access is not proof of death. Full birth times distinguish reused IDs. */
static int WINAPI ProbeOwnerNative(DWORD process_id,DWORD thread_id,ULONGLONG process_birth,
    ULONGLONG thread_birth,BOOL published)
{
    HANDLE process = NULL,thread;
    FILETIME birth,exit_time,kernel,user;
    DWORD wait;
    if (published) {
        process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,FALSE,process_id);
        if (!process) {
            return GetLastError() == ERROR_INVALID_PARAMETER ? SNAP_OWNER_DEAD : SNAP_OWNER_UNKNOWN;
        }
        wait = WaitForSingleObject(process,0);
        if (wait == WAIT_OBJECT_0) { CloseHandle(process); return SNAP_OWNER_DEAD; }
        if (wait != WAIT_TIMEOUT || !GetProcessTimes(process,&birth,&exit_time,&kernel,&user)) {
            CloseHandle(process); return SNAP_OWNER_UNKNOWN;
        }
        if (BirthValue(birth) != process_birth) { CloseHandle(process); return SNAP_OWNER_DEAD; }
        CloseHandle(process);
    }
    thread = OpenThread(SYNCHRONIZE | (published ? THREAD_QUERY_LIMITED_INFORMATION : 0),FALSE,thread_id);
    if (!thread) return GetLastError() == ERROR_INVALID_PARAMETER ? SNAP_OWNER_DEAD : SNAP_OWNER_UNKNOWN;
    wait = WaitForSingleObject(thread,0);
    if (wait == WAIT_OBJECT_0) { CloseHandle(thread); return SNAP_OWNER_DEAD; }
    if (wait != WAIT_TIMEOUT) { CloseHandle(thread); return SNAP_OWNER_UNKNOWN; }
    if (published) {
        typedef DWORD (WINAPI *GET_THREAD_PROCESS)(HANDLE);
        GET_THREAD_PROCESS get_process = (GET_THREAD_PROCESS)GetProcAddress(
            GetModuleHandle(_T("kernel32.dll")),"GetProcessIdOfThread");
        DWORD actual_process;
        if (!get_process || !(actual_process = get_process(thread))
            || !GetThreadTimes(thread,&birth,&exit_time,&kernel,&user)) {
            CloseHandle(thread); return SNAP_OWNER_UNKNOWN;
        }
        if (actual_process != process_id || BirthValue(birth) != thread_birth) {
            CloseHandle(thread); return SNAP_OWNER_DEAD;
        }
    }
    CloseHandle(thread);
    return SNAP_OWNER_ALIVE;
}

static int ProbeClaim(LONGLONG claim,const OWNER_IDENTITY * identity)
{
    BOOL published = IsPublished(claim);
    return LifecycleProbeOwner(published ? identity->process_id : 0,ClaimThread(claim),
        published ? identity->process_birth : 0,published ? identity->thread_birth : 0,published);
}
static void ReleaseOwnerSlot(int slot,LONGLONG token)
{
    if (slot >= 0 && slot < OWNER_SLOT_COUNT)
        InterlockedCompareExchange64(&g_registration_owners[slot].claim,FreeClaim(token),token);
}
static LONG RegisteredOwners(void)
{
    LONG count = 0;
    int index;
    for (index = 0; index < OWNER_SLOT_COUNT; ++index) {
        OWNER_SLOT * slot = &g_registration_owners[index];
        LONGLONG claim = ReadClaim(&slot->claim);
        OWNER_IDENTITY identity = {0};
        if (!ClaimThread(claim)) continue;
        if (IsPublished(claim)) identity = slot->identity;
        MemoryBarrier();
        if (ReadClaim(&slot->claim) != claim) { ++count; continue; }
        if (ProbeClaim(claim,&identity) == SNAP_OWNER_DEAD) {
            if (InterlockedCompareExchange64(&slot->claim,FreeClaim(claim),claim) == claim) continue;
        }
        ++count;
    }
#ifdef ALLSNAP_TESTING
    count += g_test_external_count;
#endif
    return count;
}
static BOOL ReserveOwnerSlot(const OWNER_IDENTITY * identity,int * owner_slot,LONGLONG * owner_token)
{
    int index;
    RegisteredOwners();
    for (index = 0; index < OWNER_SLOT_COUNT; ++index) {
        OWNER_SLOT * slot = &g_registration_owners[index];
        LONGLONG previous = ReadClaim(&slot->claim),claim;
        if (ClaimThread(previous)) continue;
        claim = NextClaim(previous,GetCurrentThreadId());
        if (!claim || InterlockedCompareExchange64(&slot->claim,claim,previous) != previous) continue;
        slot->identity = *identity;
        MemoryBarrier();
        *owner_token = (LONGLONG)((ULONGLONG)claim | OWNER_READY);
        if (InterlockedCompareExchange64(&slot->claim,*owner_token,claim) != claim) {
            SetLastError(ERROR_BUSY); return FALSE;
        }
        *owner_slot = index;
        return TRUE;
    }
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}
static void ClearLocalFields(void)
{
    g_local_owner_slot = -1;
    g_local_owner_token = 0;
    ZeroMemory(&g_local_identity,sizeof(g_local_identity));
    g_original_proc = NULL;
    g_subclassed_window = NULL;
    g_subclass_proc = NULL;
    g_subclass_thread = 0;
    g_subclass_installed = FALSE;
    g_moved_or_sized = FALSE;
    g_num_msgs_so_far = 0;
}
/* Loader-lock safe: exact-token release and local writes only. Never probe or wait here. */
static void ReleaseLocalSubclass(void)
{
    LONGLONG claim = ReadClaim(&g_local_claim),releasing;
    if (!ClaimThread(claim) || ((ULONGLONG)claim & OWNER_PHASE_MASK) == OWNER_RELEASING) return;
    releasing = (LONGLONG)(((ULONGLONG)claim & ~OWNER_PHASE_MASK) | OWNER_RELEASING);
    if (InterlockedCompareExchange64(&g_local_claim,releasing,claim) != claim) return;
    /* Before publication the shared-slot reference may be incomplete; a dead owner is reaped later. */
    if (IsPublished(claim)) ReleaseOwnerSlot(g_local_owner_slot,g_local_owner_token);
    ClearLocalFields();
    InterlockedExchange64(&g_local_claim,FreeClaim(claim));
}
static BOOL ReconcileLocalOwner(void)
{
    LONGLONG claim = ReadClaim(&g_local_claim),replacement;
    OWNER_IDENTITY identity = {0};
    BOOL lost_slot = FALSE;
    if (!ClaimThread(claim)) return TRUE;
    if (IsPublished(claim)) {
        identity = g_local_identity;
        lost_slot = g_local_owner_slot < 0 || g_local_owner_slot >= OWNER_SLOT_COUNT
            || ReadClaim(&g_registration_owners[g_local_owner_slot].claim) != g_local_owner_token;
    }
    MemoryBarrier();
    if (ReadClaim(&g_local_claim) != claim) return FALSE;
    if (!lost_slot && ProbeClaim(claim,&identity) != SNAP_OWNER_DEAD) return FALSE;
    /* The recovery claimant publishes its own TID before touching abandoned local fields. */
    replacement = NextClaim(FreeClaim(claim),GetCurrentThreadId());
    if (!replacement || InterlockedCompareExchange64(&g_local_claim,replacement,claim) != claim) return FALSE;
    if (IsPublished(claim)) ReleaseOwnerSlot(g_local_owner_slot,g_local_owner_token);
    ClearLocalFields();
    InterlockedExchange64(&g_local_claim,FreeClaim(replacement));
    return TRUE;
}
static void SubclassWindowDestroyed(HWND hwnd)
{
    if (hwnd == g_subclassed_window && GetCurrentThreadId() == g_subclass_thread)
        ReleaseLocalSubclass();
}

#if !defined(ALLSNAP_TESTING) || defined(ALLSNAP_PROCESS_TESTING)
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
    if (!EnsureLifecyclePinned()) return FALSE;
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
    if (RegisteredOwners() != 0) {
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
    return RegisteredOwners() == 0;
}

static BOOL TryUnSubclass(HWND hwnd)
{
    DWORD error;
    if (!ClaimThread(ReadClaim(&g_local_claim)))
        return TRUE;
    if (hwnd != g_subclassed_window) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    if (GetCurrentThreadId() != g_subclass_thread) {
        SetLastError(ERROR_INVALID_THREAD_ID);
        return FALSE;
    }
    if (!g_subclass_installed) {
        SetLastError(ERROR_BUSY);
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
    OWNER_IDENTITY identity;
    LONGLONG previous,claim,published;
    if (!EnsureLifecyclePinned()) {
        error = LifecycleError();
        ReportLifecycleFailure(SNAP_DIAG_SUBCLASS_INSTALL,error);
        return FALSE;
    }
    if (hwnd == NULL || !GetWindowRect(hwnd,&winrect)) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    if (InterlockedCompareExchange(&g_hook_stopping,0,0)) {
        SetLastError(ERROR_BUSY); return FALSE;
    }
    if (!ReconcileLocalOwner()) {
        if (g_subclass_installed && hwnd == g_subclassed_window && GetCurrentThreadId() == g_subclass_thread)
            return TRUE;
        SetLastError(ERROR_BUSY); return FALSE;
    }
    if (!CurrentIdentity(&identity)) return FALSE;
    previous = ReadClaim(&g_local_claim);
    claim = NextClaim(previous,GetCurrentThreadId());
    if (!claim || ClaimThread(previous)
        || InterlockedCompareExchange64(&g_local_claim,claim,previous) != previous) {
        SetLastError(ERROR_BUSY); return FALSE;
    }
    if (!ReserveOwnerSlot(&identity,&g_local_owner_slot,&g_local_owner_token)) {
        error = GetLastError();
        ReleaseLocalSubclass();
        SetLastError(error); return FALSE;
    }
    g_local_identity = identity;
    g_subclassed_window = hwnd;
    g_subclass_thread = GetCurrentThreadId();
    g_subclass_proc = (SUBCLASSPROC)SNAP_SUBCLASS_PROC;
    published = (LONGLONG)((ULONGLONG)claim | OWNER_READY);
    if (InterlockedCompareExchange64(&g_local_claim,published,claim) != claim) {
        ReleaseOwnerSlot(g_local_owner_slot,g_local_owner_token);
        SetLastError(ERROR_BUSY); return FALSE;
    }
    /* Publication reserves admission before any native registration side effect. */
    if (InterlockedCompareExchange(&g_hook_stopping,0,0)) {
        ReleaseLocalSubclass(); SetLastError(ERROR_BUSY); return FALSE;
    }
    SetLastError(ERROR_SUCCESS);
    if (!LifecycleInstallSubclass(hwnd,(SUBCLASSPROC)SNAP_SUBCLASS_PROC,ALLSNAP_SUBCLASS_ID,0)) {
        error = LifecycleError();
        ReleaseLocalSubclass();
        ReportLifecycleFailure(SNAP_DIAG_SUBCLASS_INSTALL,error);
        return FALSE;
    }
    g_subclass_installed = TRUE;
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
    if (!EnsureLifecyclePinned()) return CallNextHookEx(NULL,nCode,wParam,lParam);
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
    g_test_external_count = 0;
#ifndef ALLSNAP_PROCESS_TESTING
    InterlockedExchange(&g_hook_stopping, FALSE);
    InterlockedExchange(&g_module_pinned, FALSE);
#endif
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
    if (operations->pin_module) g_lifecycle_ops.pin_module = operations->pin_module;
    if (operations->probe_owner) g_lifecycle_ops.probe_owner = operations->probe_owner;
}
BOOL SnapTest_Subclass(HWND hwnd) { return TrySubclass(hwnd); }
BOOL SnapTest_UnSubclass(HWND hwnd) { return TryUnSubclass(hwnd); }
void SnapTest_ProcessDetach(void) { ReleaseLocalSubclass(); }
void SnapTest_WindowDestroyed(HWND hwnd) { SubclassWindowDestroyed(hwnd); }
void SnapTest_SetExternalSubclassCount(LONG count)
{
    g_test_external_count = count > 0 ? count : 0;
}
LONG SnapTest_ActiveSubclassCount(void) { return RegisteredOwners(); }
HWND SnapTest_SubclassWindow(void) { return g_subclassed_window; }
HHOOK SnapTest_HookHandle(void) { return g_hhook; }
LRESULT SnapTest_CallHook(int code, WPARAM wparam, LPARAM lparam) { return CallWndProc(code, wparam, lparam); }
void SnapTest_CorruptLocalOwnerBirth(void)
{
    if (g_local_owner_slot >= 0 && g_local_owner_slot < OWNER_SLOT_COUNT)
        g_registration_owners[g_local_owner_slot].identity.process_birth = 0;
}
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
