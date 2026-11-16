//snap_Crop.c
// Visible-frame, region, and manual cropping for move/resize geometry.

#include "stdafx.h"
#include "snap_Crop.h"
#include "snap_lib_internal.h"
#include "sides.h"
#include "snap_MultiSz.h"
#include "snap_WinRects.h"
#include "snap_frame.h"

// Cropping offsets captured when resizing starts.
CROP_SIZING_INFO g_sizing_crop_info = {0};

BOOL IsSkinnedAppClass(HWND our_hWnd);
void CropToRgn( LPCRECT pRgnBox, LPRECT pRect);
void UnCropSideResults(PCROP_INFO p_crop,SIDE_SNAP_RESULTS * p_ssnap);
void CropTop(HWND hwnd,PCROP_INFO p_crop,LPRECT pTempRect);
void CropRgn(HWND hwnd,PCROP_INFO p_crop,LPRECT pTempRect);

BOOL isTopCroppable(HWND hwnd){
	return (!IsSkinnedAppClass(hwnd) && isCroppingTop());
}

LPCRECT Crop_GetPCroppedRect(PCROP_INFO p_crop){
	return &(p_crop->cropped_rect);
}
LPCRECT Crop_GetPUncroppedRect(PCROP_INFO p_crop){
	return &(p_crop->uncropped_rect);
}


void CropTop(HWND hWnd,PCROP_INFO p_crop,LPRECT pTempRect){
	p_crop->cropping_top = FALSE;
	if (isTopCroppable(hWnd) && pTempRect->bottom > pTempRect->top){
		/* Manual crop is an absolute minimum after frame/region cropping.
		   Keep one visible row and add in wide arithmetic before clamping. */
		LONGLONG desired_top = (LONGLONG)p_crop->uncropped_rect.top + getCropTop();
		LONG effective_top = (LONG)min(desired_top,(LONGLONG)pTempRect->bottom - 1);
		if (effective_top > pTempRect->top){
			pTempRect->top = effective_top;
			p_crop->cropping_top = TRUE;
		}
	}
}

void CropRgn(HWND hWnd,PCROP_INFO p_crop,LPRECT pTempRect){
	RECT rcRgn;

	if (isCroppingRgn()){
		p_crop->has_rgn = WinRects_GetRgnBox(hWnd,&rcRgn);

		if (p_crop->has_rgn){
			p_crop->rcRgn = rcRgn;
				
			//DBG_MSG_PRECT(g_hWnd_app,DBGMSG_FOUND_HRGN,(&rcRgn));
			CropToRgn(&rcRgn,pTempRect);
		}
	}
	else{
		p_crop->has_rgn = FALSE;
	}
}


void InitializeCropInfo(PCROP_INFO p_crop,LPCRECT pRect){
	int width = pRect->right - pRect->left;
	int height = pRect->bottom - pRect->top;
	RECT blank_rgn = {0,0,width,height};
    
    p_crop->cropped_rect = *pRect;
	p_crop->uncropped_rect = *pRect;
	p_crop->has_rgn = FALSE;
	p_crop->rcRgn = blank_rgn;
	p_crop->cropping_top = FALSE;
}

void Crop_LoadMovingCropInfo(PCROP_INFO p_crop,HWND hWnd,LPCRECT pRect){
	RECT temp_rect = *pRect;
	
	InitializeCropInfo(p_crop,pRect);

	//THIS MUST HAPPEN FIRST (maybe only/all windows with rgns don't need croptop)
	CropRgn(hWnd,p_crop,&temp_rect);
	{
		RECT offsets;
		if (SnapFrame_GetInsets(hWnd,pRect,&offsets)){
			temp_rect.left = max(temp_rect.left,pRect->left + offsets.left);
			temp_rect.top = max(temp_rect.top,pRect->top + offsets.top);
			temp_rect.right = min(temp_rect.right,pRect->right + offsets.right);
			temp_rect.bottom = min(temp_rect.bottom,pRect->bottom + offsets.bottom);
		}
	}
	CropTop(hWnd,p_crop,&temp_rect);

	p_crop->cropped_rect = temp_rect;
}

void Crop_offset_sizing_side(enum SIDE the_side,RECT * p_temp_rect){
	if (the_side != SIDE_NONE){
		SetSideOfRect(
			the_side,
			(	GetSideOfRect(the_side,p_temp_rect) 
				+ GetSideOfRect(the_side,&(g_sizing_crop_info.offsets))
			),
			p_temp_rect);
	}
}

void Crop_CropSizingRect(
	RECT const * prect,
	enum SIDE v_side,
	enum SIDE h_side,
	RECT * p_cropped_rect
)
{
	RECT temp_rect = * prect;

	if (g_sizing_crop_info.has_rgn){
		Crop_offset_sizing_side(v_side,&temp_rect);
		Crop_offset_sizing_side(h_side,&temp_rect);
	}

	*p_cropped_rect = temp_rect;
}

