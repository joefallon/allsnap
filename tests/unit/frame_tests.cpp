#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
namespace {
    RECT logicalBounds, physicalBounds, visibleBounds;
    bool physicalContext, failContext, failPhysicalRect, failDwm, failRestore;
    int contextCalls, physicalRectCalls, dwmCalls;
    const HANDLE callerContext = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(17));
    const HANDLE physicalDpiContext = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));
    const HWND fixtureWindow = reinterpret_cast<HWND>(static_cast<UINT_PTR>(1));

    HANDLE WINAPI FrameSetContext(HANDLE context) {
        ++contextCalls;
        if (context == physicalDpiContext) {
            if (failContext) return nullptr;
            physicalContext = true;
            return callerContext;
        }
        if (context != callerContext || failRestore) return nullptr;
        physicalContext = false;
        return physicalDpiContext;
    }
    BOOL WINAPI FrameGetRect(HWND, LPRECT rectangle) {
        if (physicalContext) {
            ++physicalRectCalls;
            if (failPhysicalRect) return FALSE;
            *rectangle = physicalBounds;
        } else {
            *rectangle = logicalBounds;
        }
        return TRUE;
    }
    HRESULT WINAPI FrameGetDwm(HWND, DWORD, PVOID rectangle, DWORD) {
        ++dwmCalls;
        if (failDwm) return E_FAIL;
        *static_cast<RECT *>(rectangle) = visibleBounds;
        return S_OK;
    }
    void UseFrameOperations() {
        SNAP_FRAME_TEST_OPS operations = {FrameSetContext, FrameGetRect, FrameGetDwm};
        SnapFrame_TestSetOps(&operations);
    }
}
TEST_CLASS(FrameUnitTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        SnapFrame_TestSetOps(nullptr);
        ResetEngine();
        logicalBounds = {0,0,100,100};
        physicalBounds = {0,0,200,200};
        visibleBounds = {8,0,192,192};
        physicalContext = failContext = failPhysicalRect = failDwm = failRestore = false;
        contextCalls = physicalRectCalls = dwmCalls = 0;
    }
    TEST_METHOD_CLEANUP(Cleanup) {
        SnapFrame_TestSetOps(nullptr);
        SnapFrame_SetCoordinateWindow(nullptr);
    }

    TEST_METHOD(SpacingStillUsesPhysicalPixelsWhenDwmBoundsAreUnavailable) {
        UseFrameOperations(); failDwm = true; setWindowSpacing(8);
        SnapFrame_SetCoordinateWindow(fixtureWindow);
        Assert::AreEqual(4,SnapFrame_GetSpacing(SIDE_LEFT));
        Assert::AreEqual(4,SnapFrame_GetSpacing(SIDE_TOP));
        Assert::IsFalse(physicalContext);
    }
    TEST_METHOD(PhysicalQueryFailureRestoresCallerContext) {
        UseFrameOperations(); failPhysicalRect = true;
        RECT offsets = {9,9,9,9};
        Assert::IsFalse(SnapFrame_GetInsets(fixtureWindow,&logicalBounds,&offsets) != FALSE);
        AssertRect({0,0,0,0},offsets);
        Assert::IsFalse(physicalContext);
        Assert::AreEqual(2,contextCalls);
        Assert::AreEqual(0,dwmCalls);
    }
    TEST_METHOD(DwmQueryFailureRestoresCallerContext) {
        UseFrameOperations(); failDwm = true;
        RECT offsets = {9,9,9,9};
        Assert::IsFalse(SnapFrame_GetInsets(fixtureWindow,&logicalBounds,&offsets) != FALSE);
        AssertRect({0,0,0,0},offsets);
        Assert::IsFalse(physicalContext);
        Assert::AreEqual(2,contextCalls);
    }
    TEST_METHOD(FailedContextSwitchDoesNotIssuePhysicalQueries) {
        UseFrameOperations(); failContext = true;
        RECT offsets = {9,9,9,9};
        Assert::IsFalse(SnapFrame_GetInsets(fixtureWindow,&logicalBounds,&offsets) != FALSE);
        AssertRect({0,0,0,0},offsets);
        Assert::IsFalse(physicalContext);
        Assert::AreEqual(1,contextCalls);
        Assert::AreEqual(0,physicalRectCalls);
        Assert::AreEqual(0,dwmCalls);
    }
    TEST_METHOD(FailedContextRestorationDoesNotReportSuccessfulInsets) {
        UseFrameOperations(); failRestore = true;
        RECT offsets = {9,9,9,9};
        Assert::IsFalse(SnapFrame_GetInsets(fixtureWindow,&logicalBounds,&offsets) != FALSE);
        AssertRect({0,0,0,0},offsets);
        Assert::AreEqual(2,contextCalls);
    }
    TEST_METHOD(AsymmetricScaleUsesCorrectAxisForSpacing) {
        UseFrameOperations();
        logicalBounds = {-300,-200,-200,0};
        physicalBounds = {-600,-600,-400,0};
        visibleBounds = {-586,-579,-418,-27};
        setWindowSpacing(12);
        SnapFrame_SetCoordinateWindow(fixtureWindow);
        Assert::AreEqual(6,SnapFrame_GetSpacing(SIDE_LEFT));
        Assert::AreEqual(6,SnapFrame_GetSpacing(SIDE_RIGHT));
        Assert::AreEqual(4,SnapFrame_GetSpacing(SIDE_TOP));
        Assert::AreEqual(4,SnapFrame_GetSpacing(SIDE_BOTTOM));
        Assert::IsFalse(physicalContext);
    }
    TEST_METHOD(FractionalSpacingRoundsToNearestCallerCoordinate) {
        UseFrameOperations(); physicalBounds = {0,0,150,150};
        visibleBounds = {8,0,142,142}; setWindowSpacing(8);
        SnapFrame_SetCoordinateWindow(fixtureWindow);
        Assert::AreEqual(5,SnapFrame_GetSpacing(SIDE_LEFT));
        setWindowSpacing(3);
        Assert::AreEqual(2,SnapFrame_GetSpacing(SIDE_TOP));
    }
    TEST_METHOD(AsymmetricInsetsDoNotScaleAbsoluteOrigins) {
        RECT logical = {-300,-200,-200,0}, physical = {-600,-600,-400,0};
        RECT visible = {-586,-579,-418,-27}, offsets = {};
        Assert::IsTrue(SnapFrame_CalculateInsets(&logical,&physical,&visible,&offsets) != FALSE);
        AssertRect({7,7,-9,-9},offsets);
    }
    TEST_METHOD(NativeFailedWindowQueryRestoresThreadDpiContext) {
        typedef HANDLE (WINAPI *GET_CONTEXT)(void);
        typedef BOOL (WINAPI *EQUAL_CONTEXTS)(HANDLE,HANDLE);
        HMODULE user32 = GetModuleHandle(L"user32.dll");
        auto getContext = reinterpret_cast<GET_CONTEXT>(GetProcAddress(user32,"GetThreadDpiAwarenessContext"));
        auto equalContexts = reinterpret_cast<EQUAL_CONTEXTS>(GetProcAddress(user32,"AreDpiAwarenessContextsEqual"));
        Assert::IsTrue(getContext != nullptr && equalContexts != nullptr);
        HANDLE before = getContext();
        RECT offsets = {};
        Assert::IsFalse(SnapFrame_GetInsets(nullptr,&logicalBounds,&offsets) != FALSE);
        Assert::IsTrue(equalContexts(before,getContext()) != FALSE);
    }
    TEST_METHOD(VisibleInsetsHaveSignedRightAndBottomOffsets) {
        RECT logical = {100,100,300,300}, physical = logical;
        RECT visible = {108,100,292,292}, offsets = {};
        Assert::IsTrue(SnapFrame_CalculateInsets(&logical,&physical,&visible,&offsets) != FALSE);
        AssertRect({8,0,-8,-8},offsets);
    }
    TEST_METHOD(PhysicalInsetsAreScaledToCallerCoordinates) {
        RECT logical = {-150,50,-50,150}, physical = {-300,100,-100,300};
        RECT visible = {-292,100,-108,292}, offsets = {};
        Assert::IsTrue(SnapFrame_CalculateInsets(&logical,&physical,&visible,&offsets) != FALSE);
        AssertRect({4,0,-4,-4},offsets);
    }
    TEST_METHOD(InvalidFrameBoundsDoNotCreateOffsets) {
        RECT outer = {0,0,100,100}, visible = {-5,0,100,100}, offsets = {9,9,9,9};
        Assert::IsFalse(SnapFrame_CalculateInsets(&outer,&outer,&visible,&offsets) != FALSE);
        AssertRect({0,0,0,0},offsets);
    }
    TEST_METHOD(SpacingHasAnExplicitSafeRange) {
        setWindowSpacing(-1); Assert::AreEqual(0,getWindowSpacing());
        setWindowSpacing(1001); Assert::AreEqual(1000,getWindowSpacing());
        setWindowSpacing(8); Assert::AreEqual(8,getWindowSpacing());
    }
    TEST_METHOD(LargeSpacingCannotInvertASmallWorkArea) {
        setWindowSpacing(1000);
        RECT work = {0,0,100,80}, inset = {};
        SnapFrame_InsetWorkArea(&work,&inset);
        Assert::IsTrue(inset.right > inset.left && inset.bottom > inset.top);
    }
};
}
