#ifndef SNAP_INTESTS_INCLUDE
#define SNAP_INTESTS_INCLUDE

#include "snap_CornerInfo.h"
#include "sides.h"

BOOL InTests_GetClosestScreenToPt(POINT pt, enum SIDE side, int * p_dist,LPRECT pScreen);

int InTests_GetClosestScreenToSide(enum SIDE side,LPCRECT pRect, LPRECT pScreen);

void InTests_TestAllCorners(P_CORNER_INFO pci,LPCRECT pRect);
BOOL InTests_isPtInMonitors(int x, int y);
BOOL InTests_RectInDesktop(const RECT * rectangle);
BOOL InTests_FindNearestMove(const RECT * desired, RECT * placement);
BOOL InTests_FindNearestResize(const RECT * desired, enum SIDE vertical_side, enum SIDE horizontal_side, BOOL centering, RECT * placement);

#endif
