#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(ConfinementSpacingAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            setWindowSpacing(8);
            setKeptToScreen(TRUE);
        }
        TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

        // DC-009: spacing pads exposed desktop borders, not connected monitor seams.
        TEST_METHOD(WindowMayApproachSharedSeamFromTheLeft)
        {
            SetScreens({{0, 0, 1920, 1040}, {1920, 0, 3840, 1040}});
            const RECT proposed = {1718, 100, 1918, 300};
            AssertRect(proposed, Move(proposed));
        }

        TEST_METHOD(WindowMayCrossSharedSeamWithItsLeftEdge)
        {
            SetScreens({{0, 0, 1920, 1040}, {1920, 0, 3840, 1040}});
            const RECT proposed = {1918, 100, 2118, 300};
            AssertRect(proposed, Move(proposed));
        }

        TEST_METHOD(WindowMayStraddleSharedSeam)
        {
            SetScreens({{0, 0, 1920, 1040}, {1920, 0, 3840, 1040}});
            const RECT proposed = {1850, 100, 2050, 300};
            AssertRect(proposed, Move(proposed));
        }

        TEST_METHOD(NarrowWindowCanOccupyTheConnectedSeam)
        {
            SetScreens({{0, 0, 1920, 1040}, {1920, 0, 3840, 1040}});
            const RECT proposed = {1918, 100, 1922, 300};
            AssertRect(proposed, Move(proposed));
        }

        TEST_METHOD(SharedSeamAtNegativeCoordinatesRemainsOpen)
        {
            SetScreens({{-1000, 0, 0, 800}, {0, 0, 1000, 800}});
            const RECT proposed = {-2, 100, 198, 300};
            AssertRect(proposed, Move(proposed));
        }

        // DC-010: each exposed segment retains its margin on staggered layouts.
        TEST_METHOD(StaggeredMonitorOverlapRemainsOpenOnBothSides)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            const RECT onLeft = {798, 300, 998, 500};
            const RECT onRight = {998, 300, 1198, 500};
            AssertRect(onLeft, Move(onLeft));
            AssertRect(onRight, Move(onRight));
        }

        TEST_METHOD(StaggeredExposedUpperEdgeRetainsItsMargin)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            AssertRect({792, 50, 992, 150}, Move({810, 50, 1010, 150}));
        }

        TEST_METHOD(StaggeredConcaveCornerRetainsItsMargin)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            AssertRect({900, 208, 1100, 408}, Move({900, 205, 1100, 405}));
        }

        TEST_METHOD(StaggeredExposedLowerEdgeRetainsItsMargin)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            // Expanded by S=8, this result is {802,684,1018,800}, fully inside A+B.
            // Moving up 158 pixels is closer than moving right 198 pixels into B alone.
            AssertRect({810, 692, 1010, 792}, Move({810, 850, 1010, 950}));
        }

        TEST_METHOD(StaggeredConcaveEndpointUsesTheNearestValidCorrection)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            const RECT proposed = {795, 205, 995, 405};
            const RECT actual = Move(proposed);
            const LONG dx = actual.left - proposed.left;
            const LONG dy = actual.top - proposed.top;
            Assert::AreEqual(200L, rectWidth(actual));
            Assert::AreEqual(200L, rectHeight(actual));
            Assert::AreEqual(3L, (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy),
                L"Either nearest axis may clear the corner; no distant desktop-edge jump is necessary");
            Assert::IsTrue(actual.right <= 992 || actual.top >= 208);
        }

        TEST_METHOD(DraggedResizeEdgeCanCrossAnInternalHorizontalSeam)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            const RECT proposed = {100, 300, 1005, 500};
            AssertRect(proposed, Resize(proposed, SIDE_NONE, SIDE_RIGHT));
        }

        TEST_METHOD(DraggedResizeEdgeCanCrossAnInternalVerticalSeam)
        {
            SetScreens({{0, 0, 1000, 800}, {0, 800, 1000, 1600}});
            const RECT proposed = {100, 100, 300, 805};
            AssertRect(proposed, Resize(proposed, SIDE_BOTTOM, SIDE_NONE));
        }

        TEST_METHOD(MissingWorkAreasCannotInventAConfinementRectangle)
        {
            SetScreens({});
            const RECT proposed = {-100, -100, 100, 100};
            AssertRect(proposed, Move(proposed));
            AssertRect(proposed, Resize(proposed, SIDE_TOP, SIDE_LEFT));
        }

        // DC-011: a fully off-desktop corner move still has a known nearest work area.
        TEST_METHOD(FullyOffDesktopTopLeftMoveRecoversWithoutChangingSize)
        {
            setWindowSpacing(0);
            AssertRect({0, 0, 200, 200}, Move({-300, -300, -100, -100}));
            setWindowSpacing(8);
            AssertRect({8, 8, 208, 208}, Move({-300, -300, -100, -100}));
        }

        TEST_METHOD(FullyOffDesktopBottomRightMoveRecoversWithoutChangingSize)
        {
            setWindowSpacing(0);
            AssertRect({1720, 840, 1920, 1040}, Move({2000, 1200, 2200, 1400}));
            setWindowSpacing(8);
            AssertRect({1712, 832, 1912, 1032}, Move({2000, 1200, 2200, 1400}));
        }

        TEST_METHOD(OversizedOffDesktopFallbackKeepsItsOriginReachableAndSizeIntact)
        {
            AssertRect({8, 8, 2118, 208}, Move({-3000, -3000, -890, -2800}));
        }

        TEST_METHOD(CoupledCornerCorrectionKeepsTheCompleteWindowOnTheNearestArea)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            AssertRect({792, 42, 992, 792}, Move({900, 100, 1100, 850}));
        }

        TEST_METHOD(DistantAlignedMonitorCannotOverrideANearerValidPlacement)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 4000, 2000, 4800}});
            AssertRect({792, 592, 992, 792}, Move({1200, 900, 1400, 1100}));
        }

        TEST_METHOD(WindowCannotBridgeAnUnavailableGapBetweenWorkAreas)
        {
            SetScreens({{0, 0, 800, 1000}, {1200, 0, 2000, 1000}});
            const RECT actual = Move({700, 100, 1300, 300});
            Assert::AreEqual(600L, rectWidth(actual));
            Assert::AreEqual(200L, rectHeight(actual));
            Assert::AreEqual(100L, actual.top);
            Assert::IsTrue(actual.left == 192 || actual.left == 1208,
                L"Either equally near complete placement is valid; straddling the unavailable gap is not");
        }

        TEST_METHOD(WindowCanFitAcrossConnectedAreasEvenWhenNeitherAreaFitsItAlone)
        {
            SetScreens({{0, 0, 800, 600}, {800, 0, 1600, 600}});
            AssertRect({8, 100, 1208, 300}, Move({-50, 100, 1150, 300}));
        }

        TEST_METHOD(ResizingCannotBridgeAnUnavailableHorizontalGap)
        {
            SetScreens({{0, 0, 800, 1000}, {1200, 0, 2000, 1000}});
            AssertRect({100, 100, 792, 300}, Resize({100, 100, 1300, 300}, SIDE_NONE, SIDE_RIGHT));
        }

        TEST_METHOD(ResizingCannotBridgeAnUnavailableVerticalGap)
        {
            SetScreens({{0, 0, 1000, 800}, {0, 1200, 1000, 2000}});
            AssertRect({100, 100, 300, 792}, Resize({100, 100, 300, 1300}, SIDE_BOTTOM, SIDE_NONE));
        }

        TEST_METHOD(CornerResizeClampsBothDraggedEdgesWithoutMovingTheirOpposites)
        {
            SetScreens({{0, 0, 1000, 800}, {1000, 200, 2000, 1000}});
            AssertRect({900, 100, 992, 792}, Resize({900, 100, 1100, 850}, SIDE_BOTTOM, SIDE_RIGHT));
        }

        TEST_METHOD(ConfinementClampsAnOtherwiseValidMoveAfterProximitySnapping)
        {
            setWindowSpacing(0);
            setSnapType(SNAPT_OTHERS);
            SetWindows({{1925, 100, 2125, 400}});
            AssertRect({1720, 150, 1920, 350}, Move({1718, 150, 1918, 350}));
        }

        TEST_METHOD(ConfinementClampsTheDraggedEdgeAfterProximityResizeSnapping)
        {
            setWindowSpacing(0);
            setSnapType(SNAPT_OTHERS);
            SetWindows({{1925, 100, 2125, 400}});
            AssertRect({100, 150, 1920, 350}, Resize({100, 150, 1918, 350}, SIDE_NONE, SIDE_RIGHT));
        }
    };
}
