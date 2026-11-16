#include "../support/EngineFixture.h"
#include <dwmapi.h>
extern "C" {
#include "snap_Crop.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
class FramedWindow {
public:
    HWND handle;
    RECT outer = {}, insets = {};
    FramedWindow(int x = -28000, int y = -28000) : handle(CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",L"allSnap frame test",
        WS_OVERLAPPEDWINDOW,x,y,400,300,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)) {
        Assert::IsNotNull(handle);
        ShowWindow(handle,SW_SHOWNOACTIVATE);
        DwmFlush();
        Assert::IsTrue(GetWindowRect(handle,&outer) != FALSE);
    }
    bool ReadInsets() { return SnapFrame_GetInsets(handle,&outer,&insets) != FALSE; }
    ~FramedWindow() { if (handle) DestroyWindow(handle); }
};

// Keep native integration assertions in physical pixels regardless of the test host's default.
class PhysicalDpiCaller {
    typedef HANDLE (WINAPI *SET_CONTEXT)(HANDLE);
    SET_CONTEXT setContext;
    HANDLE previous;
public:
    PhysicalDpiCaller() : setContext(reinterpret_cast<SET_CONTEXT>(GetProcAddress(
        GetModuleHandleW(L"user32.dll"),"SetThreadDpiAwarenessContext"))), previous(nullptr) {
        Assert::IsTrue(setContext != nullptr,L"Physical-frame integration requires modern Windows DPI APIs.");
        previous = setContext(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)));
        Assert::IsTrue(previous != nullptr,L"Cannot establish the physical-coordinate test caller.");
    }
    ~PhysicalDpiCaller() { if (previous) setContext(previous); }
};

static void ApplyOwnedRectangle(HWND window, const RECT & rectangle) {
    Assert::IsTrue(SetWindowPos(window,nullptr,rectangle.left,rectangle.top,
        rectWidth(rectangle),rectHeight(rectangle),SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER) != FALSE);
    Assert::IsTrue(SUCCEEDED(DwmFlush()));
}

