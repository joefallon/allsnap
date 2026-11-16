#include "../support/SettingsFixture.h"
#include <shellapi.h>

extern "C" {
int TestApp_NotifyCount(void);
DWORD TestApp_NotifyOperation(int);
const NOTIFYICONDATA * TestApp_NotifyData(int);
void TestApp_SetNotifySuccess(BOOL);
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(TraySettingsAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // TR-001: Apply reaches the real tray controller; only Explorer is substituted.
        TEST_METHOD(PreferencesApplyUpdatesEnabledAndDisabledTrayTips)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_GENERAL, PrefsProc);
            for (bool enabled : {true, false}) {
                SettingsFixture::Check(page, IDC_ENABLED, enabled);
                SettingsFixture::Notify(page, PSN_APPLY);
                int last = TestApp_NotifyCount() - 1;
                Assert::IsTrue(last >= 0);
                Assert::AreEqual<DWORD>(NIM_MODIFY, TestApp_NotifyOperation(last));
                const NOTIFYICONDATA * data = TestApp_NotifyData(last);
                Assert::AreEqual<UINT>(NIF_ICON | NIF_TIP, data->uFlags);
                Assert::IsNotNull(data->hIcon);
                WCHAR expected[100] = {};
                Assert::IsTrue(LoadStringW(g_hInst, enabled ? IDS_ENABLED_TIP : IDS_DISABLED_TIP,
                    expected, 100) > 0);
                Assert::AreEqual(expected, data->szTip);
            }
        }

        // TR-002: a hidden icon stays hidden across restart, then can be restored.
        TEST_METHOD(AdvancedHideSurvivesExplorerRestartAndCanBeUndone)
        {
            SettingsFixture fixture;
            HWND page = fixture.Page(IDD_ADVANCED, AdvProc);
            SettingsFixture::Check(page, IDC_HIDEICON);
            SettingsFixture::Notify(page, PSN_APPLY);
            Assert::IsTrue(isIconHidden() != FALSE);
            Assert::AreEqual(1, TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_DELETE, TestApp_NotifyOperation(0));
            RestartTaskbarIcon();
            Assert::AreEqual(1, TestApp_NotifyCount());
            SettingsFixture::Check(page, IDC_HIDEICON, false);
            SettingsFixture::Notify(page, PSN_APPLY);
            Assert::IsFalse(isIconHidden() != FALSE);
            Assert::AreEqual(3, TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_ADD, TestApp_NotifyOperation(1));
            Assert::AreEqual<DWORD>(NIM_MODIFY, TestApp_NotifyOperation(2));
            RestartTaskbarIcon();
            Assert::AreEqual(5, TestApp_NotifyCount());
        }

        // TR-003: recovery from a hidden saved icon must survive opening Settings.
        TEST_METHOD(OpeningSettingsPreservesRecoveredIconVisibility)
        {
            SettingsFixture fixture;
            setIconHidden(TRUE);
            Assert::IsTrue(SaveSettingsToINI());
            setIconHidden(FALSE); // Same production setter used by MYWM_UNHIDEICON.
            int notifications = TestApp_NotifyCount();
            HWND page = fixture.OpenPreferences();
            Assert::IsFalse(isIconHidden() != FALSE,
                L"Opening Settings must not hide an icon the user just recovered.");
            Assert::AreEqual(notifications, TestApp_NotifyCount(), L"Reloading must not briefly hide and restore the icon.");
            SettingsFixture::Notify(page, PSN_RESET);
            Assert::IsFalse(isIconHidden() != FALSE);
            Assert::AreEqual<UINT>(1, GetPrivateProfileIntW(L"settings", L"icon_hidden", 99, fixture.Path()));
        }

        // TR-004: a later Explorer restart can retry a failed shell registration.
        TEST_METHOD(ExplorerRestartRetriesFailedShellRegistration)
        {
            SettingsFixture fixture;
            TestApp_SetNotifySuccess(FALSE);
            Assert::IsFalse(AddTaskbarIcon() != FALSE);
            Assert::AreEqual(1, TestApp_NotifyCount());
            TestApp_SetNotifySuccess(TRUE);
            RestartTaskbarIcon();
            Assert::AreEqual(3, TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_ADD, TestApp_NotifyOperation(1));
            Assert::AreEqual<DWORD>(NIM_MODIFY, TestApp_NotifyOperation(2));
        }
    };
}
