#pragma once

#include "EngineFixture.h"
#include <commctrl.h>
#include <string>
#include <vector>

extern "C" {
#include "resource.h"
#include "snap_ini.h"
#include "snap_taskbar.h"
extern HINSTANCE g_hInst;
void TestApp_Initialize(void);
int TestApp_MessageCount(void);
void SnapIni_TestSetPath(LPCTSTR path);
INT_PTR CALLBACK PrefsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SoundsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AdvProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK GridProc(HWND, UINT, WPARAM, LPARAM);
HWND DoPropertySheet(HWND);
void SnapSettings_TestSetSheetCreator(INT_PTR (CALLBACK *)(LPCPROPSHEETHEADER));
}

namespace AllSnapTests
{
    class SettingsFixture
    {
        const int originalCropTop = getCropTop();
        const UINT originalCenterKey = getCenterKey();
        const UINT originalEqualKey = getEqualKey();
        const std::wstring originalIgnored = getIgnoredClasses();
        const std::wstring originalSkinned = getSkinnedClasses();
        HWND parent = nullptr;
        WCHAR iniPath[MAX_PATH] = {};
        std::vector<HWND> pages;
        static INT_PTR CALLBACK CreateHiddenPreferences(LPCPROPSHEETHEADER sheet)
        {
            const PROPSHEETPAGE & page = sheet->ppsp[0];
            return reinterpret_cast<INT_PTR>(CreateDialogParamW(page.hInstance,
                page.pszTemplate, sheet->hwndParent, page.pfnDlgProc, page.lParam));
        }
        static PFNPROPSHEETCALLBACK & OriginalSheetCallback()
        {
            static PFNPROPSHEETCALLBACK callback = nullptr;
            return callback;
        }
        static int CALLBACK HideNativeSheet(HWND sheet, UINT message, LPARAM parameter)
        {
            if (OriginalSheetCallback()) OriginalSheetCallback()(sheet, message, parameter);
            if (message == PSCB_INITIALIZED) {
                SetWindowLongPtrW(sheet, GWL_EXSTYLE, GetWindowLongPtrW(sheet, GWL_EXSTYLE) |
                    WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
                ShowWindow(sheet, SW_HIDE);
            }
            return 0;
        }
        static INT_PTR CALLBACK CreateNativeSheet(LPCPROPSHEETHEADER header)
        {
            PROPSHEETHEADER sheet = *header;
            OriginalSheetCallback() = sheet.pfnCallback;
            sheet.pfnCallback = HideNativeSheet;
            HWND window = reinterpret_cast<HWND>(PropertySheetW(&sheet));
            OriginalSheetCallback() = nullptr;
            if (IsWindow(window)) ShowWindow(window, SW_HIDE);
            return reinterpret_cast<INT_PTR>(window);
        }
    public:
        SettingsFixture()
        {
            ResetEngine();
            setCropTop(3);
            TestApp_Initialize();
            WCHAR temporaryDirectory[MAX_PATH] = {};
            Assert::IsTrue(GetTempPathW(MAX_PATH, temporaryDirectory) != 0);
            Assert::IsTrue(GetTempFileNameW(temporaryDirectory, L"asn", 0, iniPath) != 0);
            SnapIni_TestSetPath(iniPath);
            parent = CreateWindowExW(0, L"STATIC", L"allSnap settings test", WS_OVERLAPPED,
                0, 0, 400, 400, nullptr, nullptr, g_hInst, nullptr);
            Assert::IsNotNull(parent);
        }

        ~SettingsFixture()
        {
            for (HWND page : pages)
                if (IsWindow(page)) DestroyWindow(page);
            if (IsWindow(parent)) DestroyWindow(parent);
            SnapIni_TestSetPath(nullptr);
            WritePrivateProfileStringW(nullptr, nullptr, nullptr, iniPath);
            DeleteFileW(iniPath);
            ResetEngine();
            setCropTop(originalCropTop);
            setCenterKey(originalCenterKey);
            setEqualKey(originalEqualKey);
            setIgnoredClasses(const_cast<LPTSTR>(originalIgnored.c_str()), static_cast<int>(originalIgnored.size()));
            setSkinnedClasses(const_cast<LPTSTR>(originalSkinned.c_str()), static_cast<int>(originalSkinned.size()));
        }

        SettingsFixture(const SettingsFixture &) = delete;
        SettingsFixture & operator=(const SettingsFixture &) = delete;

        LPCWSTR Path() const { return iniPath; }

        HWND Page(int resource, DLGPROC procedure)
        {
            HWND page = CreateDialogParamW(g_hInst, MAKEINTRESOURCEW(resource), parent, procedure, 0);
            Assert::IsNotNull(page, L"The production settings resource must create successfully.");
            pages.push_back(page);
            return page;
        }

        HWND OpenPreferences()
        {
            // Replace only native sheet creation: run the real opening path,
            // initialization handler, and resource without displaying a window.
            SnapSettings_TestSetSheetCreator(CreateHiddenPreferences);
            HWND page = DoPropertySheet(parent);
            SnapSettings_TestSetSheetCreator(nullptr);
            Assert::IsNotNull(page);
            pages.push_back(page);
            return page;
        }

        HWND OpenSheet()
        {
            SnapSettings_TestSetSheetCreator(CreateNativeSheet);
            HWND sheet = DoPropertySheet(parent);
            SnapSettings_TestSetSheetCreator(nullptr);
            Assert::IsTrue(IsWindow(sheet) != FALSE, L"The actual modeless property sheet must be created.");
            pages.push_back(sheet);
            return sheet;
        }

        static HWND SelectPage(HWND sheet, int index)
        {
            Assert::IsTrue(PropSheet_SetCurSel(sheet, nullptr, index) != FALSE);
            HWND page = PropSheet_GetCurrentPageHwnd(sheet);
            Assert::IsNotNull(page);
            return page;
        }

        static void Press(HWND sheet, int button)
        {
            PropSheet_PressButton(sheet, button);
            MSG message;
            for (int count = 0; count < 100 && PeekMessageW(&message, sheet, 0, 0, PM_REMOVE); ++count) {
                if (!PropSheet_IsDialogMessage(sheet, &message)) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
        }

        static void Click(HWND page, int control, bool checked = true)
        {
            Check(page, control, checked);
            SendMessageW(page, WM_COMMAND, MAKEWPARAM(control, BN_CLICKED),
                reinterpret_cast<LPARAM>(GetDlgItem(page, control)));
        }

        static void Notify(HWND page, UINT code)
        {
            NMHDR notification = {};
            notification.hwndFrom = GetParent(page);
            notification.code = code;
            SendMessageW(page, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&notification));
        }

        static INT_PTR NotifyProcedure(HWND page, DLGPROC procedure, UINT code)
        {
            NMHDR notification = {};
            notification.hwndFrom = GetParent(page);
            notification.code = code;
            // Property pages return TRUE to acknowledge handling and supply the
            // notification result separately through DWLP_MSGRESULT.
            return procedure(page, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&notification));
        }

        static void Check(HWND page, int control, bool checked = true)
        {
            CheckDlgButton(page, control, checked ? BST_CHECKED : BST_UNCHECKED);
        }

        static void SelectToggle(HWND page, UINT virtualKey)
        {
            HWND combo = GetDlgItem(page, IDC_TOGGLEKEYS);
            const int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
            for (int item = 0; item < count; ++item)
            {
                if (static_cast<UINT>(SendMessageW(combo, CB_GETITEMDATA, item, 0)) == virtualKey)
                {
                    SendMessageW(combo, CB_SETCURSEL, item, 0);
                    return;
                }
            }
            Assert::Fail(L"The requested toggle key was not present in the actual combo box.");
        }
    };
}
