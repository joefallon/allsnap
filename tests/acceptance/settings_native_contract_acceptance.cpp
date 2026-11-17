#include "stdafx.h"
#include <prsht.h>
#include "../support/SettingsFixture.h"
#include <cstring>

extern "C" {
int TestApp_TipCount(void);
HWND TestApp_TipControl(void);
BOOL OnContextMenu(HWND, HWND, UINT, UINT);
void SnapSettings_TestSetContextMenuOperations(
    HMENU (WINAPI *)(HINSTANCE, LPCTSTR),
    BOOL (WINAPI *)(HMENU, UINT, int, int, int, HWND, const RECT *));
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        PFNPROPSHEETCALLBACK capturedCallback;
        PROPSHEETHEADER capturedHeader;
        PROPSHEETPAGE capturedPages[3];

        INT_PTR CALLBACK CaptureSheetInputs(LPCPROPSHEETHEADER header)
        {
            capturedCallback = header->pfnCallback;
            capturedHeader = *header;
            for (UINT index = 0; index < 3 && index < header->nPages; ++index)
                capturedPages[index] = header->ppsp[index];
            return 0;
        }

        void CaptureProductionSheet()
        {
            capturedCallback = nullptr;
            SnapSettings_TestSetSheetCreator(CaptureSheetInputs);
            DoPropertySheet(nullptr);
            SnapSettings_TestSetSheetCreator(nullptr);
            Assert::IsTrue(capturedCallback != nullptr);
        }

        INT_PTR CALLBACK EmptyDialog(HWND, UINT message, WPARAM, LPARAM)
        {
            return message == WM_INITDIALOG;
        }

        void AssertHelpIsIsolated(int resource, DLGPROC procedure, int controlId)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(resource, procedure);
            HWND control = GetDlgItem(page, controlId);
            Assert::IsNotNull(control);
            const int threshold = getWinThresh();
            const int crop = getCropTop();
            gridsnap_settings_t originalGrid;
            getGridSnap(&originalGrid);
            if (resource == IDD_GRID) {
                SettingsFixture::Click(page, IDC_CHECK_HORIZONTAL);
                CheckRadioButton(page, IDC_RADIO_H_EVEN, IDC_RADIO_H_SINGLE, IDC_RADIO_H_EVEN);
            }
            if (resource == IDD_ADVANCED)
                SettingsFixture::Click(page, IDC_CROPTOPC);
            SetDlgItemInt(page, controlId, 45, FALSE);
            HELPINFO help = {};
            help.cbSize = sizeof(help);
            help.iContextType = HELPINFO_WINDOW;
            help.iCtrlId = controlId;
            help.hItemHandle = control;
            SetWindowLongPtrW(page, DWLP_MSGRESULT, 12345);

            const INT_PTR handled = procedure(page, WM_HELP, 0,
                reinterpret_cast<LPARAM>(&help));

            Assert::AreEqual(1, TestApp_TipCount());
            Assert::IsTrue(control == TestApp_TipControl());
            Assert::AreEqual(threshold, getWinThresh(), L"Help must not apply edited settings.");
            Assert::AreEqual(crop, getCropTop(), L"Help must not apply the edited crop amount.");
            gridsnap_settings_t actualGrid;
            getGridSnap(&actualGrid);
            Assert::AreEqual<BOOL>(originalGrid.h.enabled, actualGrid.h.enabled);
            Assert::AreEqual(static_cast<int>(originalGrid.h.type), static_cast<int>(actualGrid.h.type));
            Assert::AreEqual(originalGrid.h.val, actualGrid.h.val);
            Assert::AreEqual<BOOL>(originalGrid.v.enabled, actualGrid.v.enabled);
            Assert::AreEqual(static_cast<int>(originalGrid.v.type), static_cast<int>(actualGrid.v.type));
            Assert::AreEqual(originalGrid.v.val, actualGrid.v.val);
            Assert::AreEqual<UINT>(999, GetPrivateProfileIntW(
                L"settings", L"win_tresh", 999, fixture.Path()), L"Help must not save preferences.");
            Assert::AreEqual<LONG_PTR>(12345, GetWindowLongPtrW(page, DWLP_MSGRESULT));
            Assert::AreEqual<INT_PTR>(TRUE, handled, L"A processed help request must be acknowledged.");
        }

        class MenuProbe
        {
            static MenuProbe * active;
            std::vector<HMENU> menus;
            static HMENU WINAPI Load(HINSTANCE instance, LPCTSTR resource)
            {
                ++active->loads;
                if (active->failLoad) return nullptr;
                HMENU menu = active->emptyRoot ? CreateMenu() : LoadMenu(instance, resource);
                active->root = menu;
                if (menu) active->menus.push_back(menu);
                return menu;
            }
            static BOOL WINAPI Track(HMENU menu, UINT flags, int x, int y,
                int reserved, HWND owner, const RECT * exclusion)
            {
                ++active->tracks;
                active->submenu = menu;
                active->flags = flags;
                active->owner = owner;
                active->x = x;
                active->y = y;
                active->argumentsValid = reserved == 0 && exclusion == nullptr;
                return active->selected ? IDM_WHAT : 0;
            }
        public:
            int loads = 0, tracks = 0, x = 0, y = 0;
            UINT flags = 0;
            HWND owner = nullptr;
            HMENU root = nullptr, submenu = nullptr;
            bool failLoad = false, emptyRoot = false, selected = false, argumentsValid = false;
            MenuProbe()
            {
                active = this;
                SnapSettings_TestSetContextMenuOperations(Load, Track);
            }
            ~MenuProbe()
            {
                SnapSettings_TestSetContextMenuOperations(nullptr, nullptr);
                for (HMENU menu : menus)
                    if (IsMenu(menu)) DestroyMenu(menu);
                active = nullptr;
            }
        };
        MenuProbe * MenuProbe::active;

        void AssertMenuOwnership(bool selected)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            HWND control = GetDlgItem(page, IDC_WIN_THRESH);
            Assert::IsTrue(SetWindowContextHelpId(control, 1001) != FALSE);
            MenuProbe probe;
            probe.selected = selected;
            const BOOL handled = OnContextMenu(page, control, 17, 29);
            Assert::IsTrue(handled != FALSE);
            Assert::AreEqual(1, probe.loads);
            Assert::AreEqual(1, probe.tracks);
            Assert::IsTrue(probe.owner == page && probe.argumentsValid);
            Assert::AreEqual(17, probe.x);
            Assert::AreEqual(29, probe.y);
            Assert::AreEqual<UINT>(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, probe.flags);
            Assert::AreEqual(selected ? 1 : 0, TestApp_TipCount());
            if (selected) Assert::IsTrue(TestApp_TipControl() == control);
            Assert::IsFalse(IsMenu(probe.root) != FALSE, L"Release the owning root menu.");
            Assert::IsFalse(IsMenu(probe.submenu) != FALSE, L"Destroying the root releases its submenu.");
        }

