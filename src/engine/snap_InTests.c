#include "stdafx.h"
#include "snap_InTests.h"
#include "snap_WinRects.h"
#include "snap_frame.h"
#include <limits.h>

typedef struct BOUNDARY_SEGMENT {
	LONG low;
	LONG high;
} BOUNDARY_SEGMENT;

/* Ten supported work areas can split one rectangle edge into at most ten pieces. */
#define MAX_BOUNDARY_SEGMENTS 11

static void SideSpan(LPCRECT rectangle, enum SIDE side, LONG * low, LONG * high){
	if (isVerticalSide(side)){
		*low = rectangle->left;
		*high = rectangle->right;
	}else{
		*low = rectangle->top;
		*high = rectangle->bottom;
	}
}

static BOOL CoversOutsideOfEdge(LPCRECT area, LPCRECT neighbor, enum SIDE side){
	switch (side){
		case SIDE_LEFT: return neighbor->left < area->left && neighbor->right >= area->left;
		case SIDE_RIGHT: return neighbor->left <= area->right && neighbor->right > area->right;
		case SIDE_TOP: return neighbor->top < area->top && neighbor->bottom >= area->top;
		case SIDE_BOTTOM: return neighbor->top <= area->bottom && neighbor->bottom > area->bottom;
		default: return FALSE;
	}
}

/* Subtract only the portions of an edge covered across it by another work area. */
static int ExposedSegments(int area_index, enum SIDE side, BOUNDARY_SEGMENT * segments){
	int neighbor_index;
	int count = 1;
	RECT * areas = WinRects_GetScreens();
	int area_count = WinRects_GetNumScreens();
	SideSpan(&areas[area_index], side, &segments[0].low, &segments[0].high);
	if (segments[0].low >= segments[0].high) return 0;
	for (neighbor_index = 0; neighbor_index < area_count; ++neighbor_index){
		LONG cover_low, cover_high;
		int index;
		if (neighbor_index == area_index || !CoversOutsideOfEdge(&areas[area_index], &areas[neighbor_index], side))
			continue;
		SideSpan(&areas[neighbor_index], side, &cover_low, &cover_high);
		for (index = 0; index < count; ++index){
			BOUNDARY_SEGMENT segment = segments[index];
			if (cover_high <= segment.low || cover_low >= segment.high) continue;
			if (cover_low <= segment.low && cover_high >= segment.high){
				int next;
				for (next = index; next + 1 < count; ++next) segments[next] = segments[next + 1];
				--count;
				--index;
			}else if (cover_low <= segment.low){
				segments[index].low = cover_high;
			}else if (cover_high >= segment.high){
				segments[index].high = cover_low;
			}else{
				/* There is at most one additional interval per neighboring rectangle. */
				if (count >= MAX_BOUNDARY_SEGMENTS) return 0;
				segments[count].low = cover_high;
				segments[count++].high = segment.high;
				segments[index].high = cover_low;
			}
		}
	}
	return count;
}

static LONGLONG AbsoluteDistance(LONG first, LONG second){
	LONGLONG difference = (LONGLONG)first - second;
	return difference < 0 ? -difference : difference;
}

static LONGLONG IntervalDistance(LONG low, LONG high, LONG segment_low, LONG segment_high){
	if (high < segment_low) return (LONGLONG)segment_low - high;
	if (low > segment_high) return (LONGLONG)low - segment_high;
	return 0;
}


BOOL InTests_GetClosestScreenToPt(POINT pt, enum SIDE side, int * p_dist,LPRECT pScreen){
	RECT pt_in_rect_form = {pt.x,pt.y,pt.x,pt.y};
	*p_dist = InTests_GetClosestScreenToSide(side,&pt_in_rect_form,pScreen);
	return (*p_dist != -1);
}

int InTests_GetClosestScreenToSide(enum SIDE side,LPCRECT pRect, LPRECT pScreen){
	int area_index;
	BOOL found = FALSE;
	LONGLONG best_distance = LLONG_MAX;
	LONG low, high;
	RECT * areas = WinRects_GetScreens();
	int area_count = WinRects_GetNumScreens();
	SideSpan(pRect, side, &low, &high);
	for (area_index = 0; area_index < area_count; ++area_index){
		BOUNDARY_SEGMENT segments[MAX_BOUNDARY_SEGMENTS];
		int segment_count = ExposedSegments(area_index, side, segments);
		int segment_index;
		RECT padded;
		LONG boundary;
		LONGLONG distance, orthogonal_margin;
		SnapFrame_InsetWorkArea(&areas[area_index], &padded);
		boundary = GetSideOfRect(side, &padded);
		distance = AbsoluteDistance(GetSideOfRect(side,pRect), boundary);
		orthogonal_margin = isVerticalSide(side) ? padded.left - areas[area_index].left : padded.top - areas[area_index].top;
		for (segment_index = 0; segment_index < segment_count; ++segment_index){
			LONGLONG orthogonal = IntervalDistance(low, high, segments[segment_index].low, segments[segment_index].high);
			/* Padding extends around an exposed segment's endpoint as well as along its normal. */
			if (orthogonal != 0 && orthogonal >= orthogonal_margin) continue;
			if (!found || distance < best_distance){
				*pScreen = areas[area_index];
				SetSideOfRect(side, boundary, pScreen);
				best_distance = distance;
				found = TRUE;
			}
		}
	}
	return found ? (int)min(best_distance, INT_MAX) : -1;
}



