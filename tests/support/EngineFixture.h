#pragma once

#include <windows.h>
#include <initializer_list>
#include "CppUnitTest.h"

extern "C" {
#define SNAPLIBAPI
#include "snap_lib.h"
#include "snap_testers.h"
#include "snap_WinRects.h"
#include "snap_InTests.h"
#include "snap_CornerInfo.h"
#include "snap_TrackResults.h"
#include "snap_frame.h"
void reposition(LPCRECT unsnapped, SNAP_RESULTS * results, LPRECT rectangle);
void resize(SNAP_RESULTS * results, LPRECT rectangle);
void setMovingSnapped(BOOL snapped);
}

namespace AllSnapTests
{
    using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

    inline void AssertRect(const RECT & expected, const RECT & actual)
    {
        Assert::AreEqual(expected.left, actual.left, L"left");
        Assert::AreEqual(expected.top, actual.top, L"top");
        Assert::AreEqual(expected.right, actual.right, L"right");
        Assert::AreEqual(expected.bottom, actual.bottom, L"bottom");
    }

    inline void SetScreens(std::initializer_list<RECT> screens, const RECT * current = nullptr)
    {
        WinRects_TestSetScreens(screens.begin(), static_cast<int>(screens.size()), current);
    }

    inline void SetWindows(std::initializer_list<RECT> windows)
    {
        WinRects_TestSetWindows(windows.begin(), static_cast<int>(windows.size()));
    }

    inline void ResetEngine()
    {
        setEnabled(TRUE);
        setNoisy(FALSE);
        setSnapType(SNAPT_NONE);
        setKeptToScreen(FALSE);
        setSnappingInsides(TRUE);
        setWinThresh(10);
        setScreenThresh(10);
        setWindowSpacing(0);
        SnapFrame_SetCoordinateWindow(nullptr);
        setSnapMdi(FALSE);
        setCroppingTop(FALSE);
        setCroppingRgn(FALSE);
        setDisableToggle(TRUE);
        setToggleKey(VK_MENU);
        gridsnap_settings_t grid = {};
        setGridSnap(&grid);
        SetScreens({{0, 0, 1920, 1040}});
        setMovingSnapped(FALSE);
        TrackRslts_Reset();
    }

    inline SNAP_RESULTS EvaluateMove(const RECT & proposed)
    {
        TEST_RECTS candidates = {};
        WinRects_getRects(&candidates);
        SNAP_RESULTS results = {};
        moving_test_all(&proposed, getWinThresh(), getScreenThresh(), &candidates, &results);
        return results;
    }

    // Apply results through the production motion code, not a test copy of its algorithm.
    // Each call represents a new gesture; no live window or system-wide hook is created.
    inline RECT Move(const RECT & proposed)
    {
        setMovingSnapped(FALSE);
        TrackRslts_Reset();
        SNAP_RESULTS results = EvaluateMove(proposed);
        RECT rectangle = proposed;
        reposition(&proposed, &results, &rectangle);
        return rectangle;
    }

    inline RECT Resize(const RECT & proposed, SIDE verticalSide, SIDE horizontalSide)
    {
        TEST_RECTS candidates = {};
        WinRects_getRects(&candidates);
        SNAP_RESULTS results = {};
        sizing_test_all(verticalSide, horizontalSide, &proposed, getWinThresh(),
            getScreenThresh(), &candidates, &results, FALSE);
        RECT rectangle = proposed;
        resize(&results, &rectangle);
        return rectangle;
    }
}
