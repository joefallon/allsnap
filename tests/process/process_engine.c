/* No alternate lifecycle algorithm: adapt commands to the production engine. */
#include "stdafx.h"
#include <commctrl.h>
#define SNAPLIBAPI
#include "snap_lib.h"
#include "snap_lifecycle_test.h"
#include "ProcessProtocol.h"

static HANDLE reservation_entered;
static HANDLE reservation_release;
static BOOL hold_install;

static HHOOK WINAPI InstallHook(int kind, HOOKPROC procedure, HINSTANCE instance, DWORD thread)
{
    (void)kind; (void)procedure; (void)instance; (void)thread;
    return (HHOOK)(UINT_PTR)1;
}

static BOOL WINAPI RemoveHook(HHOOK hook)
{
    (void)hook;
    return TRUE;
}

static BOOL WINAPI InstallSubclass(HWND window, SUBCLASSPROC procedure, UINT_PTR id, DWORD_PTR data)
{
    if (hold_install) {
        hold_install = FALSE;
        SetEvent(reservation_entered);
        if (WaitForSingleObject(reservation_release, PROCESS_TEST_TIMEOUT) != WAIT_OBJECT_0) {
            SetLastError(ERROR_TIMEOUT);
            return FALSE;
        }
    }
    return SetWindowSubclass(window, procedure, id, data);
}

BOOL __cdecl SnapProcessDispatch(DWORD command, HWND window, HANDLE entered, HANDLE release,
    struct ProcessResult * result)
{
    BOOL success = TRUE;
    DWORD error;
    SetLastError(ERROR_SUCCESS);
    switch (command) {
    case ProcessReady: {
        SNAP_LIFECYCLE_TEST_OPS operations = { InstallHook, RemoveHook, InstallSubclass, NULL };
        SnapTest_SetLifecycleOps(&operations);
        reservation_entered = entered;
        reservation_release = release;
        break;
    }
    case ProcessStartHook: {
        OSVERSIONINFO version = {0};
        version.dwOSVersionInfoSize = sizeof(version);
        setNoisy(FALSE);
        setSnapType(SNAPT_NONE);
        setKeptToScreen(FALSE);
        success = SnapHookAll(window, 0, version);
        break;
    }
    case ProcessRegister:
        success = SnapTest_Subclass(window);
        break;
    case ProcessFinishMove:
        SendMessage(window, WM_EXITSIZEMOVE, 0, 0);
        break;
    case ProcessStopHook:
        success = SnapUnHookAll();
        break;
    case ProcessArmReservation:
        hold_install = TRUE;
        break;
    case ProcessQuery:
        break;
    case ProcessEnterHook:
        SnapTest_CallHook(-1, 0, 0);
        break;
    default:
        SetLastError(ERROR_INVALID_FUNCTION);
        success = FALSE;
        break;
    }
    error = GetLastError();
    result->success = success;
    result->error = error;
    result->active = SnapTest_ActiveSubclassCount();
    result->canStop = SnapCanUnHook();
    result->localOwner = SnapTest_SubclassWindow() != NULL;
    result->moduleMapped = TRUE;
    return success;
}