BOOL isPtInRect(LPCRECT pRect,POINT pt){
	return (	( (pt.x >= pRect->left) && (pt.x <= pRect->right))
			&&	( (pt.y >= pRect->top)  && (pt.y <= pRect->bottom)));
}

BOOL InTests_isPtInMonitors(int x, int y){
	int area_index;
	POINT pt = {x,y};
	int area_count = WinRects_GetNumScreens();
	RECT * areas = WinRects_GetScreens();
	BOOL inside = FALSE;
	for (area_index = 0; area_index < area_count; ++area_index){
		if (isPtInRect(&areas[area_index],pt)) inside = TRUE;
	}
	if (!inside) return FALSE;
	for (area_index = 0; area_index < area_count; ++area_index){
		enum SIDE side;
		RECT padded;
		SnapFrame_InsetWorkArea(&areas[area_index], &padded);
		for (side = SIDE_LEFT; side <= SIDE_BOTTOM; side = (enum SIDE)(side + 1)){
			BOUNDARY_SEGMENT segments[MAX_BOUNDARY_SEGMENTS];
			LONG axis = isVerticalSide(side) ? y : x;
			LONG orthogonal_axis = isVerticalSide(side) ? x : y;
			LONGLONG margin = AbsoluteDistance(GetSideOfRect(side,&areas[area_index]), GetSideOfRect(side,&padded));
			LONGLONG orthogonal_margin = isVerticalSide(side) ? padded.left - areas[area_index].left : padded.top - areas[area_index].top;
			int segment_count, segment_index;
			if (!margin || AbsoluteDistance(axis, GetSideOfRect(side,&areas[area_index])) >= margin) continue;
			segment_count = ExposedSegments(area_index, side, segments);
			for (segment_index = 0; segment_index < segment_count; ++segment_index){
				LONGLONG distance = IntervalDistance(orthogonal_axis, orthogonal_axis,
					segments[segment_index].low, segments[segment_index].high);
				if (distance == 0 || distance < orthogonal_margin) return FALSE;
			}
		}
	}
	return TRUE;
}


void InTests_TestAllCorners(P_CORNER_INFO pci,LPCRECT pRect){
	pci->topLeft_out	= !InTests_isPtInMonitors(pRect->left	,pRect->top		);
	pci->topRight_out	= !InTests_isPtInMonitors(pRect->right	,pRect->top		);
	pci->bottomLeft_out	= !InTests_isPtInMonitors(pRect->left	,pRect->bottom	);
	pci->bottomRight_out= !InTests_isPtInMonitors(pRect->right	,pRect->bottom	);
}

static BOOL RawPointInDesktop(LONG x, LONG y){
	POINT point = {x,y};
	RECT * areas = WinRects_GetScreens();
	int index, count = WinRects_GetNumScreens();
	for (index = 0; index < count; ++index){
		if (isPtInRect(&areas[index],point)) return TRUE;
	}
	return FALSE;
}

