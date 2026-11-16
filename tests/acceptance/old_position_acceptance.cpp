#include "../support/EngineFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    class OldPositionWindow
    {
    public:
        HWND handle;
        RECT bounds;

        OldPositionWindow()
            : handle(CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"allSnap old-position test",
                WS_POPUP, -28531, -28479, 200, 200, nullptr, nullptr, GetModuleHandle(nullptr), nullptr)), bounds{}
        {
            Assert::IsTrue(handle != nullptr, L"Create only a hidden test-owned window");
            Assert::IsTrue(GetWindowRect(handle, &bounds) != FALSE);
        }

        ~OldPositionWindow() { if (handle) DestroyWindow(handle); }
        OldPositionWindow(const OldPositionWindow &) = delete;
        OldPositionWindow & operator=(const OldPositionWindow &) = delete;

        int InitialPositionOccurrences() const
        {
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            int count = 0;
            for (int index = 0; index < candidates.num_win_rects; ++index)
                if (EqualRect(&bounds, &candidates.win_rects[index])) ++count;
            return count;
        }
    };

    TEST_CLASS(OldPositionAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize) { ResetEngine(); }
        TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

        // WS-006: old position is a complete destination without other-window snapping.
        TEST_METHOD(OldPositionOnlyCapturesAndRestoresTheInitialRectangle)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            WinRects_Refresh(getSnapType(), window.handle);
            TEST_RECTS candidates = {};
            WinRects_getRects(&candidates);
            Assert::AreEqual(1, candidates.num_win_rects);
            Assert::AreEqual(1, window.InitialPositionOccurrences());
            RECT moved = window.bounds;
            OffsetRect(&moved, 7, 7);
            AssertRect(window.bounds, Move(moved));
        }

        TEST_METHOD(OldPositionOnlyRestoresTheDraggedResizeEdge)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            WinRects_Refresh(getSnapType(), window.handle);
            RECT resized = window.bounds;
            resized.right -= 5;
            AssertRect(window.bounds, Resize(resized, SIDE_NONE, SIDE_RIGHT));
        }

        // WS-007: enabling other destinations does not duplicate the old position.
        TEST_METHOD(OldPositionAndOtherWindowsCaptureInitialRectangleOnce)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF | SNAPT_OTHERS);
            WinRects_Refresh(getSnapType(), window.handle);
            Assert::AreEqual(1, window.InitialPositionOccurrences());
        }

        // WS-008: other-window snapping does not implicitly enable old-position snapping.
        TEST_METHOD(OtherWindowsOnlyDoesNotCaptureTheDraggedWindow)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_OTHERS);
            WinRects_Refresh(getSnapType(), window.handle);
            Assert::AreEqual(0, window.InitialPositionOccurrences());
        }

        // WS-010: Old position is not an ordinary same-side neighbor exclusion.
        TEST_METHOD(OldPositionWorksWhenOrdinaryInsideEdgesAreExcluded)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            setSnappingInsides(FALSE);
            WinRects_Refresh(getSnapType(), window.handle);
            RECT moved = window.bounds;
            OffsetRect(&moved, 7, 7);
            AssertRect(window.bounds, Move(moved));
        }

        TEST_METHOD(OldPositionResizeWorksWhenOrdinaryInsideEdgesAreExcluded)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            setSnappingInsides(FALSE);
            WinRects_Refresh(getSnapType(), window.handle);
            RECT resized = window.bounds;
            resized.right -= 7;
            AssertRect(window.bounds, Resize(resized, SIDE_NONE, SIDE_RIGHT));
        }

        // WS-011: a saved position is not a phantom adjacent window.
        TEST_METHOD(OldPositionDoesNotCreateAnAdjacentGhostWindow)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            setWindowSpacing(8);
            WinRects_Refresh(getSnapType(), window.handle);
            RECT moved = window.bounds;
            OffsetRect(&moved, rectWidth(window.bounds) + 3, 0);
            AssertRect(moved, Move(moved));
        }

        TEST_METHOD(ReplacingCandidatesClearsOldPositionClassification)
        {
            OldPositionWindow window;
            setSnapType(SNAPT_SELF);
            WinRects_Refresh(getSnapType(), window.handle);
            SetWindows({{500, 100, 700, 400}});
            setSnapType(SNAPT_OTHERS);
            setSnappingInsides(FALSE);
            AssertRect({300, 150, 500, 350}, Move({295, 150, 495, 350}));
        }
    };
}
