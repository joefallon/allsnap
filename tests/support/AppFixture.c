/* Boundary adapters for settings tests. No tray icon, sound, or desktop hook. */
#include "stdafx.h"
#include <commctrl.h>
#include "snap_sounds.h"

HWND g_hWnd = NULL;
HINSTANCE g_hInst = NULL;
BOOL g_isXP = TRUE;
HICON g_hiPlay = NULL;
HICON g_hiStop = NULL;

static BOOL test_icon_hidden;
static TCHAR test_sound_paths[NUM_SOUNDS][MAX_PATH];
static int test_message_count;

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
    test_icon_hidden = FALSE;
    test_message_count = 0;
    ZeroMemory(test_sound_paths, sizeof(test_sound_paths));
}

BOOL WINAPI isIconHidden(void) { return test_icon_hidden; }
void WINAPI setIconHidden(BOOL hidden) { test_icon_hidden = hidden; }
BOOL ResetTaskbarIcon(void) { return TRUE; }
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
