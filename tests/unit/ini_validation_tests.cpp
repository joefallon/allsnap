#include "../support/SettingsFixture.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(IniValidationUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        // INI-001: invalid persisted numbers must not replace valid settings.
        TEST_METHOD(MalformedThresholdsPreserveCurrentValues)
        {
            SettingsFixture fixture;
            for (LPCWSTR invalid : {L"garbage", L"23junk", L"2147483648", L"-2147483649", L"", L"-1", L"0", L"101"}) {
                setWinThresh(23);
                setScreenThresh(31);
                Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"win_tresh", invalid, fixture.Path()) != FALSE);
                Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"screen_tresh", invalid, fixture.Path()) != FALSE);
                Assert::IsTrue(LoadSettingsFromINI());
                Assert::AreEqual(23, getWinThresh(), invalid);
                Assert::AreEqual(31, getScreenThresh(), invalid);
            }
        }

        TEST_METHOD(ThresholdBoundariesAndWhitespaceRemainValid)
        {
            SettingsFixture fixture;
            Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"win_tresh", L" 2 ", fixture.Path()) != FALSE);
            Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"screen_tresh", L"100", fixture.Path()) != FALSE);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(2, getWinThresh());
            Assert::AreEqual(100, getScreenThresh());
        }

        TEST_METHOD(LegacyHexValuesAndLeadingZeroDecimalsRemainValid)
        {
            SettingsFixture fixture;
            WritePrivateProfileStringW(L"settings", L"toggle_key", L"0x12", fixture.Path());
            WritePrivateProfileStringW(L"settings", L"win_tresh", L"0009", fixture.Path());
            WritePrivateProfileStringW(L"settings", L"screen_tresh", L"0x64", fixture.Path());
            Assert::AreEqual<UINT>(VK_MENU, GetPrivateProfileIntW(L"settings", L"toggle_key", 0, fixture.Path()));
            setToggleKey(VK_TAB);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual<UINT>(VK_MENU, getToggleKey());
            Assert::AreEqual(9, getWinThresh());
            Assert::AreEqual(100, getScreenThresh());
        }

        // INI-002: BOOL consumers use canonical TRUE/FALSE, including equality checks.
        TEST_METHOD(NonzeroPersistedBooleansBecomeCanonicalTrue)
        {
            SettingsFixture fixture;
            for (LPCWSTR value : {L"2", L"-1", L"-67"}) {
                Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"disable_toggle", value, fixture.Path()) != FALSE);
                Assert::IsTrue(LoadSettingsFromINI());
                Assert::AreEqual<BOOL>(TRUE, isDisableToggle());
            }
        }

        // INI-003: missing class lists preserve state; present empty lists clear it.
        TEST_METHOD(ExplicitEmptyClassListsClearWithoutChangingMissingKeySemantics)
        {
            SettingsFixture fixture;
            std::wstring ignored(getIgnoredClasses()), skinned(getSkinnedClasses());
            WCHAR name[] = L"allSnap retained class";
            setIgnoredClasses(name, lstrlenW(name));
            setSkinnedClasses(name, lstrlenW(name));
            Assert::IsTrue(LoadSettingsFromINI());
            bool missingPreserved = std::wstring(getIgnoredClasses()) == name && std::wstring(getSkinnedClasses()) == name;
            WritePrivateProfileStringW(L"settings", L"ignored_classes", L"", fixture.Path());
            WritePrivateProfileStringW(L"settings", L"skinned_classes", L"", fixture.Path());
            Assert::IsTrue(LoadSettingsFromINI());
            bool cleared = *getIgnoredClasses() == 0 && *getSkinnedClasses() == 0;
            setIgnoredClasses(&ignored[0], static_cast<int>(ignored.size()));
            setSkinnedClasses(&skinned[0], static_cast<int>(skinned.size()));
            Assert::IsTrue(missingPreserved);
            Assert::IsTrue(cleared, L"Explicit empty lists must clear earlier values.");
        }
    };
}
