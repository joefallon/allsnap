#include "../support/SettingsFixture.h"
#include <limits.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        // Standard Apply button identifier in the native property-sheet dialog.
        const int ApplyButtonId = 0x3021;
        void SetGridAxis(HWND page, bool horizontal, int mode, LPCWSTR value)
        {
            const int first = horizontal ? IDC_RADIO_H_EVEN : IDC_RADIO_V_EVEN;
            const int edits[2][3] = {{IDC_H_NUM_EVEN, IDC_H_PIXELS_BTWN, IDC_H_PIXELS_FROM_TOP},
                {IDC_V_NUM_EVEN, IDC_V_PIXELS_BTWN, IDC_V_PIXELS_FROM_LEFT}};
            SettingsFixture::Click(page, horizontal ? IDC_CHECK_HORIZONTAL : IDC_CHECK_VERTICAL);
            CheckRadioButton(page, first, first + 2, first + mode);
            SendMessageW(page, WM_COMMAND, MAKEWPARAM(first + mode, BN_CLICKED),
                reinterpret_cast<LPARAM>(GetDlgItem(page, first + mode)));
            const int edit = edits[horizontal ? 0 : 1][mode];
            Assert::IsTrue(SetDlgItemTextW(page, edit, value) != FALSE);
            WCHAR actual[32] = {};
            GetDlgItemTextW(page, edit, actual, 32);
            Assert::AreEqual(value, actual, L"The intended numeric text must reach the real edit control.");
        }

        void AssertInvalidGridText(bool horizontal, int mode, LPCWSTR value)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 1);
            SetGridAxis(page, horizontal, mode, value);
            Assert::IsFalse(PropSheet_SetCurSel(sheet, nullptr, 2) != FALSE,
                L"Invalid selected grid input must keep its native page active.");
            Assert::IsTrue(page == PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(1, TestApp_MessageCount());
        }

        void AssertInvalidCropText(LPCWSTR value)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 2);
            SettingsFixture::Click(page, IDC_CROPTOPC);
            Assert::IsTrue(SetDlgItemTextW(page, IDC_CROPTOP, value) != FALSE);
            WCHAR actual[32] = {};
            GetDlgItemTextW(page, IDC_CROPTOP, actual, 32);
            Assert::AreEqual(value, actual);
            Assert::IsFalse(PropSheet_SetCurSel(sheet, nullptr, 0) != FALSE,
                L"Invalid enabled top-crop input must keep Advanced active.");
            Assert::IsTrue(page == PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(1, TestApp_MessageCount());
        }
    }

    TEST_CLASS(PropertySheetAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // ST-010: actual modeless sheet dirty state, Apply, and Cancel sequence.
        TEST_METHOD(NativeApplyCommitsAndCancelDiscardsLaterEdits)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = PropSheet_GetCurrentPageHwnd(sheet);
            Assert::IsFalse(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
            SetDlgItemInt(page, IDC_WIN_THRESH, 23, FALSE);
            SetDlgItemInt(page, IDC_WINDOW_SPACING, 8, FALSE);
            Assert::IsTrue(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(8, getWindowSpacing());
            Assert::IsFalse(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
            SetDlgItemInt(page, IDC_WIN_THRESH, 45, FALSE);
            SettingsFixture::Press(sheet, PSBTN_CANCEL);
            Assert::IsNull(PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual<UINT>(23, GetPrivateProfileIntW(L"settings", L"win_tresh", 999, fixture.Path()));
        }

        // ST-011: OK applies all edited pages through native notifications.
        TEST_METHOD(NativeOkAppliesPreferencesGridAndAdvanced)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND preferences = PropSheet_GetCurrentPageHwnd(sheet);
            SetDlgItemInt(preferences, IDC_SCREEN_THRESH, 31, FALSE);
            SettingsFixture::Click(preferences, IDC_DESKTOP);
            HWND grid = SettingsFixture::SelectPage(sheet, 1);
            SetGridAxis(grid, true, GRIDTYPE_PIXELS, L"37");
            SetGridAxis(grid, false, GRIDTYPE_SINGLE, L"240");
            HWND advanced = SettingsFixture::SelectPage(sheet, 2);
            SettingsFixture::Click(advanced, IDC_KEPT);
            SettingsFixture::Click(advanced, IDC_CROPTOPC);
            SetDlgItemInt(advanced, IDC_CROPTOP, 7, FALSE);
            SettingsFixture::Press(sheet, PSBTN_OK);
            Assert::IsNull(PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(31, getScreenThresh());
            Assert::AreEqual<UINT>(SNAPT_DESKTOP, getSnapType());
            Assert::IsTrue(isKeptToScreen() && isCroppingTop());
            Assert::AreEqual(7, getCropTop());
            gridsnap_settings_t saved = {};
            getGridSnap(&saved);
            Assert::IsTrue(saved.h.enabled && saved.v.enabled);
            Assert::AreEqual(37, saved.h.val);
            Assert::AreEqual(240, saved.v.val);
            ResetEngine();
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(31, getScreenThresh());
            Assert::IsTrue(isKeptToScreen() && isCroppingTop());
            getGridSnap(&saved);
            Assert::AreEqual(37, saved.h.val);
            Assert::AreEqual(240, saved.v.val);
        }

        // ST-012: native Cancel discards edits on every settings page.
        TEST_METHOD(NativeCancelDoesNotApplyVisitedPages)
        {
            SettingsFixture fixture;
            Assert::IsTrue(SaveSettingsToINI());
            HWND sheet = fixture.OpenSheet();
            SetDlgItemInt(PropSheet_GetCurrentPageHwnd(sheet), IDC_WIN_THRESH, 45, FALSE);
            SetGridAxis(SettingsFixture::SelectPage(sheet, 1), true, GRIDTYPE_PIXELS, L"37");
            SettingsFixture::Click(SettingsFixture::SelectPage(sheet, 2), IDC_KEPT);
            SettingsFixture::Press(sheet, PSBTN_CANCEL);
            Assert::IsNull(PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(10, getWinThresh());
            Assert::IsFalse(isKeptToScreen());
            gridsnap_settings_t saved = {};
            getGridSnap(&saved);
            Assert::IsFalse(saved.h.enabled != FALSE);
            Assert::AreEqual<UINT>(10, GetPrivateProfileIntW(L"settings", L"win_tresh", 999, fixture.Path()));
        }

        // ST-013: native tab switches and OK obey Preferences validation.
        TEST_METHOD(NativeInvalidPreferencesBlockTabAndOk)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = PropSheet_GetCurrentPageHwnd(sheet);
            SetDlgItemInt(page, IDC_WIN_THRESH, 1, FALSE);
            Assert::IsFalse(PropSheet_SetCurSel(sheet, nullptr, 1) != FALSE);
            Assert::IsTrue(page == PropSheet_GetCurrentPageHwnd(sheet));
            SettingsFixture::Press(sheet, PSBTN_OK);
            Assert::IsTrue(page == PropSheet_GetCurrentPageHwnd(sheet));
            Assert::AreEqual(10, getWinThresh());
            SetDlgItemInt(page, IDC_WIN_THRESH, 2, FALSE);
            SetDlgItemInt(page, IDC_SCREEN_THRESH, 100, FALSE);
            Assert::IsTrue(PropSheet_SetCurSel(sheet, nullptr, 1) != FALSE);
        }

        TEST_METHOD(ThresholdSpinnersIncludeValidatedUpperEndpoint)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            for (int control : {IDC_WIN_THRESH_SPIN, IDC_SCREEN_THRESH_SPIN}) {
                int minimum = 0, maximum = 0;
                SendDlgItemMessageW(page, control, UDM_GETRANGE32,
                    reinterpret_cast<WPARAM>(&minimum), reinterpret_cast<LPARAM>(&maximum));
                Assert::AreEqual(2, minimum);
                Assert::AreEqual(100, maximum);
            }
        }

        // ST-014: both axes validate only their selected, enabled grid value.
        TEST_METHOD(GridRejectsMalformedSelectedNumbers)
        {
            for (bool horizontal : {true, false})
                for (LPCWSTR value : {L"", L"abc", L"12oops", L"1.5", L"-1", L"2147483648"})
                    AssertInvalidGridText(horizontal, GRIDTYPE_PIXELS, value);
        }

        TEST_METHOD(GridRejectsTooManyEvenLinesAndZeroPixelSpacing)
        {
            for (bool horizontal : {true, false}) {
                AssertInvalidGridText(horizontal, GRIDTYPE_EVEN, L"101");
                AssertInvalidGridText(horizontal, GRIDTYPE_PIXELS, L"0");
                AssertInvalidGridText(horizontal, GRIDTYPE_SINGLE, L"-1");
            }
        }

        TEST_METHOD(NativeApplyRejectsInvalidCurrentGridWithoutMutation)
        {
            SettingsFixture fixture;
            gridsnap_settings_t original = {{TRUE, GRIDTYPE_PIXELS, 37}, {FALSE, GRIDTYPE_SINGLE, 240}};
            setGridSnap(&original);
            Assert::IsTrue(SaveSettingsToINI());
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 1);
            SetGridAxis(page, true, GRIDTYPE_EVEN, L"101");
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::AreEqual(1, TestApp_MessageCount());
            Assert::IsTrue(actual.h.enabled != FALSE);
            Assert::AreEqual(37, actual.h.val);
            Assert::AreEqual<UINT>(37, GetPrivateProfileIntW(L"settings", L"hgridval", 999, fixture.Path()));
        }

        // ST-015: disabling an axis retains its configuration for re-enabling.
        TEST_METHOD(NativeGridDisablePreservesModeAndValue)
        {
            SettingsFixture fixture;
            gridsnap_settings_t original = {{TRUE, GRIDTYPE_PIXELS, 37}, {TRUE, GRIDTYPE_SINGLE, 240}};
            setGridSnap(&original);
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 1);
            SettingsFixture::Click(page, IDC_CHECK_HORIZONTAL, false);
            SettingsFixture::Click(page, IDC_CHECK_VERTICAL, false);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::IsFalse(actual.h.enabled || actual.v.enabled);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_PIXELS), static_cast<int>(actual.h.type));
            Assert::AreEqual(37, actual.h.val);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_SINGLE), static_cast<int>(actual.v.type));
            Assert::AreEqual(240, actual.v.val);
        }

        // ST-016: enabled top crop accepts whole nonnegative signed integers.
        TEST_METHOD(AdvancedRejectsMalformedNegativeAndOverflowCrop)
        {
            for (LPCWSTR value : {L"", L"abc", L"12oops", L"1.5", L"-1", L"2147483648"})
                AssertInvalidCropText(value);
        }

        TEST_METHOD(NativeApplyRejectsInvalidCropWithoutMutation)
        {
            SettingsFixture fixture;
            setCropTop(7);
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 2);
            SettingsFixture::Click(page, IDC_CROPTOPC);
            SetDlgItemTextW(page, IDC_CROPTOP, L"abc");
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::AreEqual(1, TestApp_MessageCount());
            Assert::AreEqual(7, getCropTop());
            Assert::IsFalse(isCroppingTop());
        }

        // ST-018: native save failures leave Apply/OK available for a real retry.
        TEST_METHOD(NativeApplyAndOkRetryAfterSaveFailure)
        {
            for (int button : {PSBTN_APPLYNOW, PSBTN_OK}) {
                SettingsFixture fixture;
                HWND sheet = fixture.OpenSheet();
                HWND page = PropSheet_GetCurrentPageHwnd(sheet);
                SetDlgItemInt(page, IDC_WIN_THRESH, 37, FALSE);
                WCHAR directory[MAX_PATH] = {};
                Assert::IsTrue(GetTempPathW(MAX_PATH, directory) != 0);
                SnapIni_TestSetPath(directory);
                SettingsFixture::Press(sheet, button);
                Assert::IsTrue(page == PropSheet_GetCurrentPageHwnd(sheet));
                Assert::AreEqual(1, TestApp_MessageCount());
                SnapIni_TestSetPath(fixture.Path());
                SettingsFixture::Press(sheet, button);
                Assert::AreEqual<UINT>(37, GetPrivateProfileIntW(L"settings", L"win_tresh", 999, fixture.Path()));
                if (button == PSBTN_OK)
                    Assert::IsNull(PropSheet_GetCurrentPageHwnd(sheet));
                else
                    Assert::IsFalse(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
            }
        }

        // ST-019: all Preferences checkbox and toggle-key controls signal changes.
        TEST_METHOD(AllPreferencesDestinationsAndToggleKeysApplyThroughNativeSheet)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = PropSheet_GetCurrentPageHwnd(sheet);
            const int controls[] = {IDC_DESKTOP, IDC_OTHERS, IDC_SELF, IDC_VCENTER, IDC_HCENTER};
            const UINT values[] = {SNAPT_DESKTOP, SNAPT_OTHERS, SNAPT_SELF, SNAPT_VCENTER, SNAPT_HCENTER};
            UINT expected = SNAPT_NONE;
            for (int index = 0; index < 5; ++index) {
                SettingsFixture::Click(page, controls[index]);
                Assert::IsTrue(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
                SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                expected |= values[index];
                Assert::AreEqual(expected, getSnapType());
                SettingsFixture::Click(page, controls[index], false);
                SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                expected &= ~values[index];
                Assert::AreEqual(expected, getSnapType());
            }
            SettingsFixture::Click(page, IDC_ENABLED, false);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::IsFalse(isEnabled());
            SettingsFixture::Click(page, IDC_ENABLED);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::IsTrue(isEnabled());
            for (UINT key : {VK_MENU, VK_CONTROL, VK_SHIFT, VK_TAB}) {
                SettingsFixture::SelectToggle(page, key);
                SendMessageW(page, WM_COMMAND, MAKEWPARAM(IDC_TOGGLEKEYS, CBN_SELCHANGE),
                    reinterpret_cast<LPARAM>(GetDlgItem(page, IDC_TOGGLEKEYS)));
                Assert::IsTrue(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
                SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                Assert::AreEqual(key, getToggleKey());
            }
        }

        // ST-020: every live Advanced checkbox applies both direct and inverted values.
        TEST_METHOD(AllAdvancedCheckboxesApplyAndRestoreThroughNativeSheet)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 2);
            struct Checkbox { int control; BOOL (WINAPI *get)(); bool inverted; };
            const Checkbox boxes[] = {
                {IDC_SNAPMDI, isSnapMdi, false}, {IDC_INVTOGGLE, isDisableToggle, true},
                {IDC_INSIDES, isSnappingInsides, true}, {IDC_HIDEICON, isIconHidden, false},
                {IDC_CROPTOPC, isCroppingTop, false}, {IDC_KEPT, isKeptToScreen, false},
                {IDC_CROP_RGN, isCroppingRgn, true}};
            for (const Checkbox & box : boxes) {
                const bool initial = box.get() != FALSE;
                SettingsFixture::Click(page, box.control, box.inverted ? initial : !initial);
                Assert::IsTrue(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
                SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                Assert::AreEqual(!initial, box.get() != FALSE);
                SettingsFixture::Click(page, box.control, box.inverted ? !initial : initial);
                SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                Assert::AreEqual(initial, box.get() != FALSE);
            }
            Assert::IsFalse((GetWindowLongPtrW(GetDlgItem(page, IDC_AUTOLINEUP), GWL_STYLE) & WS_VISIBLE) != 0);
        }

        // ST-021: checkbox state enables each axis's actual radio/edit controls.
        TEST_METHOD(GridCheckboxesEnableOnlyTheirOwnAxisControls)
        {
            SettingsFixture fixture;
            HWND page = SettingsFixture::SelectPage(fixture.OpenSheet(), 1);
            const int horizontal[] = {IDC_RADIO_H_EVEN, IDC_RADIO_H_PIXELS, IDC_RADIO_H_SINGLE,
                IDC_H_NUM_EVEN, IDC_H_PIXELS_BTWN, IDC_H_PIXELS_FROM_TOP};
            const int vertical[] = {IDC_RADIO_V_EVEN, IDC_RADIO_V_PIXELS, IDC_RADIO_V_SINGLE,
                IDC_V_NUM_EVEN, IDC_V_PIXELS_BTWN, IDC_V_PIXELS_FROM_LEFT};
            for (int control : horizontal) Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
            for (int control : vertical) Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
            SettingsFixture::Click(page, IDC_CHECK_HORIZONTAL);
            for (int control : horizontal) Assert::IsTrue(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
            for (int control : vertical) Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
            SettingsFixture::Click(page, IDC_CHECK_VERTICAL);
            SettingsFixture::Click(page, IDC_CHECK_HORIZONTAL, false);
            for (int control : horizontal) Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
            for (int control : vertical) Assert::IsTrue(IsWindowEnabled(GetDlgItem(page, control)) != FALSE);
        }

        // ST-022: both axes, all modes, boundaries, and unused edits are covered.
        TEST_METHOD(AllGridModesAcceptEndpointsAndIgnoreUnselectedEdits)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 1);
            for (bool horizontal : {true, false}) {
                for (int mode : {GRIDTYPE_EVEN, GRIDTYPE_PIXELS, GRIDTYPE_SINGLE}) {
                    const int minimum = mode == GRIDTYPE_PIXELS ? 1 : 0;
                    const int maximum = mode == GRIDTYPE_EVEN ? 100 : INT_MAX;
                    for (int value : {minimum, maximum}) {
                        const std::wstring text = std::to_wstring(value);
                        SetGridAxis(page, horizontal, mode, text.c_str());
                        const int edits[2][3] = {{IDC_H_NUM_EVEN, IDC_H_PIXELS_BTWN, IDC_H_PIXELS_FROM_TOP},
                            {IDC_V_NUM_EVEN, IDC_V_PIXELS_BTWN, IDC_V_PIXELS_FROM_LEFT}};
                        for (int other = 0; other < 3; ++other)
                            if (other != mode) SetDlgItemTextW(page, edits[horizontal ? 0 : 1][other], L"unused");
                        Assert::IsTrue(IsWindowEnabled(GetDlgItem(sheet, ApplyButtonId)) != FALSE);
                        SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
                        Assert::AreEqual(0, TestApp_MessageCount());
                        gridsnap_settings_t actual = {};
                        getGridSnap(&actual);
                        const gridsnap_setting_t & axis = horizontal ? actual.h : actual.v;
                        Assert::IsTrue(axis.enabled != FALSE);
                        Assert::AreEqual(mode, static_cast<int>(axis.type));
                        Assert::AreEqual(value, axis.val);
                    }
                }
            }
        }

        TEST_METHOD(AdvancedCropAcceptsEndpointsAndDisablesItsEdit)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND page = SettingsFixture::SelectPage(sheet, 2);
            Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, IDC_CROPTOP)) != FALSE);
            SettingsFixture::Click(page, IDC_CROPTOPC);
            Assert::IsTrue(IsWindowEnabled(GetDlgItem(page, IDC_CROPTOP)) != FALSE);
            SetDlgItemInt(page, IDC_CROPTOP, INT_MAX, FALSE);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::AreEqual(INT_MAX, getCropTop());
            Assert::IsTrue(isCroppingTop());
            SetDlgItemInt(page, IDC_CROPTOP, 0, FALSE);
            SettingsFixture::Press(sheet, PSBTN_APPLYNOW);
            Assert::AreEqual(0, getCropTop());
            Assert::IsFalse(isCroppingTop());
            Assert::IsFalse(IsWindowEnabled(GetDlgItem(page, IDC_CROPTOP)) != FALSE);
        }
    };
}
