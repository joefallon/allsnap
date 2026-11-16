#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
TEST_CLASS(TrackingUnitTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }
    TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

    TEST_METHOD(TrackingOwnsTheLastGeometryAndResetClearsBothAxes) {
        SNAP_RESULTS input = {};
        input.h = {SIDE_LEFT, SIDE_RIGHT, 500, 3, 10, TRUE};
        input.v = {SIDE_BOTTOM, SIDE_TOP, 700, 4, 12, FALSE};
        TrackRslts_Track(&input);
        input.h.value = -100;
        const SNAP_RESULTS * last = TrackRslts_GetLast();
        Assert::IsTrue(last->h.side == SIDE_LEFT && last->h.to_side == SIDE_RIGHT);
        Assert::AreEqual(500, last->h.value); Assert::AreEqual(10, last->h.thresh);
        Assert::IsTrue(last->h.from_oss != FALSE);
        Assert::IsTrue(last->v.side == SIDE_BOTTOM && last->v.to_side == SIDE_TOP);
        Assert::AreEqual(700, last->v.value); Assert::AreEqual(12, last->v.thresh);
        TrackRslts_Reset();
        Assert::IsTrue(NO_SNAP(*TrackRslts_GetLast()));
        Assert::IsFalse(TrackRslts_GetLast()->h.from_oss != FALSE);
        Assert::IsFalse(TrackRslts_GetLast()->v.from_oss != FALSE);
    }

    TEST_METHOD(RepeatedMovingSnapsAndAxisReleasesPreserveGeometry) {
        setSnapType(SNAPT_DESKTOP);
        for (int cycle = 0; cycle < 8; ++cycle) {
            const RECT proposed[] = {{7, 17, 207, 217}, {25, 5, 225, 205}, {25, 25, 225, 225}};
            const RECT expected[] = {{0, 17, 200, 217}, {25, 0, 225, 200}, {25, 25, 225, 225}};
            for (int index = 0; index < 3; ++index) {
                SNAP_RESULTS results = EvaluateMove(proposed[index]);
                RECT actual = proposed[index];
                reposition(&proposed[index], &results, &actual);
                AssertRect(expected[index], actual);
                TrackRslts_Track(&results);
            }
            Assert::IsTrue(NO_SNAP(*TrackRslts_GetLast()));
        }
    }

    TEST_METHOD(RepeatedResizingSnapsAndReleasesKeepOppositeEdgesFixed) {
        setSnapType(SNAPT_DESKTOP);
        TEST_RECTS candidates = {}; WinRects_getRects(&candidates);
        for (int cycle = 0; cycle < 8; ++cycle) {
            const RECT proposed[] = {{100, 100, 1913, 1033}, {100, 100, 1890, 1010}};
            const RECT expected[] = {{100, 100, 1920, 1040}, {100, 100, 1890, 1010}};
            for (int index = 0; index < 2; ++index) {
                SNAP_RESULTS results = {};
                sizing_test_all(SIDE_BOTTOM, SIDE_RIGHT, &proposed[index], getWinThresh(),
                    getScreenThresh(), &candidates, &results, FALSE);
                RECT actual = proposed[index]; resize(&results, &actual);
                AssertRect(expected[index], actual); TrackRslts_Track(&results);
            }
            Assert::IsTrue(NO_SNAP(*TrackRslts_GetLast()));
        }
    }
};
}
