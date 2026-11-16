#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(GridAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }
        TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

        TEST_METHOD(MG01_GridGuidesDoNotHideLaterMonitorWorkAreas)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}, {1000, 0, 2000, 1000}};
            // Keep both vertical edges outside the horizontal guides' snap threshold.
            const RECT window = {1900, 117, 2100, 317};
            gridsnap_settings_t grid = {{TRUE, GRIDTYPE_EVEN, 20}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            setKeptToScreen(TRUE);
            WinRects_TestRefreshScreens(screens, 2, getSnapType(), &window);
            AssertRect({1800, 117, 2000, 317}, Move(window));
        }

        TEST_METHOD(MG02_GridLinesDoNotBecomeConfinementWorkAreas)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {100, 100, 300, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid);
            WinRects_TestRefreshScreens(screens, 1, SNAPT_DESKTOP, &window);
            Assert::AreEqual(1, WinRects_GetNumScreens());
            AssertRect(screens[0], WinRects_GetScreens()[0]);
        }

        TEST_METHOD(MG03_ValidGridRemainsASnappingCandidate)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {493, 100, 693, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
            AssertRect({500, 100, 700, 300}, Move(window));
        }

        TEST_METHOD(MG04_InvalidGridSettingsLeaveConfinementAvailable)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}, {1000, 0, 2000, 1000}};
            const RECT window = {1900, 100, 2100, 300};
            gridsnap_settings_t grid = {{TRUE, static_cast<gridsnap_type_t>(99), 10},
                {TRUE, GRIDTYPE_PIXELS, -10}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            setKeptToScreen(TRUE);
            WinRects_TestRefreshScreens(screens, 2, getSnapType(), &window);
            AssertRect({1800, 100, 2000, 300}, Move(window));
        }
    };
}
