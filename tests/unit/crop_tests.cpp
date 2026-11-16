#include "../support/EngineFixture.h"

extern "C" {
#include "snap_Crop.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    class RegionWindow
    {
    public:
        HWND handle;
        RECT bounds;

        RegionWindow()
            : handle(CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"allSnap crop test",
                WS_POPUP, -30000, -30000, 300, 200, nullptr, nullptr, GetModuleHandle(nullptr), nullptr)), bounds{}
        {
            Assert::IsTrue(handle != nullptr, L"Create only a hidden test-owned window");
            Assert::IsTrue(GetWindowRect(handle, &bounds) != FALSE);
            HRGN region = CreateRectRgn(6, 9, bounds.right - bounds.left - 8, bounds.bottom - bounds.top - 11);
            Assert::IsTrue(region != nullptr);
            if (!SetWindowRgn(handle, region, FALSE))
            {
                DeleteObject(region);
                Assert::Fail(L"Cannot assign test-owned window region");
            }
        }

        ~RegionWindow() { if (handle) DestroyWindow(handle); }
        RegionWindow(const RegionWindow &) = delete;
        RegionWindow & operator=(const RegionWindow &) = delete;
    };

    TEST_CLASS(CropUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); setCroppingRgn(TRUE); }
        TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

        TEST_METHOD(MovingBoundsExcludeEveryRegionInset)
        {
            RegionWindow window;
            CROP_INFO crop = {};
            Crop_LoadMovingCropInfo(&crop, window.handle, &window.bounds);
            RECT expected = {window.bounds.left + 6, window.bounds.top + 9,
                window.bounds.right - 8, window.bounds.bottom - 11};
            AssertRect(expected, crop.cropped_rect);
        }

        TEST_METHOD(RightAndBottomSizingBoundsMoveInward)
        {
            RegionWindow window;
            Crop_LoadSizingInfo(window.handle);
            RECT actual = {};
            Crop_CropSizingRect(&window.bounds, SIDE_BOTTOM, SIDE_RIGHT, &actual);
            RECT expected = {window.bounds.left, window.bounds.top,
                window.bounds.right - 8, window.bounds.bottom - 11};
            AssertRect(expected, actual);
        }

        TEST_METHOD(LeftAndTopSizingBoundsMoveInward)
        {
            RegionWindow window;
            Crop_LoadSizingInfo(window.handle);
            RECT actual = {};
            Crop_CropSizingRect(&window.bounds, SIDE_TOP, SIDE_LEFT, &actual);
            RECT expected = {window.bounds.left + 6, window.bounds.top + 9,
                window.bounds.right, window.bounds.bottom};
            AssertRect(expected, actual);
        }

        TEST_METHOD(RightAndBottomSnapResultsRestoreOuterInsets)
        {
            RegionWindow window;
            Crop_LoadSizingInfo(window.handle);
            SNAP_RESULTS results = {};
            clear_snap_results(&results);
            results.h.side = SIDE_RIGHT;
            results.h.value = 500;
            results.v.side = SIDE_BOTTOM;
            results.v.value = 600;
            Crop_UnCropSizingResults(&results);
            Assert::AreEqual(508, results.h.value);
            Assert::AreEqual(611, results.v.value);
        }

        TEST_METHOD(LeftAndTopSnapResultsRestoreOuterInsets)
        {
            RegionWindow window;
            Crop_LoadSizingInfo(window.handle);
            SNAP_RESULTS results = {};
            clear_snap_results(&results);
            results.h.side = SIDE_LEFT;
            results.h.value = 500;
            results.v.side = SIDE_TOP;
            results.v.value = 600;
            Crop_UnCropSizingResults(&results);
            Assert::AreEqual(494, results.h.value);
            Assert::AreEqual(591, results.v.value);
        }

        TEST_METHOD(DisablingRegionCroppingKeepsSizingBounds)
        {
            RegionWindow window;
            setCroppingRgn(FALSE);
            Crop_LoadSizingInfo(window.handle);
            RECT actual = {};
            Crop_CropSizingRect(&window.bounds, SIDE_BOTTOM, SIDE_RIGHT, &actual);
            AssertRect(window.bounds, actual);
        }
    };
}
