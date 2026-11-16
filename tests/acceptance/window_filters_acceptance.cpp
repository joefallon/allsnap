#include "../support/EngineFixture.h"
#include "snap_lifecycle_test.h"
#include <string>
#include <climits>
extern "C" {
#include "snap_Crop.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
namespace {
    const WCHAR longClass[] = L"AllSnapFilterLongClassNameBeyondLegacyLimit";
    const WCHAR shortClass[] = L"AsFilterShort";
    void RegisterFilterClass(LPCWSTR name, WNDPROC procedure = DefWindowProcW) {
        WNDCLASSW type = {};
        HINSTANCE instance = GetModuleHandleW(nullptr);
        if (GetClassInfoW(instance, name, &type)) return;
        type.lpfnWndProc = procedure; type.hInstance = instance; type.lpszClassName = name;
        Assert::IsTrue(RegisterClassW(&type) != 0);
    }
    class FilterWindow {
    public:
        HWND handle;
        FilterWindow(LPCWSTR name = shortClass) {
            RegisterFilterClass(name);
            handle = CreateWindowExW(WS_EX_TOOLWINDOW, name, L"allSnap filter fixture", WS_POPUP,
                -29000, -29000, 300, 220, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            Assert::IsNotNull(handle);
        }
        ~FilterWindow() { if (IsWindow(handle)) DestroyWindow(handle); }
        RECT Bounds() const { RECT result = {}; Assert::IsTrue(GetWindowRect(handle, &result) != FALSE); return result; }
    };
    class MdiFixture {
    public:
        HWND frame = nullptr, client = nullptr, active = nullptr, target = nullptr;
        MdiFixture() {
            // A distinct native MDI class avoids reusing the top-level class procedure.
            RegisterFilterClass(L"AsFilterMdi", DefMDIChildProcW);
            frame = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"allSnap MDI fixture",
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, -30000, -30000, 1000, 800,
                nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            Assert::IsNotNull(frame);
            CLIENTCREATESTRUCT configuration = {}; configuration.idFirstChild = 100;
            client = CreateWindowExW(0, L"MDICLIENT", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                0, 0, 900, 700, frame, nullptr, GetModuleHandleW(nullptr), &configuration);
            Assert::IsNotNull(client);
            active = CreateMDIWindowW(L"AsFilterMdi", L"active", WS_VISIBLE, 40, 50, 220, 180,
                client, GetModuleHandleW(nullptr), 0);
            target = CreateMDIWindowW(L"AsFilterMdi", L"target", WS_VISIBLE, 350, 250, 240, 190,
                client, GetModuleHandleW(nullptr), 0);
            Assert::IsNotNull(active); Assert::IsNotNull(target);
            ShowWindow(frame, SW_SHOWNOACTIVATE);
        }
        ~MdiFixture() { if (IsWindow(frame)) DestroyWindow(frame); }
        void Collect() {
            const RECT area = {-32000, -32000, -27000, -27000};
            setSnapType(SNAPT_OTHERS);
            WinRects_TestRefreshWindows(SNAPT_OTHERS, active, nullptr, 0, &area, 1);
        }
    };
    int CandidateCount() { TEST_RECTS candidates = {}; WinRects_getRects(&candidates); return candidates.num_win_rects; }
    void CollectTarget(HWND target) {
        const RECT area = {-32000, -32000, -27000, -27000};
        setSnapType(SNAPT_OTHERS);
        WinRects_TestRefreshWindows(SNAPT_OTHERS, nullptr, &target, 1, &area, 1);
    }
    void SetIgnored(const std::wstring & value) {
        setIgnoredClasses(const_cast<WCHAR *>(value.c_str()), static_cast<int>(value.size()));
    }
    void SetSkinned(const std::wstring & value) {
        setSkinnedClasses(const_cast<WCHAR *>(value.c_str()), static_cast<int>(value.size()));
    }
}

TEST_CLASS(WindowFiltersAcceptanceTests) {
    std::wstring ignored, skinned;
    int cropTop;
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        ignored = getIgnoredClasses(); skinned = getSkinnedClasses(); cropTop = getCropTop();
        SnapTest_LifecycleReset(); SnapFrame_TestSetOps(nullptr); ResetEngine();
        SetIgnored(L""); SetSkinned(L"");
    }
    TEST_METHOD_CLEANUP(Cleanup) {
        SnapTest_LifecycleReset(); SnapFrame_TestSetOps(nullptr); ResetEngine();
        SetIgnored(ignored); SetSkinned(skinned); setCropTop(cropTop);
    }
    TEST_METHOD(LongIgnoredClassIsExcludedFromActualWindowCollection) {
        FilterWindow target(longClass); ShowWindow(target.handle, SW_SHOWNOACTIVATE);
        SetIgnored(longClass); CollectTarget(target.handle);
        Assert::AreEqual(0, CandidateCount());
    }
    TEST_METHOD(IgnoreListDoesNotTreatTruncatedClassPrefixAsAnExactMatch) {
        FilterWindow target(longClass); ShowWindow(target.handle, SW_SHOWNOACTIVATE);
        SetIgnored(std::wstring(longClass).substr(0, 19)); CollectTarget(target.handle);
        Assert::AreEqual(1, CandidateCount());
    }
    TEST_METHOD(ShortIgnoredClassIsExcludedAndClearingTheListRestoresIt) {
        FilterWindow target; ShowWindow(target.handle, SW_SHOWNOACTIVATE);
        SetIgnored(shortClass); CollectTarget(target.handle); Assert::AreEqual(0, CandidateCount());
        SetIgnored(L""); CollectTarget(target.handle); Assert::AreEqual(1, CandidateCount());
    }
    TEST_METHOD(LongSkinnedClassSuppressesManualTopCrop) {
        FilterWindow target(longClass); setCroppingTop(TRUE); setCropTop(30); SetSkinned(longClass);
        RECT bounds = target.Bounds(); CROP_INFO crop = {};
        Crop_LoadMovingCropInfo(&crop, target.handle, &bounds);
        Assert::AreEqual(bounds.top, crop.cropped_rect.top);
        SetSkinned(L""); Crop_LoadMovingCropInfo(&crop, target.handle, &bounds);
        Assert::AreEqual(bounds.top + 30, crop.cropped_rect.top);
    }
    TEST_METHOD(SkinnedClassStillUsesItsExplicitClippingRegion) {
        FilterWindow target(longClass); setCroppingTop(TRUE); setCropTop(30);
        setCroppingRgn(TRUE); SetSkinned(longClass);
        RECT bounds = target.Bounds();
        HRGN region = CreateRectRgn(5, 9, rectWidth(bounds) - 7, rectHeight(bounds) - 11);
        Assert::IsNotNull(region);
        if (!SetWindowRgn(target.handle, region, FALSE)) {
            DeleteObject(region); Assert::Fail(L"Cannot assign owned clipping region.");
        }
        CROP_INFO crop = {}; Crop_LoadMovingCropInfo(&crop, target.handle, &bounds);
        AssertRect({bounds.left + 5, bounds.top + 9, bounds.right - 7, bounds.bottom - 11}, crop.cropped_rect);
    }
    TEST_METHOD(IgnoredMdiSiblingIsNotASnapDestination) {
        MdiFixture mdi; SetIgnored(L"AsFilterMdi"); mdi.Collect(); Assert::AreEqual(0, CandidateCount());
    }
    TEST_METHOD(MinimizedMdiSiblingIsNotASnapDestination) {
        MdiFixture mdi; ShowWindow(mdi.target, SW_SHOWMINNOACTIVE); mdi.Collect();
        Assert::IsTrue(IsIconic(mdi.target) != FALSE); Assert::AreEqual(0, CandidateCount());
    }
    TEST_METHOD(HiddenMdiSiblingIsNotASnapDestination) {
        MdiFixture mdi; ShowWindow(mdi.target, SW_HIDE); mdi.Collect();
        Assert::AreEqual(0, CandidateCount());
    }
    TEST_METHOD(MdiTargetsUseTheSameVisibleManualCropAsOrdinaryWindows) {
        MdiFixture mdi; setCroppingTop(TRUE); setCropTop(30); mdi.Collect();
        Assert::AreEqual(1, CandidateCount());
        RECT expected = {}; Assert::IsTrue(WinRects_GetWindowRect(mdi.target, &expected) != FALSE);
        TEST_RECTS candidates = {}; WinRects_getRects(&candidates);
        AssertRect(expected, candidates.win_rects[0]);
    }
    TEST_METHOD(MdiMatchingUsesTheConfiguredVisibleTopEdge) {
        MdiFixture mdi; setCroppingTop(TRUE); setCropTop(30); mdi.Collect();
        RECT target = {}; Assert::IsTrue(GetWindowRect(mdi.target, &target) != FALSE);
        // A separate 100px window approaches the independently specified manual edge.
        // The raw target top is 23px away, outside the10px activation threshold.
        RECT proposed = {target.left + 40, target.top - 77, target.left + 140, target.top + 23};
        RECT expected = proposed; OffsetRect(&expected, 0, 7);
        AssertRect(expected, Move(proposed));
    }
    TEST_METHOD(MdiClientWorkAreaUsesDesktopCoordinates) {
        MdiFixture mdi; mdi.Collect();
        RECT expected = {}; GetClientRect(mdi.client, &expected);
        MapWindowPoints(mdi.client, nullptr, reinterpret_cast<POINT *>(&expected), 2);
        Assert::AreEqual(1, WinRects_GetNumScreens()); AssertRect(expected, WinRects_GetScreens()[0]);
    }
    TEST_METHOD(MdiOptionControlsNativeHookAdmission) {
        MdiFixture mdi; CWPSTRUCT message = {}; message.hwnd = mdi.active; message.message = WM_ENTERSIZEMOVE;
        setSnapMdi(FALSE); SnapTest_CallHook(HC_ACTION, 0, reinterpret_cast<LPARAM>(&message));
        Assert::IsTrue(SnapTest_SubclassWindow() == nullptr);
        setSnapMdi(TRUE); SnapTest_CallHook(HC_ACTION, 0, reinterpret_cast<LPARAM>(&message));
        Assert::IsTrue(SnapTest_SubclassWindow() == mdi.active);
        Assert::IsTrue(SnapTest_UnSubclass(mdi.active) != FALSE);
    }
    TEST_METHOD(IgnoredClassesRemainEligibleForTheirOwnWindowGestures) {
        FilterWindow active; SetIgnored(shortClass);
        CWPSTRUCT message = {}; message.hwnd = active.handle; message.message = WM_ENTERSIZEMOVE;
        SnapTest_CallHook(HC_ACTION, 0, reinterpret_cast<LPARAM>(&message));
        Assert::IsTrue(SnapTest_SubclassWindow() == active.handle);
        Assert::IsTrue(SnapTest_UnSubclass(active.handle) != FALSE);
    }
    TEST_METHOD(ExcessiveManualCropRetainsANonemptyVisibleWindow) {
        FilterWindow target; setCroppingTop(TRUE); setCropTop(INT_MAX);
        RECT bounds = target.Bounds(); CROP_INFO crop = {};
        Crop_LoadMovingCropInfo(&crop, target.handle, &bounds);
        Assert::AreEqual(bounds.bottom - 1, crop.cropped_rect.top);
        Assert::IsTrue(IsRectEmpty(&crop.cropped_rect) == FALSE);
        Crop_LoadSizingInfo(target.handle); RECT sizing = {};
        Crop_CropSizingRect(&bounds, SIDE_TOP, SIDE_NONE, &sizing);
        Assert::AreEqual(bounds.bottom - 1, sizing.top);
        Assert::AreEqual(INT_MAX, getCropTop(), L"The clamp is per window; the stored setting remains intact.");
    }
};
}
