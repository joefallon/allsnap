#include "../support/EngineFixture.h"
extern "C" {
#include "snap_Crop.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests {
namespace {
    // These functions describe an eight-pixel visible frame on a valid native window.
    // The synthetic API boundary does not replace production crop or uncrop calculations.
    const HANDLE syntheticCaller = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(19));
    const HANDLE syntheticPhysical = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));
    HANDLE WINAPI SyntheticContext(HANDLE context) {
        return context == syntheticPhysical ? syntheticCaller : syntheticPhysical;
    }
    HRESULT WINAPI SyntheticDwmFrame(HWND window, DWORD, PVOID output, DWORD) {
        RECT outer = {};
        if (!GetWindowRect(window,&outer)) return E_FAIL;
        InflateRect(&outer,-8,-8);
        *static_cast<RECT *>(output) = outer;
        return S_OK;
    }

    class CropFrameWindow {
    public:
        HWND handle;
        RECT outer = {};
        CropFrameWindow() : handle(CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",L"allSnap combined crop test",
            WS_POPUP,-30000,-30000,300,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)) {
            Assert::IsNotNull(handle);
            Assert::IsTrue(GetWindowRect(handle,&outer) != FALSE);
        }
        ~CropFrameWindow() { if (handle) DestroyWindow(handle); }
        void SetRegionInsets(int left, int top, int right, int bottom) {
            HRGN region = CreateRectRgn(left,top,rectWidth(outer)-right,rectHeight(outer)-bottom);
            Assert::IsNotNull(region);
            if (!SetWindowRgn(handle,region,FALSE)) {
                DeleteObject(region);
                Assert::Fail(L"Cannot assign the test-owned region.");
            }
        }
    };
}

TEST_CLASS(CropFrameUnitTests) {
    int originalCropTop = 3;
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        SnapFrame_TestSetOps(nullptr);
        originalCropTop = getCropTop();
        ResetEngine();
        SNAP_FRAME_TEST_OPS operations = {SyntheticContext,nullptr,SyntheticDwmFrame};
        SnapFrame_TestSetOps(&operations);
        setCroppingTop(TRUE);
        setCropTop(3);
    }
    TEST_METHOD_CLEANUP(Cleanup) {
        SnapFrame_TestSetOps(nullptr);
        setCropTop(originalCropTop);
        ResetEngine();
    }

    // VF-006: manual crop is an absolute minimum, not extra cropping after DWM.
    TEST_METHOD(ManualCropDoesNotAddToAnExistingDwmTopInset) {
        CropFrameWindow window;
        CROP_INFO moving = {};
        Crop_LoadMovingCropInfo(&moving,window.handle,&window.outer);
        Assert::AreEqual(8L,moving.cropped_rect.top-window.outer.top);

        Crop_LoadSizingInfo(window.handle);
        RECT sizing = {};
        Crop_CropSizingRect(&window.outer,SIDE_TOP,SIDE_NONE,&sizing);
        Assert::AreEqual(8L,sizing.top-window.outer.top);

        SNAP_RESULTS movingResult = {}, sizingResult = {};
        clear_snap_results(&movingResult);
        movingResult.v.side = SIDE_TOP;
        movingResult.v.value = 500;
        sizingResult = movingResult;
        Crop_UnCropMovingResults(&moving,&movingResult);
        Crop_UnCropSizingResults(&sizingResult);
        Assert::AreEqual(492,movingResult.v.value);
        Assert::AreEqual(492,sizingResult.v.value);
    }

    TEST_METHOD(LargerManualMinimumRemainsAbsoluteForMoveAndResize) {
        CropFrameWindow window;
        setCropTop(12);
        CROP_INFO moving = {};
        Crop_LoadMovingCropInfo(&moving,window.handle,&window.outer);
        Assert::AreEqual(12L,moving.cropped_rect.top-window.outer.top);
        Crop_LoadSizingInfo(window.handle);
        RECT sizing = {};
        Crop_CropSizingRect(&window.outer,SIDE_TOP,SIDE_NONE,&sizing);
        Assert::AreEqual(12L,sizing.top-window.outer.top);

        SNAP_RESULTS results = {};
        clear_snap_results(&results);
        results.v.side = SIDE_TOP;
        results.v.value = 500;
        Crop_UnCropSizingResults(&results);
        Assert::AreEqual(488,results.v.value);
    }

    // VF-007: explicit regions and visible-frame bounds intersect rather than add.
    TEST_METHOD(DwmFrameWinsOverSmallerRegionInsets) {
        CropFrameWindow window;
        window.SetRegionInsets(2,3,4,5);
        setCroppingRgn(TRUE);
        CROP_INFO moving = {};
        Crop_LoadMovingCropInfo(&moving,window.handle,&window.outer);
        AssertRect({window.outer.left+8,window.outer.top+8,window.outer.right-8,window.outer.bottom-8},
            moving.cropped_rect);

        Crop_LoadSizingInfo(window.handle);
        RECT sizing = {};
        Crop_CropSizingRect(&window.outer,SIDE_BOTTOM,SIDE_RIGHT,&sizing);
        AssertRect({window.outer.left,window.outer.top,window.outer.right-8,window.outer.bottom-8},sizing);
        SNAP_RESULTS results = {};
        clear_snap_results(&results);
        results.h.side = SIDE_RIGHT; results.h.value = 500;
        results.v.side = SIDE_BOTTOM; results.v.value = 600;
        Crop_UnCropSizingResults(&results);
        Assert::AreEqual(508,results.h.value);
        Assert::AreEqual(608,results.v.value);
    }

    TEST_METHOD(LargerRegionWinsOverTheDwmFrame) {
        CropFrameWindow window;
        window.SetRegionInsets(12,11,14,13);
        setCroppingRgn(TRUE);
        CROP_INFO moving = {};
        Crop_LoadMovingCropInfo(&moving,window.handle,&window.outer);
        AssertRect({window.outer.left+12,window.outer.top+11,window.outer.right-14,window.outer.bottom-13},
            moving.cropped_rect);
        Crop_LoadSizingInfo(window.handle);
        RECT sizing = {};
        Crop_CropSizingRect(&window.outer,SIDE_TOP,SIDE_LEFT,&sizing);
        AssertRect({window.outer.left+12,window.outer.top+11,window.outer.right,window.outer.bottom},sizing);
        SNAP_RESULTS results = {};
        clear_snap_results(&results);
        results.h.side = SIDE_LEFT; results.h.value = 500;
        results.v.side = SIDE_TOP; results.v.value = 600;
        Crop_UnCropMovingResults(&moving,&results);
        Assert::AreEqual(488,results.h.value);
        Assert::AreEqual(589,results.v.value);
    }
};
}
