#include "../support/SettingsFixture.h"
#include "snap_keys.h"
extern "C" {
#include "snapper.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace { SHORT WINAPI ReleasedKeys(int) { return 0; } }

    TEST_CLASS(IniSettingsAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_CLEANUP(Cleanup) { SnapKeys_TestSetReader(nullptr); }
        // INI-002: persisted true values must preserve actual enabled snapping.
        TEST_METHOD(NoncanonicalTrueToggleStillAllowsConfinementWithReleasedKey)
        {
            SettingsFixture fixture;
            setSnapType(SNAPT_NONE);
            setKeptToScreen(TRUE);
            SnapKeys_TestSetReader(ReleasedKeys);
            WritePrivateProfileStringW(L"settings", L"disable_toggle", L"2", fixture.Path());
            Assert::IsTrue(LoadSettingsFromINI());
            SetScreens({{0, 0, 1000, 800}});
            RECT proposed = {-50, 100, 150, 300};
            snapper_OnMoving(nullptr, &proposed);
            SnapKeys_TestSetReader(nullptr);
            AssertRect({0, 100, 200, 300}, proposed);
        }
    };
}
