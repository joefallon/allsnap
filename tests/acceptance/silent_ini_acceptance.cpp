#include "../support/SettingsFixture.h"
#include "snap_keys.h"
extern "C" {
#include "snapper.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        SHORT WINAPI SilentReleasedKeys(int) { return 0; }
        void WriteLegacyAudio(LPCWSTR path)
        {
            Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"play_sounds", L"7", path) != FALSE);
            Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"snap_sound_file", L"Z:\\retired-audio\\missing.wav", path) != FALSE);
            Assert::IsTrue(WritePrivateProfileStringW(L"settings", L"unsnap_sound_file", L"Z:\\retired-audio\\missing2.wav", path) != FALSE);
        }
    }

    TEST_CLASS(SilentIniAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_CLEANUP(Cleanup) { SnapKeys_TestSetReader(nullptr); }

        // AU-003: old audio keys are ignored while normal settings and confinement work.
        TEST_METHOD(LegacyAudioKeysDoNotPreventLoadingOrConfinement)
        {
            SettingsFixture fixture;
            WriteLegacyAudio(fixture.Path());
            WritePrivateProfileStringW(L"settings", L"win_tresh", L"23", fixture.Path());
            WritePrivateProfileStringW(L"settings", L"window_spacing", L"8", fixture.Path());
            WritePrivateProfileStringW(L"settings", L"kept_to_screen", L"1", fixture.Path());
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(8, getWindowSpacing());
            Assert::IsTrue(isKeptToScreen());
            SnapKeys_TestSetReader(SilentReleasedKeys);
            SetScreens({{0, 0, 1000, 800}});
            RECT proposed = {-50, 100, 150, 300};
            snapper_OnMoving(nullptr, &proposed);
            AssertRect({8, 100, 208, 300}, proposed);
        }

        // AU-004: fresh configuration saves create no retired settings.
        TEST_METHOD(FreshSaveContainsOnlyCurrentSettings)
        {
            SettingsFixture fixture;
            setWinThresh(23);
            setWindowSpacing(8);
            Assert::IsTrue(SaveSettingsToINI());
            for (LPCWSTR name : {L"play_sounds", L"snap_sound_file", L"unsnap_sound_file"}) {
                WCHAR value[32] = {};
                GetPrivateProfileStringW(L"settings", name, L"not-present", value, 32, fixture.Path());
                Assert::AreEqual(L"not-present", value);
            }
            ResetEngine();
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(8, getWindowSpacing());
        }

        TEST_METHOD(SavePreservesOtherPreferencesAndDoesNotRewriteObsoleteKeys)
        {
            SettingsFixture fixture;
            WriteLegacyAudio(fixture.Path());
            WritePrivateProfileStringW(L"settings", L"user_note", L"retained", fixture.Path());
            Assert::IsTrue(LoadSettingsFromINI());
            setScreenThresh(31);
            Assert::IsTrue(SaveSettingsToINI());
            Assert::AreEqual<UINT>(31, GetPrivateProfileIntW(L"settings", L"screen_tresh", 0, fixture.Path()));
            Assert::AreEqual<UINT>(7, GetPrivateProfileIntW(L"settings", L"play_sounds", 0, fixture.Path()));
            WCHAR value[MAX_PATH] = {};
            GetPrivateProfileStringW(L"settings", L"snap_sound_file", L"", value, MAX_PATH, fixture.Path());
            Assert::AreEqual(L"Z:\\retired-audio\\missing.wav", value);
            GetPrivateProfileStringW(L"settings", L"user_note", L"", value, MAX_PATH, fixture.Path());
            Assert::AreEqual(L"retained", value);
        }
    };
}
