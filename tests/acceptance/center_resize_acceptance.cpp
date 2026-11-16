#include "../support/EngineFixture.h"
#include "snap_keys.h"
#include <dwmapi.h>
extern "C" {
#include "snapper.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        bool centerHeld = true;
        SHORT WINAPI CenterResizeKeys(int key)
        {
            return key == VK_CONTROL && centerHeld ? static_cast<SHORT>(0x8000) : 0;
        }

        class CenteredWindow
        {
        public:
            HWND handle;
            CenteredWindow(DWORD style = WS_POPUP) : handle(CreateWindowExW(WS_EX_TOOLWINDOW,
                L"STATIC", L"allSnap center resize test", style, -28400, -28400, 300, 300,
                nullptr, nullptr, GetModuleHandleW(nullptr), nullptr)) { Assert::IsNotNull(handle); }
            ~CenteredWindow() { if (handle) DestroyWindow(handle); }
        };

        class PhysicalCoordinates
        {
            typedef HANDLE (WINAPI *SET_CONTEXT)(HANDLE);
            SET_CONTEXT setContext = reinterpret_cast<SET_CONTEXT>(GetProcAddress(
                GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
            HANDLE previous = nullptr;
        public:
            PhysicalCoordinates()
            {
                Assert::IsTrue(setContext != nullptr);
                previous = setContext(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)));
                Assert::IsTrue(previous != nullptr);
            }
            ~PhysicalCoordinates() { if (previous) setContext(previous); }
        };

        RECT CenterResize(WPARAM edge, RECT proposed, int spacing,
            const RECT & area = {-28500, -28500, -28000, -28000}, bool snapOther = false)
        {
            PhysicalCoordinates coordinates;
            CenteredWindow window;
            setWindowSpacing(spacing);
            setSnapType(snapOther ? SNAPT_OTHERS : SNAPT_NONE);
            // Initialize cached centers and crop information through the real gesture entry.
            // Fixture work areas are supplied afterwards, without moving any user window.
            snapper_OnEnterSizeMove(window.handle);
            SetScreens({area});
            if (snapOther) SetWindows({{-27995, -28400, -27795, -28100}});
            snapper_OnSizing(window.handle, edge, &proposed);
            Assert::AreEqual(-56500L, proposed.left + proposed.right, L"Original horizontal center");
            Assert::AreEqual(-56500L, proposed.top + proposed.bottom, L"Original vertical center");
            return proposed;
        }

        void NativeCenterResize(WPARAM edge)
        {
            PhysicalCoordinates coordinates;
            CenteredWindow window(WS_OVERLAPPEDWINDOW);
            ShowWindow(window.handle, SW_SHOWNOACTIVATE);
            Assert::IsTrue(SUCCEEDED(DwmFlush()));
            RECT initial = {};
            Assert::IsTrue(GetWindowRect(window.handle, &initial) != FALSE);
            LONG centerX = initial.left + (rectWidth(initial) + 1) / 2;
            LONG centerY = initial.top + (rectHeight(initial) + 1) / 2;
            RECT area = {centerX - 250, centerY - 250, centerX + 250, centerY + 250};
            for (int spacing : {0, 8})
            {
                setWindowSpacing(spacing);
                snapper_OnEnterSizeMove(window.handle);
                SetScreens({area});
                RECT proposed = initial;
                if (edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT) proposed.right += 500;
                if (edge == WMSZ_LEFT) proposed.left -= 500;
                if (edge == WMSZ_TOP) proposed.top -= 500;
                if (edge == WMSZ_BOTTOM || edge == WMSZ_BOTTOMRIGHT) proposed.bottom += 500;
                snapper_OnSizing(window.handle, edge, &proposed);
                Assert::AreEqual(2 * centerX, proposed.left + proposed.right);
                Assert::AreEqual(2 * centerY, proposed.top + proposed.bottom);
                Assert::IsTrue(SetWindowPos(window.handle, nullptr, proposed.left, proposed.top,
                    rectWidth(proposed), rectHeight(proposed), SWP_NOACTIVATE | SWP_NOZORDER) != FALSE);
                Assert::IsTrue(SUCCEEDED(DwmFlush()));
                RECT visible = {};
                Assert::IsTrue(SUCCEEDED(DwmGetWindowAttribute(window.handle,
                    DWMWA_EXTENDED_FRAME_BOUNDS, &visible, sizeof(visible))));
                Assert::IsTrue(visible.left >= area.left + spacing, L"Visible left margin");
                Assert::IsTrue(visible.top >= area.top + spacing, L"Visible top margin");
                Assert::IsTrue(visible.right <= area.right - spacing, L"Visible right margin");
                Assert::IsTrue(visible.bottom <= area.bottom - spacing, L"Visible bottom margin");
                // Restore the original center/size before initializing the next gesture.
                Assert::IsTrue(SetWindowPos(window.handle, nullptr, initial.left, initial.top,
                    rectWidth(initial), rectHeight(initial), SWP_NOACTIVATE | SWP_NOZORDER) != FALSE);
                Assert::IsTrue(SUCCEEDED(DwmFlush()));
            }
        }

        void NativeOrdinaryResize(WPARAM edge)
        {
            PhysicalCoordinates coordinates;
            CenteredWindow window(WS_OVERLAPPEDWINDOW);
            centerHeld = false;
            ShowWindow(window.handle, SW_SHOWNOACTIVATE);
            Assert::IsTrue(SUCCEEDED(DwmFlush()));
            const RECT area = {-28500, -28500, -28000, -28000};
            for (int spacing : {0, 8})
            {
                RECT initial = {}, insets = {};
                Assert::IsTrue(GetWindowRect(window.handle, &initial) != FALSE);
                Assert::IsTrue(SnapFrame_GetInsets(window.handle, &initial, &insets) != FALSE);
                if (edge == WMSZ_RIGHT) OffsetRect(&initial, area.left + spacing - initial.left - insets.left, 0);
                else OffsetRect(&initial, 0, area.bottom - spacing - initial.bottom - insets.bottom);
                Assert::IsTrue(SetWindowPos(window.handle, nullptr, initial.left, initial.top,
                    rectWidth(initial), rectHeight(initial), SWP_NOACTIVATE | SWP_NOZORDER) != FALSE);
                Assert::IsTrue(SUCCEEDED(DwmFlush()));
                setWindowSpacing(spacing);
                snapper_OnEnterSizeMove(window.handle);
                SetScreens({area});
                RECT proposed = initial;
                if (edge == WMSZ_RIGHT) proposed.right = area.right + 100;
                else proposed.top = area.top - 100;
                snapper_OnSizing(window.handle, edge, &proposed);
                if (edge == WMSZ_RIGHT) Assert::AreEqual(initial.left, proposed.left, L"Fixed outer left edge");
                else Assert::AreEqual(initial.bottom, proposed.bottom, L"Fixed outer bottom edge");
                Assert::IsTrue(SetWindowPos(window.handle, nullptr, proposed.left, proposed.top,
                    rectWidth(proposed), rectHeight(proposed), SWP_NOACTIVATE | SWP_NOZORDER) != FALSE);
                Assert::IsTrue(SUCCEEDED(DwmFlush()));
                RECT visible = {};
                Assert::IsTrue(SUCCEEDED(DwmGetWindowAttribute(window.handle,
                    DWMWA_EXTENDED_FRAME_BOUNDS, &visible, sizeof(visible))));
                if (edge == WMSZ_RIGHT) {
                    Assert::AreEqual(area.left + spacing, visible.left);
                    Assert::AreEqual(area.right - spacing, visible.right);
                } else {
                    Assert::AreEqual(area.top + spacing, visible.top);
                    Assert::AreEqual(area.bottom - spacing, visible.bottom);
                }
            }
        }
    }

    TEST_CLASS(CenterResizeAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            SnapFrame_TestSetOps(nullptr);
            setKeptToScreen(TRUE);
            setCenterKey(VK_CONTROL);
            setEqualKey(VK_SHIFT);
            centerHeld = true;
            SnapKeys_TestSetReader(CenterResizeKeys);
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            SnapKeys_TestSetReader(nullptr);
            ResetEngine();
        }

        // DC-016: Ctrl preserves the original center while limiting both moving sides.
        TEST_METHOD(CenteredRightResizeFitsDesktopAndMargin)
        {
            for (int spacing : {0, 8})
                AssertRect({-28500 + spacing, -28400, -28000 - spacing, -28100},
                    CenterResize(WMSZ_RIGHT, {-28400, -28400, -27900, -28100}, spacing));
        }
        TEST_METHOD(CenteredLeftResizeFitsDesktopAndMargin)
        {
            for (int spacing : {0, 8})
                AssertRect({-28500 + spacing, -28400, -28000 - spacing, -28100},
                    CenterResize(WMSZ_LEFT, {-28600, -28400, -28100, -28100}, spacing));
        }
        TEST_METHOD(CenteredTopResizeFitsDesktopAndMargin)
        {
            for (int spacing : {0, 8})
                AssertRect({-28400, -28500 + spacing, -28100, -28000 - spacing},
                    CenterResize(WMSZ_TOP, {-28400, -28600, -28100, -28100}, spacing));
        }
        TEST_METHOD(CenteredBottomResizeFitsDesktopAndMargin)
        {
            for (int spacing : {0, 8})
                AssertRect({-28400, -28500 + spacing, -28100, -28000 - spacing},
                    CenterResize(WMSZ_BOTTOM, {-28400, -28400, -28100, -27900}, spacing));
        }
        TEST_METHOD(CenteredCornerResizeFitsBothAxes)
        {
            for (int spacing : {0, 8})
                AssertRect({-28500 + spacing, -28500 + spacing, -28000 - spacing, -28000 - spacing},
                    CenterResize(WMSZ_BOTTOMRIGHT, {-28400, -28400, -27900, -27900}, spacing));
        }
        TEST_METHOD(CenteredRightResizeAlsoHonorsNearerLeftBoundary)
        {
            for (int spacing : {0, 8})
                AssertRect({-28450 + spacing, -28400, -28050 - spacing, -28100},
                    CenterResize(WMSZ_RIGHT, {-28400, -28400, -27900, -28100}, spacing,
                        {-28450, -28500, -28000, -28000}));
        }
        TEST_METHOD(CenteredBottomResizeAlsoHonorsNearerTopBoundary)
        {
            for (int spacing : {0, 8})
                AssertRect({-28400, -28450 + spacing, -28100, -28050 - spacing},
                    CenterResize(WMSZ_BOTTOM, {-28400, -28400, -28100, -27900}, spacing,
                        {-28500, -28450, -28000, -28000}));
        }
        TEST_METHOD(CenteredProximitySnapCannotReflectOppositeSideOffDesktop)
        {
            AssertRect({-28500, -28400, -28000, -28100},
                CenterResize(WMSZ_RIGHT, {-28400, -28400, -28002, -28100}, 0,
                    {-28500, -28500, -28000, -28000}, true));
        }
        TEST_METHOD(CenteredRightGestureConfinesAnOppositeLeftSideSnap)
        {
            PhysicalCoordinates coordinates;
            CenteredWindow window;
            const RECT area = {-28450, -28450, -27450, -27650};
            for (int spacing : {0, 8})
            {
                setWindowSpacing(spacing);
                setSnapType(SNAPT_OTHERS);
                snapper_OnEnterSizeMove(window.handle);
                SetScreens({area});
                SetWindows({{area.left + spacing - 5, -28400,
                    area.left + spacing + 195, -28100}});
                RECT proposed = {-28400, -28400, -28050 - spacing, -28100};
                snapper_OnSizing(window.handle, WMSZ_RIGHT, &proposed);
                AssertRect({area.left + spacing, -28400, -28050 - spacing, -28100}, proposed);
            }
        }
        TEST_METHOD(CenteredBottomGestureConfinesAnOppositeTopSideSnap)
        {
            PhysicalCoordinates coordinates;
            CenteredWindow window;
            const RECT area = {-28450, -28450, -27450, -27650};
            for (int spacing : {0, 8})
            {
                setWindowSpacing(spacing);
                setSnapType(SNAPT_OTHERS);
                snapper_OnEnterSizeMove(window.handle);
                SetScreens({area});
                SetWindows({{-28400, area.top + spacing - 5,
                    -28100, area.top + spacing + 195}});
                RECT proposed = {-28400, -28400, -28100, -28050 - spacing};
                snapper_OnSizing(window.handle, WMSZ_BOTTOM, &proposed);
                AssertRect({-28400, area.top + spacing, -28100, -28050 - spacing}, proposed);
            }
        }
        TEST_METHOD(NativeFramedCenteredRightResizeKeepsVisibleMargins) { NativeCenterResize(WMSZ_RIGHT); }
        TEST_METHOD(NativeFramedCenteredBottomResizeKeepsVisibleMargins) { NativeCenterResize(WMSZ_BOTTOM); }
        TEST_METHOD(NativeFramedCenteredLeftResizeKeepsVisibleMargins) { NativeCenterResize(WMSZ_LEFT); }
        TEST_METHOD(NativeFramedCenteredTopResizeKeepsVisibleMargins) { NativeCenterResize(WMSZ_TOP); }
        TEST_METHOD(NativeFramedCenteredCornerResizeKeepsVisibleMargins) { NativeCenterResize(WMSZ_BOTTOMRIGHT); }
        TEST_METHOD(NativeOrdinaryRightResizeAllowsFixedVisibleLeftAtDesktopMargin) { NativeOrdinaryResize(WMSZ_RIGHT); }
        TEST_METHOD(NativeOrdinaryTopResizeAllowsFixedVisibleBottomAtDesktopMargin) { NativeOrdinaryResize(WMSZ_TOP); }
    };
}