#pragma pack(push, 1)
        struct ExtendedDialogTemplate
        {
            WORD version, signature;
            DWORD helpId, extendedStyle, style;
            WORD items;
            short x, y, width, height;
            WORD menu, windowClass, title;
        };
#pragma pack(pop)

        struct StandardDialogTemplate
        {
            DLGTEMPLATE dialog;
            WORD menu, windowClass, title;
        };
    }

    TEST_CLASS(SettingsNativeContractAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_CLEANUP(Cleanup)
        {
            SnapSettings_TestSetSheetCreator(nullptr);
            SnapSettings_TestSetContextMenuOperations(nullptr, nullptr);
        }

        // ST-024: actual child controls retain settings while help is acknowledged.
        TEST_METHOD(PreferencesHelpIsHandledWithoutApplyOrSave)
        { AssertHelpIsIsolated(IDD_GENERAL, PrefsProc, IDC_WIN_THRESH); }
        TEST_METHOD(GridHelpIsHandledWithoutApplyOrSave)
        { AssertHelpIsIsolated(IDD_GRID, GridProc, IDC_H_NUM_EVEN); }
        TEST_METHOD(AdvancedHelpIsHandledWithoutApplyOrSave)
        { AssertHelpIsIsolated(IDD_ADVANCED, AdvProc, IDC_CROPTOP); }

        // ST-025: popup selection/dismissal releases real native menu resources.
        TEST_METHOD(SelectedContextHelpReleasesRootAndSubmenu) { AssertMenuOwnership(true); }
        TEST_METHOD(DismissedContextHelpReleasesRootAndSubmenu) { AssertMenuOwnership(false); }

        TEST_METHOD(FailedContextMenuLoadDoesNotTrackOrShowHelp)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            HWND control = GetDlgItem(page, IDC_WIN_THRESH);
            SetWindowContextHelpId(control, 1001);
            MenuProbe probe;
            probe.failLoad = true;
            Assert::IsFalse(OnContextMenu(page, control, 17, 29) != FALSE);
            Assert::AreEqual(0, probe.tracks);
            Assert::AreEqual(0, TestApp_TipCount());
        }

        TEST_METHOD(MissingContextSubmenuReleasesLoadedRoot)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            HWND control = GetDlgItem(page, IDC_WIN_THRESH);
            SetWindowContextHelpId(control, 1001);
            MenuProbe probe;
            probe.emptyRoot = true;
            const BOOL handled = OnContextMenu(page, control, 17, 29);
            Assert::IsFalse(IsMenu(probe.root) != FALSE);
            Assert::IsFalse(handled != FALSE);
            Assert::AreEqual(0, probe.tracks);
            Assert::AreEqual(0, TestApp_TipCount());
        }

        TEST_METHOD(ControlsWithoutContextHelpDoNotAllocateMenus)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            HWND control = GetDlgItem(page, IDC_WIN_THRESH);
            SetWindowContextHelpId(control, 0);
            MenuProbe probe;
            Assert::IsFalse(OnContextMenu(page, nullptr, 17, 29) != FALSE);
            Assert::IsFalse(OnContextMenu(page, control, 17, 29) != FALSE);
            Assert::AreEqual(0, probe.loads);
            Assert::AreEqual(0, probe.tracks);
        }

        // ST-026: PRECREATE centers both native formats without corrupting headers.
        TEST_METHOD(StandardPropertySheetTemplateKeepsItsNativeHeader)
        {
            SettingsFixture fixture;
            CaptureProductionSheet();
            alignas(DWORD) StandardDialogTemplate dialog = {};
            dialog.dialog.style = WS_POPUP | WS_CAPTION | DS_MODALFRAME;
            dialog.dialog.cx = 120;
            dialog.dialog.cy = 90;
            StandardDialogTemplate expected = dialog;
            expected.dialog.style |= DS_CENTER;
            const int result = capturedCallback(nullptr, PSCB_PRECREATE,
                reinterpret_cast<LPARAM>(&dialog));
            Assert::IsTrue(std::memcmp(&expected, &dialog, sizeof(dialog)) == 0);
            HWND window = CreateDialogIndirectParamW(g_hInst, &dialog.dialog, nullptr, EmptyDialog, 0);
            const bool created = window != nullptr;
            if (window) DestroyWindow(window);
            Assert::IsTrue(created, L"The centered standard template must still create a native dialog.");
            Assert::AreEqual(0, result, L"The callback must obey its int return contract.");
        }

        TEST_METHOD(ExtendedPropertySheetTemplateKeepsItsNativeHeader)
        {
            SettingsFixture fixture;
            CaptureProductionSheet();
            alignas(DWORD) ExtendedDialogTemplate dialog = {};
            dialog.version = 1;
            dialog.signature = 0xffff;
            dialog.helpId = 123;
            dialog.extendedStyle = WS_EX_TOOLWINDOW;
            dialog.style = WS_POPUP | WS_CAPTION | DS_MODALFRAME;
            dialog.width = 120;
            dialog.height = 90;
            ExtendedDialogTemplate expected = dialog;
            expected.style |= DS_CENTER;
            capturedCallback(nullptr, PSCB_PRECREATE, reinterpret_cast<LPARAM>(&dialog));
            Assert::IsTrue(std::memcmp(&expected, &dialog, sizeof(dialog)) == 0,
                L"Centering an extended template must preserve its version, signature, and help ID.");
            HWND window = CreateDialogIndirectParamW(g_hInst,
                reinterpret_cast<LPCDLGTEMPLATE>(&dialog), nullptr, EmptyDialog, 0);
            const bool created = window != nullptr;
            if (window) DestroyWindow(window);
            Assert::IsTrue(created, L"The centered extended template must still create a native dialog.");
        }

        // ST-027: inactive optional inputs are explicitly empty at the API boundary.
        TEST_METHOD(PropertySheetInputsLeaveUnusedOptionalFieldsEmpty)
        {
            SettingsFixture fixture;
            CaptureProductionSheet();
            Assert::AreEqual<UINT>(3, capturedHeader.nPages);
            Assert::IsNull(capturedHeader.hbmWatermark);
            Assert::IsNull(capturedHeader.hplWatermark);
            Assert::IsNull(capturedHeader.hbmHeader);
            for (const auto & page : capturedPages) {
                Assert::IsNull(page.hIcon);
                Assert::IsNull(page.pcRefParent);
                Assert::IsNull(page.pszHeaderTitle);
                Assert::IsNull(page.pszHeaderSubTitle);
                Assert::IsNull(page.hActCtx);
            }
        }
    };
}
