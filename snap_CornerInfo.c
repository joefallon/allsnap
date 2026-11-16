#include "stdafx.h"
#include "snap_CornerInfo.h"

BOOL isCornerOut (P_CORNER_INFO pci, enum SIDE v_side, enum SIDE h_side){
	if (v_side == SIDE_TOP){
		
		if (h_side == SIDE_RIGHT){
			return pci->topRight_out;
		}
		else{
			return pci->topLeft_out;
		}

	}
	else{ // (v_side == SIDE_BOTTOM)
		
		if (h_side == SIDE_RIGHT){
			return pci->bottomRight_out;
		}
		else{
			return pci->bottomLeft_out;
		}
	}
}

BOOL isSideOut (P_CORNER_INFO pci, enum SIDE side){
	enum SIDE adj1 = AdjacentSide(side);
	enum SIDE adj2 = OppositeSide(adj1);

	if (isVerticalSide(side)){
		return isCornerOut(pci,side,adj1) &&
			   isCornerOut(pci,side,adj2);
	}
	else{ 
		return isCornerOut(pci,adj1,side) &&
			   isCornerOut(pci,adj2,side);
	}
}

BOOL isSideIn (P_CORNER_INFO pci, enum SIDE side){
	enum SIDE adj1 = AdjacentSide(side);
	enum SIDE adj2 = OppositeSide(AdjacentSide(side));

	if (isVerticalSide(side)){
		return (!isCornerOut(pci,side,adj1))
			&& (!isCornerOut(pci,side,adj2));
	}
	else{ 
		return (!isCornerOut(pci,adj1,side))
			&& (!isCornerOut(pci,adj2,side));
	}
}

#define XOR(x,y) (((x)||(y))&&!((x)&&(y)))

BOOL isOnlyOneCornerOfSideOut (P_CORNER_INFO pci, enum SIDE side){
	enum SIDE adj1 = AdjacentSide(side);
	enum SIDE adj2 = OppositeSide(adj1);

	if (isVerticalSide(side)){
		return XOR(isCornerOut(pci,side,adj1),isCornerOut(pci,side,adj2));
	}
	else{ 
		return XOR(isCornerOut(pci,adj1,side),isCornerOut(pci,adj2,side));
	}
}