void Crop_CropSizingFixedEdges(RECT * rectangle, enum SIDE vertical_side, enum SIDE horizontal_side, BOOL centering){
	enum SIDE sides[4] = {SIDE_LEFT,SIDE_RIGHT,SIDE_TOP,SIDE_BOTTOM};
	int index;
	if (!g_sizing_crop_info.has_rgn) return;
	for (index = 0; index < 4; ++index){
		enum SIDE side = sides[index];
		enum SIDE opposite = OppositeSide(side);
		int offset = GetSideOfRect(side,&g_sizing_crop_info.offsets);
		if (side == vertical_side || side == horizontal_side) continue;
		/* Full-rectangle confinement needs visible fixed edges too. For a
		   centered opposite edge, CenterSize_Adjust has already reflected the
		   dragged inset around the outer center: add BOTH signed offsets to
		   match final uncrop followed by outer-center reflection. */
		if (centering && (opposite == vertical_side || opposite == horizontal_side))
			offset += GetSideOfRect(opposite,&g_sizing_crop_info.offsets);
		SetSideOfRect(side,GetSideOfRect(side,rectangle)+offset,rectangle);
	}
}


void Crop_LoadSizingInfo(HWND hwnd){
	RECT outer;
	CROP_INFO crop;
	g_sizing_crop_info.has_rgn = FALSE;
	SetRectEmpty(&g_sizing_crop_info.offsets);
	if (!GetWindowRect(hwnd,&outer)){
		return;
	}
	/* Moving and resizing must use the same visible bounds and signed offsets. */
	Crop_LoadMovingCropInfo(&crop,hwnd,&outer);
	g_sizing_crop_info.offsets.left = crop.cropped_rect.left - outer.left;
	g_sizing_crop_info.offsets.top = crop.cropped_rect.top - outer.top;
	g_sizing_crop_info.offsets.right = crop.cropped_rect.right - outer.right;
	g_sizing_crop_info.offsets.bottom = crop.cropped_rect.bottom - outer.bottom;
	g_sizing_crop_info.has_rgn = g_sizing_crop_info.offsets.left != 0
		|| g_sizing_crop_info.offsets.top != 0 || g_sizing_crop_info.offsets.right != 0
		|| g_sizing_crop_info.offsets.bottom != 0;
}
void UnCropSizingSideResults(SIDE_SNAP_RESULTS * p_ssnap){
	enum SIDE side = p_ssnap->side;

	if (side != SIDE_NONE){
		int offset = GetSideOfRect(side,&(g_sizing_crop_info.offsets));
		DBG_MSG_SIDE_VAL(g_hWnd_app,DBGMSG_ADJUST_HRGN,side,offset);
		p_ssnap->value -= offset;
	}
}
void Crop_UnCropSizingResults(SNAP_RESULTS * p_snap){
	if (g_sizing_crop_info.has_rgn){
        UnCropSizingSideResults(&(p_snap->h));
		UnCropSizingSideResults(&(p_snap->v));
	}
}

void UnCropSideResults(PCROP_INFO p_crop,SIDE_SNAP_RESULTS * p_ssnap){
	enum SIDE side = p_ssnap->side;

	if (side != SIDE_NONE){
		int offset = GetSideOfRect(side,&(p_crop->cropped_rect)) -
					 GetSideOfRect(side,&(p_crop->uncropped_rect));
		DBG_MSG_SIDE_VAL(g_hWnd_app,DBGMSG_ADJUST_HRGN,side,offset);
		p_ssnap->value -= offset;
	}
}


void Crop_UnCropMovingResults(PCROP_INFO p_crop,SNAP_RESULTS * p_snap){
	/* A zero offset is a no-op; DWM frame adjustments also require conversion. */
	UnCropSideResults(p_crop,&(p_snap->h));
	UnCropSideResults(p_crop,&(p_snap->v));
}

void CropToRgn( LPCRECT pRgnBox, LPRECT pRect){
	int offsetLeft  = pRgnBox->left;
	int offsetTop	= pRgnBox->top;

	int width = pRect->right   - pRect->left;
	int height= pRect->bottom  - pRect->top;

	int offsetRight = width - pRgnBox->right;
	int offsetBottom = height - pRgnBox->bottom;

	//???
	pRect->left		+= max(offsetLeft,0);
	pRect->top		+= max(offsetTop,0);

	pRect->right	-= max(offsetRight,0);
	pRect->bottom	-= max(offsetBottom,0);
}

BOOL IsSkinnedAppClass(HWND our_hWnd){
	return is_window_class_in_classlist(our_hWnd,getSkinnedClasses());
}
