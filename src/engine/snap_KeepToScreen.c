#include "stdafx.h"
#include "snap_WinRects.h"
#include "snap_testinfo.h"
#include "snap_InTests.h"
#include "snap_frame.h"
#include <limits.h>

/* Called only after the bounded full-fit search proves no complete placement exists. */
static BOOL FindReachableOrigin(const RECT * desired, RECT * placement){
    RECT * areas = WinRects_GetScreens();
    LONGLONG best_distance = LLONG_MAX;
    int width = rectWidth(*desired), height = rectHeight(*desired);
    int index, count = WinRects_GetNumScreens();
    BOOL found = FALSE;
    if (width <= 0 || height <= 0) return FALSE;
    for (index = 0; index < count; ++index){
        RECT area, candidate;
        LONGLONG dx, dy, distance;
        SnapFrame_InsetWorkArea(&areas[index],&area);
        candidate.left = width <= rectWidth(area)
            ? max(area.left,min(desired->left,area.right-width)) : area.left;
        candidate.top = height <= rectHeight(area)
            ? max(area.top,min(desired->top,area.bottom-height)) : area.top;
        candidate.right = candidate.left + width;
        candidate.bottom = candidate.top + height;
        dx = (LONGLONG)candidate.left - desired->left;
        dy = (LONGLONG)candidate.top - desired->top;
        distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (distance < best_distance){
            *placement = candidate;
            best_distance = distance;
            found = TRUE;
        }
    }
    return found;
}

static void ForceSide(SNAP_RESULTS * results, enum SIDE side, LONG value){
    SIDE_SNAP_RESULTS * result = isVerticalSide(side) ? &results->v : &results->h;
    result->side = side;
    result->to_side = side;
    result->value = value;
    result->closeness = 0;
    result->thresh = 0;
    result->from_oss = FALSE;
}

void KeepToScreen(TEST_INFO * info, SNAP_RESULTS * results){
    RECT desired = *info->mine;
    RECT placement;
    enum SIDE horizontal_side = info->h_sizing_side;
    enum SIDE vertical_side = info->v_sizing_side;
    if (WinRects_GetNumScreens() == 0) return;

    /* Center sizing may match the opposite side; Snapper uses that matched
       side for its final reflection, so confinement must solve the same one. */
    if (info->sizing && info->centering){
        if (results->h.side != SIDE_NONE) horizontal_side = results->h.side;
        if (results->v.side != SIDE_NONE) vertical_side = results->v.side;
    }

    /* Confinement validates the complete proposed snap, not just the unsnapped input. */
    if (results->h.side != SIDE_NONE){
        if (info->sizing) SetSideOfRect(results->h.side,results->h.value,&desired);
        else AlignToSide(&desired,results->h.side,results->h.value);
    }
    if (results->v.side != SIDE_NONE){
        if (info->sizing) SetSideOfRect(results->v.side,results->v.value,&desired);
        else AlignToSide(&desired,results->v.side,results->v.value);
    }
    if (info->sizing && info->centering){
        /* The final CenterSize_Adjust reflects snapped dragged edges around the
           original center. Validate that same geometry before confinement. */
        if (horizontal_side != SIDE_NONE){
            LONGLONG opposite = (LONGLONG)info->mine->left + info->mine->right
                - GetSideOfRect(horizontal_side,&desired);
            if (opposite < LONG_MIN || opposite > LONG_MAX){ clear_snap_results(results); return; }
            SetSideOfRect(OppositeSide(horizontal_side),(LONG)opposite,&desired);
        }
        if (vertical_side != SIDE_NONE){
            LONGLONG opposite = (LONGLONG)info->mine->top + info->mine->bottom
                - GetSideOfRect(vertical_side,&desired);
            if (opposite < LONG_MIN || opposite > LONG_MAX){ clear_snap_results(results); return; }
            SetSideOfRect(OppositeSide(vertical_side),(LONG)opposite,&desired);
        }
    }
    if (InTests_RectInDesktop(&desired)) return;

    if (info->sizing){
        if (!InTests_FindNearestResize(&desired,vertical_side,horizontal_side,info->centering,&placement)){
            /* A fixed opposite edge already outside the desktop may make a full fit
               impossible. Never invent movement of that fixed edge. */
            clear_snap_results(results);
            return;
        }
        clear_snap_results(results);
        if (horizontal_side != SIDE_NONE
            && GetSideOfRect(horizontal_side,&placement) != GetSideOfRect(horizontal_side,info->mine))
            ForceSide(results,horizontal_side,GetSideOfRect(horizontal_side,&placement));
        if (vertical_side != SIDE_NONE
            && GetSideOfRect(vertical_side,&placement) != GetSideOfRect(vertical_side,info->mine))
            ForceSide(results,vertical_side,GetSideOfRect(vertical_side,&placement));
        return;
    }

    if (!InTests_FindNearestMove(&desired,&placement) && !FindReachableOrigin(&desired,&placement)) return;
    clear_snap_results(results);
    if (placement.left != info->mine->left) ForceSide(results,SIDE_LEFT,placement.left);
    if (placement.top != info->mine->top) ForceSide(results,SIDE_TOP,placement.top);
}
