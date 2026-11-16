#include "stdafx.h"
#include "snap_testers.h"
#include "snap_TrackResults.h"

static SNAP_RESULTS g_last_result={{SIDE_NONE,0,0,0},{SIDE_NONE,0,0,0}};

PSNAP_RESULTS  TrackRslts_GetLast(void){
	return &g_last_result;
}


void	TrackRslts_Reset(void){
	clear_snap_results(&g_last_result);
}


void	TrackRslts_Track(PSNAP_RESULTS psnap_results){
	g_last_result = *psnap_results;
}
