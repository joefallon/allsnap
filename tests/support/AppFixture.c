/* Boundary adapters for settings tests. No system tray icon or desktop hook. */
#include "stdafx.h"
#include <commctrl.h>
#include <shellapi.h>

HWND g_hWnd = NULL;
HINSTANCE g_hInst = NULL;
BOOL g_isXP = TRUE;

static int test_message_count;
static int test_notify_count;
static DWORD test_notify_operation[32];
static NOTIFYICONDATA test_notify_data[32];
static BOOL test_notify_success;
static BOOL test_notify_failure_enabled;
static DWORD test_notify_failure_operation;
static int test_tip_count;
static HWND test_tip_control;
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
    test_notify_failure_enabled = FALSE;
    test_tip_count = 0;
    test_tip_control = NULL;
}

BOOL SnapTaskbar_TestNotifyIcon(DWORD operation, PNOTIFYICONDATA data)
{
    if (test_notify_count < 32) {
        test_notify_operation[test_notify_count] = operation;
        test_notify_data[test_notify_count] = *data;
    }
    ++test_notify_count;
    if (!test_notify_success || (test_notify_failure_enabled && operation == test_notify_failure_operation)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return TRUE;
}
int TestApp_NotifyCount(void) { return test_notify_count; }
DWORD TestApp_NotifyOperation(int index) { return test_notify_operation[index]; }
const NOTIFYICONDATA * TestApp_NotifyData(int index) { return &test_notify_data[index]; }
void TestApp_SetNotifySuccess(BOOL success) { test_notify_success = success; }
void TestApp_SetNotifyFailure(DWORD operation)
{
    test_notify_failure_enabled = TRUE;
    test_notify_failure_operation = operation;
}
void TestApp_ClearNotifyFailure(void) { test_notify_failure_enabled = FALSE; }
void tips_show(HWND control) { ++test_tip_count; test_tip_control = control; }
int TestApp_TipCount(void) { return test_tip_count; }
HWND TestApp_TipControl(void) { return test_tip_control; }

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
