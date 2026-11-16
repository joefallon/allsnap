#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(GeometryUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }

        TEST_METHOD(OppositeSidesRoundTrip)
        {
            const SIDE sides[] = {SIDE_LEFT, SIDE_RIGHT, SIDE_TOP, SIDE_BOTTOM};
            for (SIDE side : sides)
                Assert::AreEqual(static_cast<int>(side), static_cast<int>(OppositeSide(OppositeSide(side))));
        }

        TEST_METHOD(AlignmentPreservesWidthAndHeight)
        {
            const SIDE sides[] = {SIDE_LEFT, SIDE_RIGHT, SIDE_TOP, SIDE_BOTTOM};
            for (SIDE side : sides)
            {
                RECT rectangle = {-100, -50, 200, 150};
                AlignToSide(&rectangle, side, 30);
                Assert::AreEqual(30, GetSideOfRect(side, &rectangle));
                Assert::AreEqual<LONG>(300, rectWidth(rectangle));
                Assert::AreEqual<LONG>(200, rectHeight(rectangle));
            }
        }

        TEST_METHOD(SettingOneEdgeLeavesOppositeEdgeFixed)
        {
            RECT rectangle = {10, 20, 110, 220};
            SetSideOfRect(SIDE_RIGHT, 150, &rectangle);
            AssertRect({10, 20, 150, 220}, rectangle);
        }

        TEST_METHOD(SizingCornerSplitsIntoItsTwoEdges)
        {
            SIDE vertical = SIDE_NONE, horizontal = SIDE_NONE;
            split_edge(WMSZ_BOTTOMLEFT, &vertical, &horizontal);
            Assert::AreEqual(static_cast<int>(SIDE_BOTTOM), static_cast<int>(vertical));
            Assert::AreEqual(static_cast<int>(SIDE_LEFT), static_cast<int>(horizontal));
        }

        TEST_METHOD(CornerMembershipRecognizesNegativeCoordinates)
        {
            SetScreens({{-1920, -200, 0, 880}});
            CORNER_INFO corners = {};
            RECT rectangle = {-1000, -100, -800, 100};
            InTests_TestAllCorners(&corners, &rectangle);
            Assert::IsFalse(corners.topLeft_out || corners.topRight_out ||
                corners.bottomLeft_out || corners.bottomRight_out);
        }

        TEST_METHOD(NearestScreenSelectionUsesAlignedWorkArea)
        {
            SetScreens({{-1920, 0, 0, 1040}, {0, 0, 1920, 1040}});
            RECT rectangle = {1800, 100, 2000, 300}, selected = {};
            Assert::AreEqual(80, InTests_GetClosestScreenToSide(SIDE_RIGHT, &rectangle, &selected));
            AssertRect({0, 0, 1920, 1040}, selected);
        }

        TEST_METHOD(NoAlignedScreenReturnsFailure)
        {
            RECT rectangle = {100, 2000, 300, 2200}, selected = {};
            Assert::AreEqual(-1, InTests_GetClosestScreenToSide(SIDE_LEFT, &rectangle, &selected));
        }

        TEST_METHOD(ClearResultsRemovesBothSnaps)
        {
            SNAP_RESULTS results = {};
            results.h.side = SIDE_LEFT;
            results.v.side = SIDE_TOP;
            clear_snap_results(&results);
            Assert::IsTrue(NO_SNAP(results));
        }

        TEST_METHOD(NoCandidatesProducesNoSnap)
        {
            SetScreens({});
            setSnapType(SNAPT_OTHERS | SNAPT_DESKTOP);
            RECT rectangle = {100, 100, 300, 300};
            SNAP_RESULTS results = EvaluateMove(rectangle);
            Assert::IsTrue(NO_SNAP(results));
        }

        TEST_METHOD(SnapAtDistanceBelowThreshold)
        {
            setSnapType(SNAPT_DESKTOP);
            RECT rectangle = {9, 100, 209, 300};
            SNAP_RESULTS results = EvaluateMove(rectangle);
            Assert::AreEqual(static_cast<int>(SIDE_LEFT), static_cast<int>(results.h.side));
            Assert::AreEqual(0, results.h.value);
        }

        TEST_METHOD(ExactThresholdActivatesSnap)
        {
            setSnapType(SNAPT_DESKTOP);
            RECT rectangle = {10, 100, 210, 300};
            SNAP_RESULTS results = EvaluateMove(rectangle);
            Assert::AreEqual(static_cast<int>(SIDE_LEFT),static_cast<int>(results.h.side));
            Assert::AreEqual(0,results.h.value);
        }

        TEST_METHOD(WindowThresholdDoesNotReplaceScreenThreshold)
        {
            setSnapType(SNAPT_DESKTOP);
            setWinThresh(100);
            setScreenThresh(3);
            RECT rectangle = {4, 100, 204, 300};
            SNAP_RESULTS results = EvaluateMove(rectangle);
            Assert::IsTrue(NO_SNAP(results));
        }
    };
}
