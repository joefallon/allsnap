#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(SnappingAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }

        // WS-001: moving inside the screen threshold aligns the complete window.
        TEST_METHOD(MovingNearScreenEdgePreservesWindowSize)
        {
            setSnapType(SNAPT_DESKTOP);
            AssertRect({0, 100, 200, 300}, Move({5, 100, 205, 300}));
        }

        // WS-002: an activation threshold is not a permanent spacing offset.
        TEST_METHOD(MovingNearOtherWindowProducesTouchingEdges)
        {
            setSnapType(SNAPT_OTHERS);
            SetWindows({{500, 100, 700, 400}});
            AssertRect({300, 150, 500, 350}, Move({295, 150, 495, 350}));
        }

        // WS-003: proximity snapping does not apply beyond the threshold.
        TEST_METHOD(MovingBeyondWindowThresholdRemainsUnchanged)
        {
            setSnapType(SNAPT_OTHERS);
            SetWindows({{500, 100, 700, 400}});
            const RECT proposed = {289, 150, 489, 350};
            AssertRect(proposed, Move(proposed));
        }

        // WS-004: resizing changes the dragged side and preserves its opposite side.
        TEST_METHOD(ResizingNearWindowEdgeKeepsOppositeSideFixed)
        {
            setSnapType(SNAPT_OTHERS);
            SetWindows({{500, 100, 700, 400}});
            AssertRect({100, 150, 500, 350}, Resize({100, 150, 495, 350}, SIDE_NONE, SIDE_RIGHT));
        }

        // WS-005: disabling snap destinations leaves geometry unchanged.
        TEST_METHOD(NoEnabledSnapDestinationsLeavesRectangleUnchanged)
        {
            SetWindows({{500, 100, 700, 400}});
            const RECT proposed = {295, 150, 495, 350};
            AssertRect(proposed, Move(proposed));
        }

        // WS-009: the configured activation distance includes its boundary.
        TEST_METHOD(MovingAtWindowThresholdActivatesSnapping)
        {
            setSnapType(SNAPT_OTHERS);
            SetWindows({{500,100,700,400}});
            AssertRect({300,150,500,350},Move({290,150,490,350}));
        }
    };
}
