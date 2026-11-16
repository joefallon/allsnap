#ifndef SNAP_LIFECYCLE_TEST_H
#define SNAP_LIFECYCLE_TEST_H

#ifdef ALLSNAP_TESTING
#include <windows.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SNAP_LIFECYCLE_TEST_OPS {
    HHOOK (WINAPI *install_hook)(int, HOOKPROC, HINSTANCE, DWORD);
    BOOL (WINAPI *remove_hook)(HHOOK);
    BOOL (WINAPI *install_subclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
    BOOL (WINAPI *remove_subclass)(HWND, SUBCLASSPROC, UINT_PTR);
} SNAP_LIFECYCLE_TEST_OPS;

void SnapTest_LifecycleReset(void);
void SnapTest_SetLifecycleOps(const SNAP_LIFECYCLE_TEST_OPS * operations);
BOOL SnapTest_Subclass(HWND hwnd);
BOOL SnapTest_UnSubclass(HWND hwnd);
void SnapTest_ProcessDetach(void);
void SnapTest_WindowDestroyed(HWND hwnd);
void SnapTest_SetExternalSubclassCount(LONG count);
LONG SnapTest_ActiveSubclassCount(void);
HWND SnapTest_SubclassWindow(void);
HHOOK SnapTest_HookHandle(void);
LRESULT SnapTest_CallHook(int code, WPARAM wparam, LPARAM lparam);

#ifdef __cplusplus
}
#endif
#endif
#endif
