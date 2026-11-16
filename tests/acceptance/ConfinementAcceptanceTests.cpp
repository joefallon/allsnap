#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(ConfinementAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            setKeptToScreen(TRUE);
        }

        // DC-001: confinement is independent of enabling proximity snapping.
        TEST_METHOD(MoveCannotCrossLeftDesktopBoundary)
        {
            AssertRect({0, 100, 200, 300}, Move({-25, 100, 175, 300}));
        }

        TEST_METHOD(MoveCannotCrossRightDesktopBoundary)
        {
            AssertRect({1720, 100, 1920, 300}, Move({1820, 100, 2020, 300}));
        }

        TEST_METHOD(MoveCannotCrossTopDesktopBoundary)
        {
            AssertRect({100, 0, 300, 200}, Move({100, -25, 300, 175}));
        }

        // DC-002: the work area reserves the taskbar region.
        TEST_METHOD(MoveStopsAboveTaskbarWorkAreaBoundary)
        {
            AssertRect({100, 840, 300, 1040}, Move({100, 940, 300, 1140}));
        }

        // DC-003: confinement covers virtual desktops with negative coordinates.
        TEST_METHOD(MoveConfinementSupportsNegativeMonitorCoordinates)
        {
            SetScreens({{-1920, -200, 0, 880}});
            AssertRect({-1920, -100, -1720, 100}, Move({-1940, -100, -1740, 100}));
        }

        // DC-004: the internal monitor seam is not an outside desktop boundary.
        TEST_METHOD(MoveMayCrossBetweenAdjacentMonitors)
        {
            SetScreens({{0, 0, 1920, 1040}, {1920, 0, 3840, 1040}});
            const RECT proposed = {1850, 100, 2050, 300};
            AssertRect(proposed, Move(proposed));
        }

        TEST_METHOD(MoveMayEnterStaggeredMonitorWhereWorkAreasOverlap)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            const RECT proposed = {900, 300, 1100, 500};
            AssertRect(proposed, Move(proposed));
        }

        // DC-005: moving confinement never shrinks an oversized window.
        TEST_METHOD(OversizedWindowRetainsItsDimensionsAndAccessibleOrigin)
        {
            const RECT proposed = {-10, 100, 2100, 300};
            const RECT actual = Move(proposed);
            Assert::AreEqual(rectWidth(proposed), rectWidth(actual));
            Assert::AreEqual(rectHeight(proposed), rectHeight(actual));
            Assert::IsTrue(actual.left >= 0 && actual.left < 1920);
            Assert::IsTrue(actual.top >= 0 && actual.top < 1040);
        }

        // DC-006: resizing clamps the dragged edge without moving the other edge.
        TEST_METHOD(ResizeStopsDraggedRightEdgeAtDesktopBoundary)
        {
            AssertRect({100, 100, 1920, 300}, Resize({100, 100, 2050, 300}, SIDE_NONE, SIDE_RIGHT));
        }

        TEST_METHOD(ResizeStopsDraggedBottomEdgeAtTaskbarBoundary)
        {
            AssertRect({100, 100, 300, 1040}, Resize({100, 100, 300, 1150}, SIDE_BOTTOM, SIDE_NONE));
        }

        // DC-007: disabling confinement restores unconstrained movement.
        TEST_METHOD(DisabledConfinementLeavesOutsideRectangleUnchanged)
        {
            setKeptToScreen(FALSE);
            const RECT proposed = {-25, 100, 175, 300};
            AssertRect(proposed, Move(proposed));
        }

        // DC-008: separate gestures must not inherit stale snap results.
        TEST_METHOD(RepeatedGesturesRemainDeterministic)
        {
            for (int gesture = 0; gesture < 5; ++gesture)
            {
                AssertRect({0, 100, 200, 300}, Move({-25, 100, 175, 300}));
                AssertRect({100, 100, 300, 300}, Move({100, 100, 300, 300}));
            }
        }
    };
}
