#include "../support/EngineFixture.h"
#include "snap_keys.h"
extern "C" {
#include "snapper.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        SHORT WINAPI GridKeysReleased(int) { return 0; }
        class GridMdiWindow
        {
        public:
            HWND frame = nullptr, client = nullptr, child = nullptr;
            GridMdiWindow()
            {
                WNDCLASSW type = {};
                HINSTANCE instance = GetModuleHandleW(nullptr);
                if (!GetClassInfoW(instance, L"AllSnapGridMdi", &type)) {
                    type.hInstance = instance; type.lpfnWndProc = DefMDIChildProcW;
                    type.lpszClassName = L"AllSnapGridMdi";
                    Assert::IsTrue(RegisterClassW(&type) != 0);
                }
                frame = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"allSnap MDI grid fixture",
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, -30000, -30000, 1000, 800,
                    nullptr, nullptr, instance, nullptr);
                Assert::IsNotNull(frame);
                CLIENTCREATESTRUCT configuration = {}; configuration.idFirstChild = 100;
                client = CreateWindowExW(0, L"MDICLIENT", L"", WS_CHILD | WS_VISIBLE,
                    0, 0, 900, 700, frame, nullptr, instance, &configuration);
                Assert::IsNotNull(client);
                child = CreateMDIWindowW(L"AllSnapGridMdi", L"owned grid child", WS_VISIBLE,
                    30, 40, 220, 180, client, instance, 0);
                Assert::IsNotNull(child); ShowWindow(frame, SW_SHOWNOACTIVATE);
            }
            ~GridMdiWindow() { if (IsWindow(frame)) DestroyWindow(frame); }
        };

        void AssertMdiGridMatches(bool horizontal)
        {
            GridMdiWindow window;
            gridsnap_settings_t grid = {{horizontal, GRIDTYPE_SINGLE, 500},
                {!horizontal, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid); setSnapType(SNAPT_NONE); setSnapMdi(TRUE);
            snapper_OnEnterSizeMove(window.child);
            RECT client = {}, outer = {}, visible = {};
            Assert::IsTrue(GetClientRect(window.client, &client) != FALSE);
            MapWindowPoints(window.client, nullptr, reinterpret_cast<POINT *>(&client), 2);
            Assert::IsTrue(GetWindowRect(window.child, &outer) != FALSE);
            Assert::IsTrue(WinRects_GetWindowRect(window.child, &visible) != FALSE);
            LONG guide = (horizontal ? client.top : client.left) + 500;
            RECT proposed = outer;
            OffsetRect(&proposed, horizontal ? 0 : guide - 7 - visible.left,
                horizontal ? guide - 7 - visible.top : 0);
            RECT expected = proposed; OffsetRect(&expected, horizontal ? 0 : 7, horizontal ? 7 : 0);
            snapper_OnMoving(window.child, &proposed);
            AssertRect(expected, proposed);

            snapper_OnEnterSizeMove(window.child);
            proposed = outer;
            if (horizontal) proposed.bottom = guide - 7 - (visible.bottom - outer.bottom);
            else proposed.right = guide - 7 - (visible.right - outer.right);
            expected = proposed;
            if (horizontal) expected.bottom += 7; else expected.right += 7;
            snapper_OnSizing(window.child, horizontal ? WMSZ_BOTTOM : WMSZ_RIGHT, &proposed);
            AssertRect(expected, proposed);
        }
    }

    TEST_CLASS(GridAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); SnapKeys_TestSetReader(GridKeysReleased); }
        TEST_METHOD_CLEANUP(Cleanup) { SnapKeys_TestSetReader(nullptr); ResetEngine(); }

        TEST_METHOD(MG01_GridGuidesDoNotHideLaterMonitorWorkAreas)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}, {1000, 0, 2000, 1000}};
            // Keep both vertical edges outside the horizontal guides' snap threshold.
            const RECT window = {1900, 117, 2100, 317};
            gridsnap_settings_t grid = {{TRUE, GRIDTYPE_EVEN, 20}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            setKeptToScreen(TRUE);
            WinRects_TestRefreshScreens(screens, 2, getSnapType(), &window);
            AssertRect({1800, 117, 2000, 317}, Move(window));
        }

        TEST_METHOD(MG02_GridLinesDoNotBecomeConfinementWorkAreas)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {100, 100, 300, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid);
            WinRects_TestRefreshScreens(screens, 1, SNAPT_DESKTOP, &window);
            Assert::AreEqual(1, WinRects_GetNumScreens());
            AssertRect(screens[0], WinRects_GetScreens()[0]);
        }

        TEST_METHOD(MG03_ValidGridRemainsASnappingCandidate)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {493, 100, 693, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
            AssertRect({500, 100, 700, 300}, Move(window));
        }

        TEST_METHOD(MG04_InvalidGridSettingsLeaveConfinementAvailable)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}, {1000, 0, 2000, 1000}};
            const RECT window = {1900, 100, 2100, 300};
            gridsnap_settings_t grid = {{TRUE, static_cast<gridsnap_type_t>(99), 10},
                {TRUE, GRIDTYPE_PIXELS, -10}};
            setGridSnap(&grid);
            setSnapType(SNAPT_DESKTOP);
            setKeptToScreen(TRUE);
            WinRects_TestRefreshScreens(screens, 2, getSnapType(), &window);
            AssertRect({1800, 100, 2000, 300}, Move(window));
        }

        TEST_METHOD(MG05_HorizontalGridMatchesWithDesktopEdgesDisabled)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {100, 493, 300, 693};
            gridsnap_settings_t grid = {{TRUE, GRIDTYPE_SINGLE, 500}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&grid);
            setSnapType(SNAPT_NONE);
            WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
            AssertRect({100, 500, 300, 700}, Move(window));
            AssertRect({100, 500, 300, 693}, Resize(window, SIDE_TOP, SIDE_NONE));
        }

        TEST_METHOD(MG05_VerticalGridMatchesWithDesktopEdgesDisabled)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {493, 100, 693, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid);
            setSnapType(SNAPT_NONE);
            WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
            AssertRect({500, 100, 700, 300}, Move(window));
            AssertRect({500, 100, 693, 300}, Resize(window, SIDE_NONE, SIDE_LEFT));
        }

        TEST_METHOD(MG06_HorizontalSingleZeroCreatesAnUnpaddedBoundaryGuide)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {100, 1, 300, 201};
            gridsnap_settings_t grid = {{TRUE, GRIDTYPE_SINGLE, 0}, {FALSE, GRIDTYPE_EVEN, 0}};
            setGridSnap(&grid);
            setWindowSpacing(8); setScreenThresh(2);
            for (UINT destinations : {SNAPT_NONE, SNAPT_DESKTOP}) {
                setSnapType(destinations);
                WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
                // The padded work-area top at 8 is outside threshold; only guide 0 can match.
                AssertRect({100, 0, 300, 200}, Move(window));
                AssertRect({100, 0, 300, 201}, Resize(window, SIDE_TOP, SIDE_NONE));
            }
        }

        TEST_METHOD(MG06_VerticalSingleZeroCreatesAnUnpaddedBoundaryGuide)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {1, 100, 201, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 0}};
            setGridSnap(&grid);
            setWindowSpacing(8); setScreenThresh(2);
            for (UINT destinations : {SNAPT_NONE, SNAPT_DESKTOP}) {
                setSnapType(destinations);
                WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
                AssertRect({0, 100, 200, 300}, Move(window));
                AssertRect({0, 100, 201, 300}, Resize(window, SIDE_NONE, SIDE_LEFT));
            }
        }

        TEST_METHOD(MG07_IndependentGridDoesNotEnableDesktopEdgeMatching)
        {
            const RECT screens[] = {{0, 0, 1000, 1000}};
            const RECT window = {3, 100, 203, 300};
            gridsnap_settings_t grid = {{FALSE, GRIDTYPE_EVEN, 0}, {TRUE, GRIDTYPE_SINGLE, 500}};
            setGridSnap(&grid); setSnapType(SNAPT_NONE);
            WinRects_TestRefreshScreens(screens, 1, getSnapType(), &window);
            AssertRect(window, Move(window));
            const RECT sizing = {100, 100, 997, 300};
            AssertRect(sizing, Resize(sizing, SIDE_NONE, SIDE_RIGHT));
        }

        TEST_METHOD(MG08_NativeMdiHorizontalGridUsesClientOffsetWithDesktopDisabled)
        {
            AssertMdiGridMatches(true);
        }

        TEST_METHOD(MG08_NativeMdiVerticalGridUsesClientOffsetWithDesktopDisabled)
        {
            AssertMdiGridMatches(false);
        }
    };
}
