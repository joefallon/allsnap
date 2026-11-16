#include "../support/SettingsFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(NoSoundSettingsAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // ST-023: the actual native sheet exposes only the three retained pages.
        TEST_METHOD(NativeSettingsHasExactlyPreferencesGridAndAdvanced)
        {
            SettingsFixture fixture;
            HWND sheet = fixture.OpenSheet();
            HWND tabs = PropSheet_GetTabControl(sheet);
            Assert::IsNotNull(tabs);
            Assert::AreEqual(3, TabCtrl_GetItemCount(tabs));
            const LPCWSTR titles[] = {L"Preferences", L"Snap to grid", L"Advanced"};
            const int controls[] = {IDC_WIN_THRESH, IDC_CHECK_HORIZONTAL, IDC_KEPT};
            for (int index = 0; index < 3; ++index) {
                WCHAR text[64] = {};
                TCITEMW item = {};
                item.mask = TCIF_TEXT;
                item.pszText = text;
                item.cchTextMax = 64;
                Assert::IsTrue(TabCtrl_GetItem(tabs, index, &item) != FALSE);
                Assert::AreEqual(titles[index], text);
                HWND page = SettingsFixture::SelectPage(sheet, index);
                Assert::IsNotNull(GetDlgItem(page, controls[index]));
            }
        }

        TEST_METHOD(RetiredSoundDialogIsNotEmbedded)
        {
            SettingsFixture fixture;
            // Resource 169 was the removed Sounds page. It must not remain as
            // a dormant dialog in the application resource image.
            Assert::IsNull(FindResourceW(g_hInst, MAKEINTRESOURCEW(169), RT_DIALOG));
        }
    };
}
