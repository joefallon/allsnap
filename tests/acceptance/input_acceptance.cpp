#include "../support/EngineFixture.h"
#include "snap_keys.h"
#include "snap_lifecycle_test.h"
extern "C" {
#include "snapper.h"
#include "snap_CenterSize.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        SHORT menuState, controlState;
        int subclassAttempts;
        SHORT WINAPI ReadInputState(int key)
        {
            if (key == VK_MENU) return menuState;
            if (key == VK_CONTROL) return controlState;
            return 0;
        }
        BOOL WINAPI CountSubclassAttempt(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR)
        {
            ++subclassAttempts;
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
        RECT MoveWithProductionToggle()
        {
            // A new gesture avoids depending on the live cursor position used
            // only when continuing an already-snapped gesture.
            setMovingSnapped(FALSE);
            TrackRslts_Reset();
            RECT proposed = {5, 100, 205, 300};
            snapper_OnMoving(nullptr, &proposed);
            return proposed;
        }
    }

    TEST_CLASS(InputAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            SnapTest_LifecycleReset();
            setSnapType(SNAPT_DESKTOP);
            setCenterKey(VK_CONTROL);
            menuState = controlState = 0;
            subclassAttempts = 0;
            SnapKeys_TestSetReader(ReadInputState);
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            SnapKeys_TestSetReader(nullptr);
            SnapTest_LifecycleReset();
            ResetEngine();
        }

        // IN-001: default toggle means hold the configured key to bypass snapping.
        TEST_METHOD(DefaultToggleReleasedAllowsSnapping)
        {
            AssertRect({0, 100, 200, 300}, MoveWithProductionToggle());
        }
        TEST_METHOD(DefaultToggleHeldBypassesSnapping)
        {
            menuState = static_cast<SHORT>(0x8000);
            AssertRect({5, 100, 205, 300}, MoveWithProductionToggle());
        }
        TEST_METHOD(DefaultToggleRecentlyReleasedAllowsSnapping)
        {
            menuState = 0x0001;
            AssertRect({0, 100, 200, 300}, MoveWithProductionToggle());
        }

        // IN-002: inverted toggle requires the key to be held for snapping.
        TEST_METHOD(InvertedToggleReleasedBypassesSnapping)
        {
            setDisableToggle(FALSE);
            AssertRect({5, 100, 205, 300}, MoveWithProductionToggle());
        }
        TEST_METHOD(InvertedToggleHeldAllowsSnapping)
        {
            setDisableToggle(FALSE);
            menuState = static_cast<SHORT>(0x8000);
            AssertRect({0, 100, 200, 300}, MoveWithProductionToggle());
        }
        TEST_METHOD(InvertedToggleRecentlyReleasedBypassesSnapping)
        {
            setDisableToggle(FALSE);
            menuState = 0x0001;
            AssertRect({5, 100, 205, 300}, MoveWithProductionToggle());
        }

        // IN-003: the center-resize modifier follows held state, not recent presses.
        TEST_METHOD(CenterResizeModifierReleasedIsInactive)
        {
            Assert::IsFalse(isCentered() != FALSE);
        }
        TEST_METHOD(CenterResizeModifierHeldIsActive)
        {
            controlState = static_cast<SHORT>(0x8000);
            Assert::IsTrue(isCentered() != FALSE);
        }
        TEST_METHOD(CenterResizeModifierRecentlyReleasedIsInactive)
        {
            controlState = 0x0001;
            Assert::IsFalse(isCentered() != FALSE);
        }

        // IN-004: disabled dispatch must never attempt to subclass a window.
        TEST_METHOD(DisabledSnappingPreventsHookDispatchRegistration)
        {
            HWND window = CreateWindowExW(0, L"STATIC", L"allSnap input test", WS_POPUP,
                -30000, -30000, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            Assert::IsNotNull(window);
            SNAP_LIFECYCLE_TEST_OPS operations = {nullptr, nullptr, CountSubclassAttempt, nullptr};
            SnapTest_SetLifecycleOps(&operations);
            setEnabled(FALSE);
            CWPSTRUCT message = {};
            message.hwnd = window;
            message.message = WM_ENTERSIZEMOVE;
            SnapTest_CallHook(HC_ACTION, 0, reinterpret_cast<LPARAM>(&message));
            DestroyWindow(window);
            Assert::AreEqual(0, subclassAttempts);
            Assert::IsTrue(SnapCanUnHook() != FALSE);
        }

        // IN-005: the Advanced inside-edge exclusion changes actual matching.
        TEST_METHOD(InsideEdgesAreSnapTargetsWhenEnabled)
        {
            setSnapType(SNAPT_OTHERS);
            SetWindows({{500, 100, 700, 400}});
            AssertRect({500, 150, 700, 350}, Move({505, 150, 705, 350}));
        }
        TEST_METHOD(InsideEdgesAreExcludedWhenRequested)
        {
            setSnapType(SNAPT_OTHERS);
            setSnappingInsides(FALSE);
            SetWindows({{500, 100, 700, 400}});
            AssertRect({505, 150, 705, 350}, Move({505, 150, 705, 350}));
        }
    };
}