/* The whole rectangle plus its margin must belong to the union, including holes. */
BOOL InTests_RectInDesktop(const RECT * rectangle){
	int area_index;
	int count = WinRects_GetNumScreens();
	RECT * areas = WinRects_GetScreens();
	RECT expanded;
	POINT interior;
	BOOL interior_covered = FALSE;
	int x = SnapFrame_GetSpacing(SIDE_LEFT), y = SnapFrame_GetSpacing(SIDE_TOP);
	if (IsRectEmpty(rectangle)
		|| (LONGLONG)rectangle->left - x < LONG_MIN || (LONGLONG)rectangle->right + x > LONG_MAX
		|| (LONGLONG)rectangle->top - y < LONG_MIN || (LONGLONG)rectangle->bottom + y > LONG_MAX) return FALSE;
	expanded.left = rectangle->left - x;
	expanded.right = rectangle->right + x;
	expanded.top = rectangle->top - y;
	expanded.bottom = rectangle->bottom + y;
	interior.x = (LONG)((LONGLONG)expanded.left + ((LONGLONG)expanded.right - expanded.left)/2);
	interior.y = (LONG)((LONGLONG)expanded.top + ((LONGLONG)expanded.bottom - expanded.top)/2);
	/* Closed boundary points alone can surround a completely uncovered gap.
	   Probe the interior with RECT's half-open coverage before testing crossings. */
	for (area_index = 0; area_index < count; ++area_index){
		if (interior.x >= areas[area_index].left && interior.x < areas[area_index].right
			&& interior.y >= areas[area_index].top && interior.y < areas[area_index].bottom){
			interior_covered = TRUE;
			break;
		}
	}
	if (!interior_covered) return FALSE;
	if (!RawPointInDesktop(expanded.left,expanded.top) || !RawPointInDesktop(expanded.right,expanded.top)
		|| !RawPointInDesktop(expanded.left,expanded.bottom) || !RawPointInDesktop(expanded.right,expanded.bottom)) return FALSE;
	/* Corners alone miss a disconnected gap or enclosed hole. Any exposed edge
	   crossing the expanded interior proves that some of that interior is outside. */
	for (area_index = 0; area_index < count; ++area_index){
		enum SIDE side;
		for (side = SIDE_LEFT; side <= SIDE_BOTTOM; side = (enum SIDE)(side + 1)){
			BOUNDARY_SEGMENT segments[MAX_BOUNDARY_SEGMENTS];
			LONG low, high, boundary = GetSideOfRect(side,&areas[area_index]);
			LONG axis_low = isVerticalSide(side) ? expanded.top : expanded.left;
			LONG axis_high = isVerticalSide(side) ? expanded.bottom : expanded.right;
			int segment_index, segment_count;
			if (boundary <= axis_low || boundary >= axis_high) continue;
			SideSpan(&expanded,side,&low,&high);
			segment_count = ExposedSegments(area_index,side,segments);
			for (segment_index = 0; segment_index < segment_count; ++segment_index){
				if (segments[segment_index].low < high && segments[segment_index].high > low) return FALSE;
			}
		}
	}
	return TRUE;
}

/* Each of the ten work areas contributes at most two coordinates per axis. */
#define MAX_PLACEMENT_COORDINATES 21

static void AddCoordinate(LONG * coordinates, int * count, LONGLONG value){
	int index;
	if (value < LONG_MIN || value > LONG_MAX || *count >= MAX_PLACEMENT_COORDINATES) return;
	for (index = 0; index < *count; ++index){
		if (coordinates[index] == (LONG)value) return;
	}
	coordinates[(*count)++] = (LONG)value;
}

static int MoveCoordinates(const RECT * desired, BOOL horizontal, LONG * coordinates){
	enum SIDE first = horizontal ? SIDE_LEFT : SIDE_TOP;
	enum SIDE last = horizontal ? SIDE_RIGHT : SIDE_BOTTOM;
	LONGLONG extent = horizontal ? (LONGLONG)desired->right - desired->left : (LONGLONG)desired->bottom - desired->top;
	RECT * areas = WinRects_GetScreens();
	int area_index, count = 0, area_count = WinRects_GetNumScreens();
	AddCoordinate(coordinates,&count,horizontal ? desired->left : desired->top);
	for (area_index = 0; area_index < area_count; ++area_index){
		BOUNDARY_SEGMENT segments[MAX_BOUNDARY_SEGMENTS];
		if (ExposedSegments(area_index,first,segments) != 0)
			AddCoordinate(coordinates,&count,(LONGLONG)GetSideOfRect(first,&areas[area_index]) + SnapFrame_GetSpacing(first));
		if (ExposedSegments(area_index,last,segments) != 0)
			AddCoordinate(coordinates,&count,(LONGLONG)GetSideOfRect(last,&areas[area_index]) - SnapFrame_GetSpacing(last) - extent);
	}
	return count;
}

