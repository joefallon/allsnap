#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
TEST_CLASS(SpacingAcceptanceTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); setWindowSpacing(8); }

    // SP-001: one setting produces one gap, not two margins between neighbors.
    TEST_METHOD(AdjacentWindowsHaveOneCollapsedHorizontalGap) {
        setSnapType(SNAPT_OTHERS); SetWindows({{500,100,700,400}});
        AssertRect({292,150,492,350},Move({295,150,495,350}));
    }
    TEST_METHOD(WindowOnRightUsesTheSameGap) {
        setSnapType(SNAPT_OTHERS); SetWindows({{100,100,300,400}});
        AssertRect({308,150,508,350},Move({313,150,513,350}));
    }
    TEST_METHOD(VerticalNeighborsHaveTheSameCollapsedGap) {
        setSnapType(SNAPT_OTHERS); SetWindows({{100,500,400,700}});
        AssertRect({150,292,350,492},Move({150,295,350,495}));
    }
    TEST_METHOD(ResizeUsesGapWithoutMovingTheOppositeEdge) {
        setSnapType(SNAPT_OTHERS); SetWindows({{500,100,700,400}});
        AssertRect({100,150,492,350},Resize({100,150,495,350},SIDE_NONE,SIDE_RIGHT));
    }
    // SP-002: work-area margins use the same setting.
    TEST_METHOD(DesktopEdgeUsesTheSameSpacing) {
        setSnapType(SNAPT_DESKTOP);
        AssertRect({8,100,208,300},Move({13,100,213,300}));
    }
    TEST_METHOD(ConfinementRespectsTheDesktopMargin) {
        setKeptToScreen(TRUE);
        AssertRect({8,100,208,300},Move({-25,100,175,300}));
        AssertRect({1712,100,1912,300},Move({1820,100,2020,300}));
    }
    TEST_METHOD(ConfinedResizePreservesTheOppositeEdge) {
        setKeptToScreen(TRUE);
        AssertRect({100,100,1912,300},Resize({100,100,2050,300},SIDE_NONE,SIDE_RIGHT));
    }
    // SP-003: spacing is not an activation-distance setting.
    TEST_METHOD(ActivationThresholdRemainsIndependent) {
        setSnapType(SNAPT_OTHERS); setWinThresh(2); SetWindows({{500,100,700,400}});
        RECT proposed = {288,150,488,350};
        AssertRect(proposed,Move(proposed));
    }
    TEST_METHOD(ActivationMeasuresDistanceFromTheDesiredSpacedPosition) {
        setSnapType(SNAPT_OTHERS); setWinThresh(6); SetWindows({{500,100,700,400}});
        AssertRect({292,150,492,350},Move({287,150,487,350}));
    }
    TEST_METHOD(WorkAreaSnappingUsesASafeInsetOnSmallScreens) {
        SetScreens({{0,0,100,80}}); setWindowSpacing(1000); setSnapType(SNAPT_DESKTOP);
        AssertRect({49,10,69,30},Move({44,10,64,30}));
    }
    TEST_METHOD(GridGuidesAreNotPadded) {
        RECT screen = {0,0,1000,800}, window = {295,150,495,350};
        gridsnap_settings_t grid = {}; grid.v = {TRUE,GRIDTYPE_SINGLE,500};
        setGridSnap(&grid); setSnapType(SNAPT_DESKTOP);
        WinRects_TestRefreshScreens(&screen,1,SNAPT_DESKTOP,&window);
        AssertRect({300,150,500,350},Move(window));
    }
    TEST_METHOD(CenterGuidesAreNotPadded) {
        RECT screen = {0,0,1000,800}, window = {395,150,595,350};
        setSnapType(SNAPT_HCENTER);
        WinRects_TestRefreshScreens(&screen,1,SNAPT_HCENTER,&window);
        AssertRect({400,150,600,350},Move(window));
    }
    TEST_METHOD(SameSideWindowAlignmentIsNotPadded) {
        setSnapType(SNAPT_OTHERS); SetWindows({{500,100,700,400}});
        AssertRect({500,150,700,350},Move({505,150,705,350}));
    }
    // SP-004: zero spacing intentionally produces flush visible geometry.
    TEST_METHOD(ZeroSpacingRetainsFlushEdgeSnapping) {
        setWindowSpacing(0); setSnapType(SNAPT_OTHERS); SetWindows({{500,100,700,400}});
        AssertRect({300,150,500,350},Move({295,150,495,350}));
    }
};
}
