#include "../support/SettingsFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        void AssertApplyFailureAndRetry(int resource, DLGPROC procedure)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(resource, procedure);
            WCHAR directory[MAX_PATH] = {};
            Assert::IsTrue(GetTempPathW(MAX_PATH, directory) != 0);
            SnapIni_TestSetPath(directory);
            INT_PTR handledFailure = SettingsFixture::NotifyProcedure(page, procedure, PSN_APPLY);
            LONG_PTR failureResult = GetWindowLongPtrW(page, DWLP_MSGRESULT);
            int failureMessages = TestApp_MessageCount();

            SnapIni_TestSetPath(fixture.Path());
            INT_PTR handledRetry = SettingsFixture::NotifyProcedure(page, procedure, PSN_APPLY);
            LONG_PTR retryResult = GetWindowLongPtrW(page, DWLP_MSGRESULT);

            Assert::AreEqual(1, failureMessages);
            Assert::AreEqual<LONG_PTR>(PSNRET_INVALID_NOCHANGEPAGE, failureResult);
            Assert::AreEqual<INT_PTR>(TRUE, handledFailure,
                L"A failed save must be handled so the property sheet observes the rejection.");
            Assert::AreEqual<INT_PTR>(TRUE, handledRetry);
            Assert::AreEqual<LONG_PTR>(PSNRET_NOERROR, retryResult,
                L"A successful retry must clear the earlier Apply failure.");
            Assert::AreEqual(1, TestApp_MessageCount());
        }

        void AssertOpeningPreservesEnabled(BOOL stored, BOOL current)
        {
            SettingsFixture fixture;
            setEnabled(stored);
            Assert::IsTrue(SaveSettingsToINI());
            setEnabled(current);
            HWND page = fixture.OpenPreferences();
            const BOOL enabledAfterOpening = isEnabled();
            const UINT checkbox = IsDlgButtonChecked(page, IDC_ENABLED);
            SettingsFixture::Check(page, IDC_ENABLED, current == FALSE);
            SettingsFixture::Notify(page, PSN_RESET);

            Assert::AreEqual<BOOL>(current, enabledAfterOpening,
                L"Opening Settings must preserve the current tray-enabled state.");
            Assert::AreEqual<UINT>(current ? BST_CHECKED : BST_UNCHECKED, checkbox);
            Assert::AreEqual<BOOL>(current, isEnabled());
            Assert::AreEqual<UINT>(stored, GetPrivateProfileIntW(
                L"settings", L"enabled", 999, fixture.Path()));
        }
    }

    TEST_CLASS(SettingsAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // ST-001: actual production dialog controls initialize from engine settings.
        TEST_METHOD(PreferencesInitializeFromEngineValues)
        {
            SettingsFixture fixture;
            setWinThresh(23);
            setScreenThresh(31);
            setWindowSpacing(8);
            setSnapType(SNAPT_DESKTOP | SNAPT_HCENTER);
            setToggleKey(VK_TAB);
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            Assert::AreEqual<UINT>(23, GetDlgItemInt(page, IDC_WIN_THRESH, nullptr, FALSE));
            Assert::AreEqual<UINT>(31, GetDlgItemInt(page, IDC_SCREEN_THRESH, nullptr, FALSE));
            Assert::AreEqual<UINT>(8, GetDlgItemInt(page, IDC_WINDOW_SPACING, nullptr, FALSE));
            Assert::AreEqual<UINT>(BST_CHECKED, IsDlgButtonChecked(page, IDC_DESKTOP));
            Assert::AreEqual<UINT>(BST_CHECKED, IsDlgButtonChecked(page, IDC_HCENTER));
            Assert::AreEqual<UINT>(BST_UNCHECKED, IsDlgButtonChecked(page, IDC_OTHERS));
            HWND combo = GetDlgItem(page, IDC_TOGGLEKEYS);
            LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
            Assert::AreEqual<UINT>(VK_TAB, static_cast<UINT>(SendMessageW(combo, CB_GETITEMDATA, selected, 0)));
        }

        // ST-002: apply traverses the real settings controller and persistence mapping.
        TEST_METHOD(PreferencesApplyPersistsAndReloadsEditedValues)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            SetDlgItemInt(page, IDC_WIN_THRESH, 23, FALSE);
            SetDlgItemInt(page, IDC_SCREEN_THRESH, 31, FALSE);
            SetDlgItemInt(page, IDC_WINDOW_SPACING, 8, FALSE);
            SettingsFixture::Check(page, IDC_DESKTOP);
            SettingsFixture::Check(page, IDC_OTHERS);
            SettingsFixture::Check(page, IDC_SELF);
            SettingsFixture::Check(page, IDC_VCENTER);
            SettingsFixture::Check(page, IDC_HCENTER, false);
            SettingsFixture::Check(page, IDC_ENABLED, false);
            SettingsFixture::SelectToggle(page, VK_TAB);
            SettingsFixture::Notify(page, PSN_APPLY);
            Assert::IsFalse(isEnabled());
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(31, getScreenThresh());
            Assert::AreEqual(8, getWindowSpacing());
            Assert::AreEqual<UINT>(VK_TAB, getToggleKey());
            ResetEngine();
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(31, getScreenThresh());
            Assert::AreEqual(8, getWindowSpacing());
            Assert::IsFalse(isEnabled());
            Assert::AreEqual<UINT>(VK_TAB, getToggleKey());
            Assert::AreEqual<UINT>(SNAPT_DESKTOP | SNAPT_OTHERS | SNAPT_SELF | SNAPT_VCENTER, getSnapType());
        }

        // ST-003: Cancel must not mutate engine settings or write the temporary INI.
        TEST_METHOD(PreferencesCancelDoesNotApplyPendingEdits)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            SetDlgItemInt(page, IDC_WIN_THRESH, 45, FALSE);
            SettingsFixture::Check(page, IDC_DESKTOP);
            SettingsFixture::Notify(page, PSN_RESET);
            Assert::AreEqual(10, getWinThresh());
            Assert::AreEqual<UINT>(SNAPT_NONE, getSnapType());
            Assert::AreEqual<UINT>(999, GetPrivateProfileIntW(L"settings", L"win_tresh", 999, fixture.Path()));
        }

        // ST-004: Advanced checkbox labels include inverse-valued settings.
        TEST_METHOD(AdvancedApplyPersistsConfinementAndCheckboxInversions)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_ADVANCED, AdvProc);
            SettingsFixture::Check(page, IDC_SNAPMDI);
            SettingsFixture::Check(page, IDC_INVTOGGLE);
            SettingsFixture::Check(page, IDC_INSIDES);
            SettingsFixture::Check(page, IDC_HIDEICON);
            SettingsFixture::Check(page, IDC_KEPT);
            SettingsFixture::Check(page, IDC_CROP_RGN);
            SettingsFixture::Check(page, IDC_CROPTOPC);
            SetDlgItemInt(page, IDC_CROPTOP, 7, FALSE);
            SettingsFixture::Notify(page, PSN_APPLY);
            Assert::IsTrue(isKeptToScreen());
            Assert::IsTrue(isSnapMdi());
            Assert::IsFalse(isDisableToggle());
            Assert::IsFalse(isSnappingInsides());
            Assert::IsFalse(isCroppingRgn());
            Assert::IsTrue(isIconHidden());
            Assert::IsTrue(isCroppingTop());
            Assert::AreEqual(7, getCropTop());
            ResetEngine();
            setIconHidden(FALSE);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::IsTrue(isKeptToScreen() && isSnapMdi() && isIconHidden() && isCroppingTop());
            Assert::IsFalse(isDisableToggle() || isSnappingInsides() || isCroppingRgn());
            Assert::AreEqual(7, getCropTop());
        }

        // ST-005: real grid controls retain integer values through Apply/save/reload.
        TEST_METHOD(GridApplyPersistsModesAndIntegerSpacing)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GRID, GridProc);
            SettingsFixture::Check(page, IDC_CHECK_HORIZONTAL);
            SettingsFixture::Check(page, IDC_CHECK_VERTICAL);
            SettingsFixture::Check(page, IDC_RADIO_H_EVEN, false);
            SettingsFixture::Check(page, IDC_RADIO_H_PIXELS);
            SettingsFixture::Check(page, IDC_RADIO_H_SINGLE, false);
            SettingsFixture::Check(page, IDC_RADIO_V_EVEN, false);
            SettingsFixture::Check(page, IDC_RADIO_V_PIXELS, false);
            SettingsFixture::Check(page, IDC_RADIO_V_SINGLE);
            SetDlgItemInt(page, IDC_H_PIXELS_BTWN, 37, FALSE);
            SetDlgItemInt(page, IDC_V_PIXELS_FROM_LEFT, 240, FALSE);
            SettingsFixture::Notify(page, PSN_APPLY);
            gridsnap_settings_t grid = {};
            setGridSnap(&grid);
            Assert::IsTrue(LoadSettingsFromINI());
            getGridSnap(&grid);
            Assert::IsTrue(grid.h.enabled && grid.v.enabled);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_PIXELS), static_cast<int>(grid.h.type));
            Assert::AreEqual(37, grid.h.val);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_SINGLE), static_cast<int>(grid.v.type));
            Assert::AreEqual(240, grid.v.val);
        }

        TEST_METHOD(AdvancedCancelDoesNotEnableConfinement)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_ADVANCED, AdvProc);
            SettingsFixture::Check(page, IDC_KEPT);
            SettingsFixture::Notify(page, PSN_RESET);
            Assert::IsFalse(isKeptToScreen());
            Assert::AreEqual<UINT>(999, GetPrivateProfileIntW(L"settings", L"kept_to_screen", 999, fixture.Path()));
        }

        // ST-006: validation uses the real page path without showing test dialogs.
        TEST_METHOD(PreferencesRejectInvalidSpacingBeforeLeavingPage)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            SetDlgItemInt(page, IDC_WINDOW_SPACING, 1001, FALSE);
            SettingsFixture::Notify(page, PSN_KILLACTIVE);
            Assert::AreEqual(1, TestApp_MessageCount());
            Assert::AreEqual<LONG_PTR>(TRUE, GetWindowLongPtrW(page, DWLP_MSGRESULT),
                L"Validation must keep the page active after focusing the invalid control.");
            Assert::AreEqual(0, getWindowSpacing());
        }

        TEST_METHOD(PreferencesRejectInvalidThresholdBeforeLeavingPage)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            SetDlgItemInt(page, IDC_WIN_THRESH, 1, FALSE);
            SettingsFixture::Notify(page, PSN_KILLACTIVE);
            Assert::AreEqual(1, TestApp_MessageCount());
            Assert::AreEqual<LONG_PTR>(TRUE, GetWindowLongPtrW(page, DWLP_MSGRESULT),
                L"Validation must preserve rejection after focusing an invalid threshold.");
            Assert::AreEqual(10, getWinThresh());
        }

        // ST-007: controller handles an actual persistence failure visibly.
        TEST_METHOD(PreferencesApplyReportsSaveFailureAndKeepsPageOpen)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            WCHAR directory[MAX_PATH] = {};
            Assert::IsTrue(GetTempPathW(MAX_PATH, directory) != 0);
            SnapIni_TestSetPath(directory);
            SettingsFixture::Notify(page, PSN_APPLY);
            Assert::AreEqual(1, TestApp_MessageCount());
            Assert::AreEqual<LONG_PTR>(PSNRET_INVALID_NOCHANGEPAGE, GetWindowLongPtrW(page, DWLP_MSGRESULT));
        }

        // ST-008: acknowledgement and DWLP_MSGRESULT are separate parts of PSN_APPLY.
        TEST_METHOD(PreferencesApplyHandlesFailureAndSuccessfulRetry)
        {
            AssertApplyFailureAndRetry(IDD_GENERAL, PrefsProc);
        }
        TEST_METHOD(GridApplyHandlesFailureAndSuccessfulRetry)
        {
            AssertApplyFailureAndRetry(IDD_GRID, GridProc);
        }
        TEST_METHOD(AdvancedApplyHandlesFailureAndSuccessfulRetry)
        {
            AssertApplyFailureAndRetry(IDD_ADVANCED, AdvProc);
        }

        // ST-009: opening Settings is not a command to undo the tray toggle.
        TEST_METHOD(OpeningSettingsPreservesRuntimeDisabledState)
        {
            AssertOpeningPreservesEnabled(TRUE, FALSE);
        }
        TEST_METHOD(OpeningSettingsPreservesRuntimeEnabledState)
        {
            AssertOpeningPreservesEnabled(FALSE, TRUE);
        }
    };
}