BOOL InTests_FindNearestMove(const RECT * desired, RECT * placement){
	LONG xs[MAX_PLACEMENT_COORDINATES], ys[MAX_PLACEMENT_COORDINATES];
	LONGLONG width = (LONGLONG)desired->right - desired->left;
	LONGLONG height = (LONGLONG)desired->bottom - desired->top;
	LONGLONG best_distance = LLONG_MAX;
	int xi, yi, x_count, y_count;
	BOOL found = FALSE;
	if (InTests_RectInDesktop(desired)) { *placement = *desired; return TRUE; }
	if (width <= 0 || height <= 0) return FALSE;
	x_count = MoveCoordinates(desired,TRUE,xs);
	y_count = MoveCoordinates(desired,FALSE,ys);
	for (xi = 0; xi < x_count; ++xi){
		for (yi = 0; yi < y_count; ++yi){
			RECT candidate;
			LONGLONG distance = AbsoluteDistance(xs[xi],desired->left) + AbsoluteDistance(ys[yi],desired->top);
			if (distance >= best_distance || (LONGLONG)xs[xi] + width > LONG_MAX || (LONGLONG)ys[yi] + height > LONG_MAX) continue;
			candidate.left = xs[xi]; candidate.top = ys[yi];
			candidate.right = (LONG)(xs[xi] + width); candidate.bottom = (LONG)(ys[yi] + height);
			if (InTests_RectInDesktop(&candidate)){
				*placement = candidate;
				best_distance = distance;
				found = TRUE;
			}
		}
	}
	return found;
}

static int ResizeCoordinates(const RECT * desired, enum SIDE side, BOOL centering, LONG * coordinates){
	RECT * areas = WinRects_GetScreens();
	int area_index, count = 0, area_count = WinRects_GetNumScreens();
	enum SIDE opposite;
	LONGLONG center_sum;
	if (side == SIDE_NONE){ coordinates[0] = 0; return 1; }
	opposite = OppositeSide(side);
	center_sum = (LONGLONG)GetSideOfRect(side,desired) + GetSideOfRect(opposite,desired);
	AddCoordinate(coordinates,&count,GetSideOfRect(side,desired));
	for (area_index = 0; area_index < area_count; ++area_index){
		BOUNDARY_SEGMENT segments[MAX_BOUNDARY_SEGMENTS];
		LONGLONG boundary = GetSideOfRect(side,&areas[area_index]);
		if (ExposedSegments(area_index,side,segments) != 0){
			boundary += isLargerSide(side) ? -SnapFrame_GetSpacing(side) : SnapFrame_GetSpacing(side);
			AddCoordinate(coordinates,&count,boundary);
		}
		/* A centered resize can hit the opposite exposed border first. Reflect
		   that border into a dragged-edge candidate around the unchanged center. */
		if (centering && ExposedSegments(area_index,opposite,segments) != 0){
			boundary = GetSideOfRect(opposite,&areas[area_index]);
			boundary += isLargerSide(opposite) ? -SnapFrame_GetSpacing(opposite) : SnapFrame_GetSpacing(opposite);
			AddCoordinate(coordinates,&count,center_sum-boundary);
		}
	}
	return count;
}

BOOL InTests_FindNearestResize(const RECT * desired, enum SIDE vertical_side, enum SIDE horizontal_side, BOOL centering, RECT * placement){
	LONG xs[MAX_PLACEMENT_COORDINATES], ys[MAX_PLACEMENT_COORDINATES];
	int xi, yi, x_count, y_count;
	LONGLONG best_distance = LLONG_MAX;
	BOOL found = FALSE;
	if (InTests_RectInDesktop(desired)) { *placement = *desired; return TRUE; }
	x_count = ResizeCoordinates(desired,horizontal_side,centering,xs);
	y_count = ResizeCoordinates(desired,vertical_side,centering,ys);
	for (xi = 0; xi < x_count; ++xi){
		for (yi = 0; yi < y_count; ++yi){
			RECT candidate = *desired;
			LONGLONG distance = 0;
			if (horizontal_side != SIDE_NONE){
				LONGLONG opposite = (LONGLONG)desired->left + desired->right - xs[xi];
				if (centering && (opposite < LONG_MIN || opposite > LONG_MAX)) continue;
				SetSideOfRect(horizontal_side,xs[xi],&candidate);
				if (centering) SetSideOfRect(OppositeSide(horizontal_side),(LONG)opposite,&candidate);
				distance += AbsoluteDistance(xs[xi],GetSideOfRect(horizontal_side,desired));
			}
			if (vertical_side != SIDE_NONE){
				LONGLONG opposite = (LONGLONG)desired->top + desired->bottom - ys[yi];
				if (centering && (opposite < LONG_MIN || opposite > LONG_MAX)) continue;
				SetSideOfRect(vertical_side,ys[yi],&candidate);
				if (centering) SetSideOfRect(OppositeSide(vertical_side),(LONG)opposite,&candidate);
				distance += AbsoluteDistance(ys[yi],GetSideOfRect(vertical_side,desired));
			}
			if (distance >= best_distance) continue;
			if (InTests_RectInDesktop(&candidate)){
				*placement = candidate;
				best_distance = distance;
				found = TRUE;
			}
		}
	}
	return found;
}
