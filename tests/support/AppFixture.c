/* Boundary adapters for settings tests. No tray icon, sound, or desktop hook. */
#include "stdafx.h"
#include <commctrl.h>
#include <shellapi.h>
#include "snap_sounds.h"

HWND g_hWnd = NULL;
HINSTANCE g_hInst = NULL;
BOOL g_isXP = TRUE;
HICON g_hiPlay = NULL;
HICON g_hiStop = NULL;

static TCHAR test_sound_paths[NUM_SOUNDS][MAX_PATH];
static int test_message_count;
static int test_notify_count;
static DWORD test_notify_operation[32];
static NOTIFYICONDATA test_notify_data[32];
static BOOL test_notify_success;
void SnapTaskbar_TestReset(void);

void InitCmnCtls(void)
{
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&controls);
}

void TestApp_Initialize(void)
{
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)&TestApp_Initialize, &g_hInst);
    InitCmnCtls();
    SnapTaskbar_TestReset();
    test_message_count = 0;
    test_notify_count = 0;
    test_notify_success = TRUE;
    ZeroMemory(test_sound_paths, sizeof(test_sound_paths));
}

BOOL SnapTaskbar_TestNotifyIcon(DWORD operation, PNOTIFYICONDATA data)
{
    if (test_notify_count < 32) {
        test_notify_operation[test_notify_count] = operation;
        test_notify_data[test_notify_count] = *data;
    }
    ++test_notify_count;
    if (!test_notify_success) SetLastError(ERROR_ACCESS_DENIED);
    return test_notify_success;
}
int TestApp_NotifyCount(void) { return test_notify_count; }
DWORD TestApp_NotifyOperation(int index) { return test_notify_operation[index]; }
const NOTIFYICONDATA * TestApp_NotifyData(int index) { return &test_notify_data[index]; }
void TestApp_SetNotifySuccess(BOOL success) { test_notify_success = success; }
void tips_show(HWND control) { (void)control; }

int SnapSettings_TestMessageBox(HWND owner, LPCTSTR text, LPCTSTR caption, UINT flags)
{
    (void)owner;
    (void)text;
    (void)caption;
    (void)flags;
    ++test_message_count;
    return IDOK;
}
int TestApp_MessageCount(void) { return test_message_count; }

void SnapSounds_setPath(enum SNAP_SOUNDS sound, LPCTSTR path)
{
    if (sound >= SOUND_SNAP && sound < NUM_SOUNDS)
        lstrcpyn(test_sound_paths[sound], path, MAX_PATH);
}
LPTSTR SnapSounds_getPath(enum SNAP_SOUNDS sound) { return test_sound_paths[sound]; }
void SnapSounds_Test(LPCTSTR path) { (void)path; }
void SnapSounds_StopTest(void) { }
void WINAPI setSnapSound(LPCTSTR path, int length)
{
    (void)length;
    SnapSounds_setPath(SOUND_SNAP, path);
}
LPTSTR WINAPI getSnapSound(void) { return SnapSounds_getPath(SOUND_SNAP); }
void WINAPI setUnsnapSound(LPCTSTR path, int length)
{
    (void)length;
    SnapSounds_setPath(SOUND_UNSNAP, path);
}
LPTSTR WINAPI getUnsnapSound(void) { return SnapSounds_getPath(SOUND_UNSNAP); }