static RECT ReadPhysicalVisibleBounds(HWND window) {
    RECT visible = {};
    Assert::IsTrue(SUCCEEDED(DwmGetWindowAttribute(window,DWMWA_EXTENDED_FRAME_BOUNDS,
        &visible,sizeof(visible))),L"Native frame tests require active desktop composition.");
    return visible;
}
TEST_CLASS(VisibleFrameAcceptanceTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) { SnapFrame_TestSetOps(nullptr); ResetEngine(); }
    TEST_METHOD_CLEANUP(Cleanup) { SnapFrame_TestSetOps(nullptr); ResetEngine(); }

    // VF-004: native DWM bounds, both windows' crop paths, matching, and outer conversion agree.
    TEST_METHOD(TwoNativeVisibleFramesUseOneCollapsedMovingGap) {
        PhysicalDpiCaller caller;
        FramedWindow moving(-28000,-28000), target(-26000,-28000);
        for (int spacing : {0,8,24}) {
            setWindowSpacing(spacing);
            ApplyOwnedRectangle(moving.handle,{-28000,-28000,-27600,-27700});
            Assert::IsTrue(GetWindowRect(moving.handle,&moving.outer) != FALSE);
            Assert::IsTrue(moving.ReadInsets());
            SnapFrame_SetCoordinateWindow(moving.handle);
            RECT candidate = {};
            Assert::IsTrue(WinRects_GetWindowRect(target.handle,&candidate) != FALSE);
            SetWindows({candidate}); setSnapType(SNAPT_OTHERS);
            RECT proposed = moving.outer;
            OffsetRect(&proposed,candidate.left - SnapFrame_GetSpacing(SIDE_RIGHT) - 3
                - (proposed.right + moving.insets.right),0);
            CROP_INFO crop = {};
            Crop_LoadMovingCropInfo(&crop,moving.handle,&proposed);
            SNAP_RESULTS results = EvaluateMove(crop.cropped_rect);
            Assert::IsTrue(results.h.side == SIDE_RIGHT,L"The real matcher must select the adjacent edge.");
            Crop_UnCropMovingResults(&crop,&results);
            RECT actual = proposed;
            setMovingSnapped(FALSE);
            reposition(&proposed,&results,&actual);
            ApplyOwnedRectangle(moving.handle,actual);
            RECT actualVisible = ReadPhysicalVisibleBounds(moving.handle);
            RECT targetVisible = ReadPhysicalVisibleBounds(target.handle);
            Assert::AreEqual(static_cast<LONG>(spacing),targetVisible.left - actualVisible.right,
                L"Configured spacing is one physical gap, without a second margin or hidden border.");
            Assert::AreEqual(rectWidth(proposed),rectWidth(actual));
        }
    }

    // VF-005: the same native pipeline preserves the anchored edge during right resizing.
    TEST_METHOD(TwoNativeVisibleFramesUseOneCollapsedResizingGap) {
        PhysicalDpiCaller caller;
        FramedWindow moving(-28000,-28000), target(-26000,-28000);
        for (int spacing : {0,8,24}) {
            setWindowSpacing(spacing);
            RECT candidate = {};
            Assert::IsTrue(WinRects_GetWindowRect(target.handle,&candidate) != FALSE);
            RECT initial = {candidate.left - 500,-28000,candidate.left - 100,-27700};
            ApplyOwnedRectangle(moving.handle,initial);
            Assert::IsTrue(GetWindowRect(moving.handle,&moving.outer) != FALSE);
            Assert::IsTrue(moving.ReadInsets());
            SnapFrame_SetCoordinateWindow(moving.handle);
            SetWindows({candidate}); setSnapType(SNAPT_OTHERS);
            Crop_LoadSizingInfo(moving.handle);
            RECT proposed = moving.outer;
            proposed.right = candidate.left - SnapFrame_GetSpacing(SIDE_RIGHT) - 3 - moving.insets.right;
            RECT cropped = {};
            Crop_CropSizingRect(&proposed,SIDE_NONE,SIDE_RIGHT,&cropped);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            SNAP_RESULTS results = {};
            sizing_test_all(SIDE_NONE,SIDE_RIGHT,&cropped,getWinThresh(),getScreenThresh(),
                &candidates,&results,FALSE);
            Assert::IsTrue(results.h.side == SIDE_RIGHT,L"The real sizing matcher must select the adjacent edge.");
            Crop_UnCropSizingResults(&results);
            RECT actual = proposed;
            resize(&results,&actual);
            Assert::AreEqual(proposed.left,actual.left,L"Right resizing must preserve its anchored left edge.");
            ApplyOwnedRectangle(moving.handle,actual);
            RECT actualVisible = ReadPhysicalVisibleBounds(moving.handle);
            RECT targetVisible = ReadPhysicalVisibleBounds(target.handle);
            Assert::AreEqual(static_cast<LONG>(spacing),targetVisible.left - actualVisible.right,
                L"Moving and resizing must produce the same single physical gap.");
        }
    }
    // VF-001: constrain the visible frame, including the DWM resize-border conversion.
    TEST_METHOD(ConfinementPlacesVisibleEdgeAtTheConfiguredDesktopMargin) {
        FramedWindow window;
        Assert::IsTrue(window.ReadInsets(),L"DWM frame bounds unavailable: this integration test requires an active Windows desktop.");
        SnapFrame_SetCoordinateWindow(window.handle);
        RECT area = {-30000,-30000,-26000,-26000};
        SetScreens({area}); setKeptToScreen(TRUE); setWindowSpacing(8);
        RECT proposed = window.outer;
        OffsetRect(&proposed,area.left - 50 - proposed.left,0);
        CROP_INFO crop = {};
        Crop_LoadMovingCropInfo(&crop,window.handle,&proposed);
        SNAP_RESULTS results = EvaluateMove(crop.cropped_rect);
        Crop_UnCropMovingResults(&crop,&results);
        RECT actual = proposed;
        reposition(&proposed,&results,&actual);
        Assert::AreEqual(area.left + SnapFrame_GetSpacing(SIDE_LEFT),actual.left + window.insets.left);
        Assert::AreEqual(rectWidth(proposed),rectWidth(actual));
    }

    // VF-002: moving and resizing agree about which frame border is visible.
    TEST_METHOD(ResizingUsesTheSameDwmVisibleBoundsAsMoving) {
        FramedWindow window;
        Assert::IsTrue(window.ReadInsets(),L"DWM frame bounds unavailable: this integration test requires an active Windows desktop.");
        CROP_INFO moving = {};
        Crop_LoadMovingCropInfo(&moving,window.handle,&window.outer);
        Crop_LoadSizingInfo(window.handle);
        RECT sizing;
        Crop_CropSizingRect(&window.outer,SIDE_BOTTOM,SIDE_RIGHT,&sizing);
        Assert::AreEqual(moving.cropped_rect.right,sizing.right);
        Assert::AreEqual(moving.cropped_rect.bottom,sizing.bottom);
        Assert::AreEqual(window.outer.right + window.insets.right,sizing.right);
        Assert::AreEqual(window.outer.bottom + window.insets.bottom,sizing.bottom);
    }
};
}
