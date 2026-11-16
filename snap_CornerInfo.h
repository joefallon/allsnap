#ifndef SNAP_CORNER_INFO_INCLUDE
#define SNAP_CORNER_INFO_INCLUDE

#include <windef.h>
#include "sides.h"

typedef struct CORNER_INFO_TAG{
	BOOL topLeft_out;	
	BOOL topRight_out;	
	BOOL bottomLeft_out;	
	BOOL bottomRight_out;
} CORNER_INFO, * P_CORNER_INFO;

BOOL isCornerOut (P_CORNER_INFO pci, enum SIDE v_side, enum SIDE h_side);
BOOL isSideOut	 (P_CORNER_INFO pci, enum SIDE side);
BOOL isSideIn	 (P_CORNER_INFO pci, enum SIDE side);
BOOL isOnlyOneCornerOfSideOut (P_CORNER_INFO pci, enum SIDE side);


void updateCornerInfo(P_CORNER_INFO pci,LPCRECT pRect);



#endif