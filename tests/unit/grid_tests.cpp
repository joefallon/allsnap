#include "../support/EngineFixture.h"
#include <climits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(GridUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }
        TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

        TEST_METHOD(ValidIntegerSpacingIsPreserved)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_PIXELS, 25}, {TRUE, GRIDTYPE_SINGLE, 400}};
            setGridSnap(&settings);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::IsTrue(actual.h.enabled != FALSE);
            Assert::AreEqual(25, actual.h.val);
            Assert::AreEqual(400, actual.v.val);
        }

        TEST_METHOD(InvalidGridTypesAreDisabledAndSafeForUiIndexing)
        {
            gridsnap_settings_t settings = {{TRUE, static_cast<gridsnap_type_t>(-1), 3},
                {TRUE, static_cast<gridsnap_type_t>(INT_MAX), 3}};
            setGridSnap(&settings);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::IsFalse(actual.h.enabled != FALSE);
            Assert::IsFalse(actual.v.enabled != FALSE);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_EVEN), static_cast<int>(actual.h.type));
            Assert::AreEqual(static_cast<int>(GRIDTYPE_EVEN), static_cast<int>(actual.v.type));
        }

        TEST_METHOD(NegativeGridValuesCannotReachDivision)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_EVEN, -1}, {TRUE, GRIDTYPE_PIXELS, -25}};
            setGridSnap(&settings);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::IsFalse(actual.h.enabled != FALSE);
            Assert::IsFalse(actual.v.enabled != FALSE);
            Assert::AreEqual(0, actual.h.val);
            Assert::AreEqual(0, actual.v.val);
        }

        TEST_METHOD(ExtremeEvenCountCannotOverflowItsDivisor)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_EVEN, INT_MAX}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&settings);
            gridsnap_settings_t actual = {};
            getGridSnap(&actual);
            Assert::IsFalse(actual.h.enabled != FALSE);
            Assert::AreEqual(0, actual.h.val);
        }

        TEST_METHOD(ZeroRetainsExistingNoGuidesBehavior)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_PIXELS, 0}, {TRUE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&settings);
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {100, 100, 300, 300};
            WinRects_TestRefreshScreens(screens, 1, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(1, candidates.num_screens);
            Assert::AreEqual(1, WinRects_GetNumScreens());
        }

        TEST_METHOD(DenseGridsReserveSpaceForAllWorkAreas)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_PIXELS, 1}, {TRUE, GRIDTYPE_PIXELS, 1}};
            setGridSnap(&settings);
            const RECT screens[] = {{0, 0, 1000, 1000}, {1000, 0, 2000, 1000}};
            const RECT window = {100, 100, 300, 300};
            WinRects_TestRefreshScreens(screens, 2, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(2, WinRects_GetNumScreens());
            AssertRect(screens[0], WinRects_GetScreens()[0]);
            AssertRect(screens[1], WinRects_GetScreens()[1]);
            Assert::AreEqual(102, candidates.num_screens, L"Two work areas plus at most 100 guides");
        }

        TEST_METHOD(CenterArraysStopAtTheirActualCapacity)
        {
            RECT screens[11];
            for (int index = 0; index < 11; ++index)
                screens[index] = {index * 1000, 0, (index + 1) * 1000, 1000};
            const RECT window = {100, 100, 300, 300};
            WinRects_TestRefreshScreens(screens, 11, SNAPT_DESKTOP | SNAPT_HCENTER | SNAPT_VCENTER, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(10, WinRects_GetNumScreens());
            Assert::AreEqual(10, candidates.num_hcenter_move_rects);
            Assert::AreEqual(10, candidates.num_hcenter_size_rects);
            Assert::AreEqual(10, candidates.num_vcenter_move_rects);
            Assert::AreEqual(10, candidates.num_vcenter_size_rects);
        }

        TEST_METHOD(SingleGuideOutsideWorkAreaDoesNotOverflowCoordinates)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_SINGLE, INT_MAX}, {TRUE, GRIDTYPE_SINGLE, INT_MAX}};
            setGridSnap(&settings);
            const RECT screens[] = {{1000, 1000, 2000, 2000}};
            const RECT window = {1100, 1100, 1300, 1300};
            WinRects_TestRefreshScreens(screens, 1, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(1, candidates.num_screens);
        }

        TEST_METHOD(WorkAreaClassificationExcludesGridAndCenterGuides)
        {
            gridsnap_settings_t settings = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&settings);
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = screens[0];
            WinRects_TestRefreshScreens(screens, 1, SNAPT_DESKTOP | SNAPT_HCENTER, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(2, candidates.num_screens);
            Assert::IsTrue(WinRects_IsWorkArea(&candidates.screens[0]) != FALSE);
            Assert::IsFalse(WinRects_IsWorkArea(&candidates.screens[1]) != FALSE);
            Assert::AreEqual(1, candidates.num_hcenter_move_rects);
            AssertRect(screens[0], candidates.hcenter_move_rects[0]);
            Assert::IsFalse(WinRects_IsWorkArea(&candidates.hcenter_move_rects[0]) != FALSE,
                L"A center guide remains a guide even when its coordinates match a work area");
        }

        TEST_METHOD(EvenHorizontalGuidesDistributeRoundingAcrossFullHeight)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_EVEN, 100}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&settings);
            const RECT area = {0, 0, 1000, 1000}, window = {100, 100, 300, 300};
            WinRects_TestRefreshScreens(&area, 1, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(101, candidates.num_screens);
            Assert::AreEqual(10L, candidates.screens[1].top);
            Assert::AreEqual(990L, candidates.screens[100].top,
                L"100 interior guides divide a 1000-pixel height into approximately equal intervals.");
        }

        TEST_METHOD(EvenVerticalGuidesDistributeRoundingWithNegativeOrigin)
        {
            gridsnap_settings_t settings = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_EVEN, 100}};
            setGridSnap(&settings);
            const RECT area = {-500, -400, 500, 600}, window = {-400, -300, -200, -100};
            WinRects_TestRefreshScreens(&area, 1, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(101, candidates.num_screens);
            Assert::AreEqual(-490L, candidates.screens[1].left);
            Assert::AreEqual(490L, candidates.screens[100].left);
        }

        TEST_METHOD(EvenGuidesOnTinyWorkAreaAreDistinctInteriorLines)
        {
            gridsnap_settings_t settings = {{TRUE, GRIDTYPE_EVEN, 100}, {TRUE, GRIDTYPE_EVEN, 100}};
            setGridSnap(&settings);
            const RECT area = {0, 0, 4, 3}, window = {0, 0, 2, 2};
            WinRects_TestRefreshScreens(&area, 1, SNAPT_DESKTOP, &window);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(6, candidates.num_screens,
                L"Only two horizontal and three vertical interior integer positions exist.");
            AssertRect({0, 1, 4, 1}, candidates.screens[1]);
            AssertRect({0, 2, 4, 2}, candidates.screens[2]);
            AssertRect({1, 0, 1, 3}, candidates.screens[3]);
            AssertRect({2, 0, 2, 3}, candidates.screens[4]);
            AssertRect({3, 0, 3, 3}, candidates.screens[5]);
        }
    };
}
