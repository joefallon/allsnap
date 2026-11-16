#include "stdafx.h"
#include <windowsx.h>
#include <tchar.h>
#include <limits.h>


#include <Prsht.h>
#include <commctrl.h>
#include "snap_tips.h"

#ifndef ALLSNAP_TESTING
#define COMPILE_MULTIMON_STUBS
#endif
#include <Multimon.h>

#include "resource.h"
#include "snapper.h"
#include "snap_lib.h"
#include "snap_settings.h"
#include "snap_taskbar.h"
#include "snap_App.h"
#include "ctxhelp.h"
#include "snap_ini.h"
#ifdef ALLSNAP_TESTING
int SnapSettings_TestMessageBox(HWND hwnd, LPCTSTR text, LPCTSTR title, UINT type);
#undef MessageBox
#define MessageBox SnapSettings_TestMessageBox

static INT_PTR (CALLBACK *test_create_property_sheet)(LPCPROPSHEETHEADER);
void SnapSettings_TestSetSheetCreator(INT_PTR (CALLBACK *creator)(LPCPROPSHEETHEADER)){
	test_create_property_sheet=creator;
}
#endif

static BOOL SaveAppliedSettings(HWND page){
	if (SaveSettingsToINI()){
		SetWindowLongPtr(page,DWLP_MSGRESULT,PSNRET_NOERROR);
		return TRUE;
	}
	MessageBox(page,_T("Settings could not be saved. Check that allsnap.ini is writable, then apply again."),
		_T("allSnap - Settings not saved"),MB_OK|MB_ICONERROR);
	SetWindowLongPtr(page,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
	return FALSE;
}

/*====================================================================
Globals
=====================================================================*/

HWND		g_hwndPropSheet;					//PropertySheet Window

static const int g_num_toggle_keys=4;
static CB_VK_ITEM g_aToggleKeys[] =	{	
	{VK_MENU,	 _T("Alt")	},			
	{VK_CONTROL, _T("Ctrl")},			
	{VK_SHIFT,	 _T("Shift")},			
	{VK_TAB,	 _T("Tab")	}
						};

static int HGRID_RADIOS[] = {
	IDC_RADIO_H_EVEN,
	IDC_RADIO_H_PIXELS,
	IDC_RADIO_H_SINGLE
};
static int VGRID_RADIOS[] = {
	IDC_RADIO_V_EVEN,
	IDC_RADIO_V_PIXELS,
	IDC_RADIO_V_SINGLE
};
static int HGRID_EDITS[] =
{
	IDC_H_NUM_EVEN,
	IDC_H_PIXELS_BTWN,
	IDC_H_PIXELS_FROM_TOP
};
static int VGRID_EDITS[] =
{
	IDC_V_NUM_EVEN,
	IDC_V_PIXELS_BTWN,
	IDC_V_PIXELS_FROM_LEFT
};

static int DEFAULT_GRID_VALS[] = {1,2,0};

static const int NUM_HGRID_EDITS = sizeof(HGRID_EDITS)/sizeof(HGRID_EDITS[0]);
static const int NUM_VGRID_EDITS = sizeof(VGRID_EDITS)/sizeof(VGRID_EDITS[0]);
static const int NUM_HGRID_RADIOS = sizeof(HGRID_RADIOS)/sizeof(HGRID_RADIOS[0]);
static const int NUM_VGRID_RADIOS = sizeof(VGRID_RADIOS)/sizeof(VGRID_RADIOS[0]);

static SNAP_TYPE_BOX snap_type_boxes[]={
	{IDC_DESKTOP,SNAPT_DESKTOP},
	{IDC_SELF,SNAPT_SELF},
	{IDC_OTHERS,SNAPT_OTHERS},
  {IDC_VCENTER,SNAPT_VCENTER},
  {IDC_HCENTER,SNAPT_HCENTER}};

static const int num_snap_type_boxes = sizeof(snap_type_boxes)/sizeof(snap_type_boxes[0]);
static UINT	thresh_boxes[] = {IDC_WIN_THRESH,IDC_SCREEN_THRESH,0};

/*====================================================================
 Local Function Prototypes
=====================================================================*/
void	CALLBACK PropSheetCallback(HWND hwndPropSheet, UINT uMsg, LPARAM lParam);

INT_PTR CALLBACK PrefsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AdvProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK GridProc(HWND,UINT,WPARAM,LPARAM);

void ValidatePrefs(HWND hDlg);
BOOL ValidateAdv(HWND hDlg);
BOOL ValidateGrid(HWND hDlg);

LRESULT Prefs_OnInitDialog	(HWND hwnd, HWND hwndFocus, LPARAM lParam);
LRESULT Adv_OnInitDialog	(HWND hwnd, HWND hwndFocus, LPARAM lParam);
LRESULT Grid_OnInitDialog	(HWND hwnd, HWND hwndFocus, LPARAM lParam);

BOOL	OnContextMenu(HWND hWnd, HWND hwndCtl, UINT x, UINT y);
BOOL	Prefs_OnCommand		(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
BOOL	Adv_OnCommand		(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
BOOL	Grid_OnCommand		(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

void Grid_UpdateEnabledDim(	HWND hDlg,	int check_id,const int radios[],const int edits[]);

void	InitGeneral(HWND hDlg);
void	InitSnapTo(HWND hDlg);

void	InitToggleKeys(HWND hCbCtl,CB_VK_ITEM toggleKeys[], int num_toggle_keys,UINT selected_key);
void	InitSnapTypeBoxes(HWND hDlg,UINT snap_type);

void	ApplyPrefs(HWND hDlg);
void	ApplySnapTo(HWND hDlg);
void	ApplyGrid(HWND hDlg);
UINT	getNewSnapType(HWND hDlg);

void	ApplyThresh(HWND hDlg);
void	ApplyToggleKey(HWND hDlg);


void	ApplyAdv(HWND hDlg);


UINT	getSelectedToggleKey	(HWND hDlg);

void CenterWindow(HWND hwnd);

static void InitPropertyPage(PROPSHEETPAGE * page, UINT template_id,
	DLGPROC dialog_proc, LPCTSTR title){
	page->dwSize = sizeof(PROPSHEETPAGE);
	page->dwFlags = PSP_USETITLE;
	page->hInstance = g_hInst;
	page->pszTemplate = MAKEINTRESOURCE(template_id);
	page->pfnDlgProc = dialog_proc;
	page->pszTitle = title;
	page->lParam = 0;
	page->pfnCallback = NULL;
}

// DoPropertySheet - creates a property sheet that contains three pages.
// hwndOwner - handle to the owner window of the property sheet.
//
// Global variables
//     g_hInst - instance handle
HWND DoPropertySheet(HWND hwndOwner)
{
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;

	InitCmnCtls();
	LoadSettingsForDialog();

	InitPropertyPage(&psp[0],IDD_GENERAL,PrefsProc,_T("Preferences"));
	InitPropertyPage(&psp[1],IDD_GRID,GridProc,_T("Snap to grid"));
	InitPropertyPage(&psp[2],IDD_ADVANCED,AdvProc,_T("Advanced"));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_MODELESS | PSH_USECALLBACK;
    psh.hwndParent = hwndOwner;
	psh.pszIcon	= MAKEINTRESOURCE(IDI_SNAPIT);
    psh.hInstance = g_hInst;
    psh.pszCaption = (LPTSTR) _T("allSnap Properties");
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = (PFNPROPSHEETCALLBACK)PropSheetCallback;

#ifdef ALLSNAP_TESTING
	if (test_create_property_sheet)
		return (HWND)test_create_property_sheet(&psh);
#endif
	return (HWND)PropertySheet(&psh);
}
 
BOOL	OnContextMenu(HWND hWnd, HWND hwndCtl, UINT x, UINT y){
	DWORD  context_id;
	HMENU hContextMenu;

	if (hwndCtl == NULL) return FALSE;
	context_id = GetWindowContextHelpId(hwndCtl);
	if (context_id == 0) return FALSE;

	hContextMenu = GetSubMenu(LoadMenu(g_hInst,MAKEINTRESOURCE(IDR_WHAT)),0);
	if (TrackPopupMenu(hContextMenu,
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
		x,y,0,hWnd,NULL)){
		tips_show(hwndCtl);
	}
	return TRUE;
}

void CALLBACK PropSheetCallback(HWND hwndPropSheet, UINT uMsg, LPARAM lParam)
{
	switch(uMsg){
		//called before the dialog is created, hwndPropSheet = NULL, lParam points to dialog resource
		case PSCB_PRECREATE:
			{
				LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;

				if(!(lpTemplate->style & DS_CENTER)){
					lpTemplate->style |= DS_CENTER;
				}
			}
			break;
		case PSCB_INITIALIZED:
			SetClassLongPtr(hwndPropSheet,GCLP_HICON,(LONG_PTR)LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_SNAPIT)));
			break;


	}
}

// Mesage handler for Preferences Page.
INT_PTR CALLBACK PrefsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR lpnmhdr;


	switch (message)
	{
		HANDLE_MSG(hDlg,WM_CONTEXTMENU,OnContextMenu);
		HANDLE_MSG(hDlg,WM_INITDIALOG,Prefs_OnInitDialog);
		HANDLE_MSG(hDlg,WM_COMMAND,Prefs_OnCommand);

		case WM_HELP:
			tips_show(((LPHELPINFO)lParam)->hItemHandle);
		
		case WM_NOTIFY:
			lpnmhdr = (NMHDR FAR *)lParam;

			switch (lpnmhdr->code)
				{
				case PSN_APPLY:   //sent when OK or Apply button pressed
					ApplyPrefs(hDlg);
					SaveAppliedSettings(hDlg);
					return TRUE;

				case PSN_RESET:   //sent when Cancel button pressed
					break;

				case PSN_KILLACTIVE:
					ValidatePrefs(hDlg);
					return TRUE;

				default:
					break;
				}
			break;
	}

	return (FALSE);
}


INT_PTR CALLBACK AdvProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnmhdr;
	
	switch (message)
	{
		HANDLE_MSG(hDlg,WM_INITDIALOG,Adv_OnInitDialog);
		HANDLE_MSG(hDlg,WM_COMMAND,Adv_OnCommand);
		HANDLE_MSG(hDlg,WM_CONTEXTMENU,OnContextMenu);
					
		case WM_HELP:
			tips_show(((LPHELPINFO)lParam)->hItemHandle);
			break;
		case WM_NOTIFY:
			lpnmhdr = (NMHDR FAR *)lParam;

			switch (lpnmhdr->code)
				{
				case PSN_APPLY:   //sent when OK or Apply button pressed
					if (!ValidateAdv(hDlg)){
						SetWindowLongPtr(hDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
						return TRUE;
					}
					ApplyAdv(hDlg);
					SaveAppliedSettings(hDlg);
					return TRUE;
				case PSN_RESET: 
					break;
				
				case PSN_KILLACTIVE:
					ValidateAdv(hDlg);
					return TRUE;

				default:
					break;
				}
			break;
	}

	return (FALSE);
}


INT_PTR CALLBACK GridProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnmhdr;
	
	switch (message)
	{
		HANDLE_MSG(hDlg,WM_INITDIALOG,Grid_OnInitDialog);
		HANDLE_MSG(hDlg,WM_COMMAND,Grid_OnCommand);
		HANDLE_MSG(hDlg,WM_CONTEXTMENU,OnContextMenu);
					
		case WM_HELP:
			tips_show(((LPHELPINFO)lParam)->hItemHandle);
			break;
		case WM_NOTIFY:
			lpnmhdr = (NMHDR FAR *)lParam;

			switch (lpnmhdr->code)
				{
				case PSN_APPLY:   //sent when OK or Apply button pressed
					if (!ValidateGrid(hDlg)){
						SetWindowLongPtr(hDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
						return TRUE;
					}
					ApplyGrid(hDlg);
					SaveAppliedSettings(hDlg);
					return TRUE;
				case PSN_RESET: 
					break;
				
				case PSN_KILLACTIVE:
					ValidateGrid(hDlg);
					return TRUE;

				default:
					break;
				}
			break;
	}

	return (FALSE);
}

void ValidatePrefs(HWND hDlg){
	UINT	box_id;
	int		i=0;

	while(box_id = thresh_boxes[i++]){
		BOOL fOK = TRUE;
		int entered_thresh 
			= GetDlgItemInt(hDlg,box_id,&fOK,FALSE);

		if (!fOK 
		|| !(  (entered_thresh		>= MIN_THRESH )	
			&& (entered_thresh	<= MAX_THRESH))
	
		){
			TCHAR	szError[MAX_LOADSTRING];
			TCHAR	szTitle[MAX_LOADSTRING];

			LoadString(g_hInst,IDS_APP_TITLE,		szTitle,MAX_LOADSTRING);
			LoadString(g_hInst,IDS_THRESHOLD_ERROR,	szError,MAX_LOADSTRING);

            MessageBox(hDlg,szError,szTitle,MB_OK|MB_ICONEXCLAMATION);
			Edit_SetSel(GetDlgItem(hDlg,box_id),0,-1L);
			SetFocus(GetDlgItem(hDlg,box_id));
			SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);

			return;
		}	
	}

	{
		BOOL valid;
		UINT spacing = GetDlgItemInt(hDlg,IDC_WINDOW_SPACING,&valid,FALSE);
		if (!valid || spacing > 1000){
			MessageBox(hDlg,_T("Enter a spacing from 0 to 1000 pixels."),
				_T("allSnap - Window spacing"),MB_OK|MB_ICONEXCLAMATION);
			SetFocus(GetDlgItem(hDlg,IDC_WINDOW_SPACING));
			SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
			return;
		}
	}
	SetWindowLongPtr(hDlg,DWLP_MSGRESULT,FALSE);
}

static BOOL ValidateInteger(HWND page,int control,int minimum,int maximum,LPCTSTR message){
	BOOL valid;
	int value = (int)GetDlgItemInt(page,control,&valid,TRUE);
	if (valid && value >= minimum && value <= maximum)
		return TRUE;
	MessageBox(page,message,_T("allSnap - Invalid setting"),MB_OK|MB_ICONEXCLAMATION);
	Edit_SetSel(GetDlgItem(page,control),0,-1);
	SetFocus(GetDlgItem(page,control));
	SetWindowLongPtr(page,DWLP_MSGRESULT,TRUE);
	return FALSE;
}

BOOL ValidateAdv(HWND page){
	if (Is_Checked(page,IDC_CROPTOPC) && !ValidateInteger(page,IDC_CROPTOP,0,INT_MAX,
		_T("Enter a whole number from 0 to 2147483647 for top cropping.")))
		return FALSE;
	SetWindowLongPtr(page,DWLP_MSGRESULT,FALSE);
	return TRUE;
}

static BOOL ValidateGridDimension(HWND page,int enabled,const int radios[],const int edits[]){
	int mode;
	if (!Is_Checked(page,enabled))
		return TRUE;
	for (mode=0;mode<3;mode++){
		if (Is_Checked(page,radios[mode])){
			return ValidateInteger(page,edits[mode],mode==GRIDTYPE_PIXELS ? 1 : 0,
				mode==GRIDTYPE_EVEN ? 100 : INT_MAX,
				mode==GRIDTYPE_EVEN ? _T("Enter from 0 to 100 evenly spaced grid lines.") :
				mode==GRIDTYPE_PIXELS ? _T("Enter a whole pixel spacing from 1 to 2147483647.") :
				_T("Enter a whole grid offset from 0 to 2147483647 pixels."));
		}
	}
	MessageBox(page,_T("Select a grid mode."),_T("allSnap - Invalid setting"),MB_OK|MB_ICONEXCLAMATION);
	SetFocus(GetDlgItem(page,radios[0]));
	SetWindowLongPtr(page,DWLP_MSGRESULT,TRUE);
	return FALSE;
}

BOOL ValidateGrid(HWND page){
	if (!ValidateGridDimension(page,IDC_CHECK_HORIZONTAL,HGRID_RADIOS,HGRID_EDITS) ||
		!ValidateGridDimension(page,IDC_CHECK_VERTICAL,VGRID_RADIOS,VGRID_EDITS))
		return FALSE;
	SetWindowLongPtr(page,DWLP_MSGRESULT,FALSE);
	return TRUE;
}
/*******************************************************************************************
	INIT functions
*******************************************************************************************/
LRESULT Adv_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam){
	BOOL cropping_top = isCroppingTop();

#ifdef ASNAP_AUTO_LINEUP
	Set_Checked(hDlg,IDC_AUTOLINEUP,IsAutolineupOn());

	EnableWindow(GetDlgItem(hDlg,IDC_AUTOLINEUP),!g_isXP);

#else
	ShowWindow(GetDlgItem(hDlg,IDC_AUTOLINEUP),SW_HIDE);
#endif

	Set_Checked(hDlg,IDC_INVTOGGLE,!isDisableToggle());
	Set_Checked(hDlg,IDC_SNAPMDI,isSnapMdi());
	Set_Checked(hDlg,IDC_INSIDES,!isSnappingInsides());
	Set_Checked(hDlg,IDC_HIDEICON,isIconHidden());
	
	SetDlgItemInt(hDlg,IDC_CROPTOP,getCropTop(),FALSE);
	Edit_SetSel(GetDlgItem(hDlg,IDC_CROPTOP),0,-1);
	EnableWindow(GetDlgItem(hDlg,IDC_CROPTOP),cropping_top);
	Set_Checked(hDlg,IDC_CROPTOPC,cropping_top);
	Set_Checked(hDlg,IDC_CROP_RGN,!isCroppingRgn());

	Set_Checked(hDlg,IDC_KEPT,isKeptToScreen());

	return TRUE;
	
}


/*******************************************************************************************
	INIT functions
*******************************************************************************************/
void Grid_UpdateEnabledDim
(
	HWND hDlg,
	int check_id,
	const int radios[],
	const int edits[]
){
	int i;
	BOOL checked = Is_Checked(hDlg,check_id);
	for (i=0;i<3;i++){
		EnableWindow(GetDlgItem(hDlg,edits[i]),checked);
		EnableWindow(GetDlgItem(hDlg,radios[i]),checked);
	}
}

void Grid_InitDim
(
	HWND hDlg,
	int check_id,
	const int radios[],
	const int edits[],
    gridsnap_setting_t const * p_setting
){
	int i;
	for (i=0;i<3;i++){
		if (p_setting->type != i){
			SetDlgItemInt(
				hDlg,
				edits[i],
				DEFAULT_GRID_VALS[i],
				FALSE
			);
		}
	}

	Set_Checked(hDlg,check_id,p_setting->enabled);
	Set_Checked(hDlg,radios[p_setting->type],TRUE);
	SetDlgItemInt(hDlg,edits[p_setting->type],p_setting->val,FALSE);
	Grid_UpdateEnabledDim(hDlg,check_id,radios,edits);
}

LRESULT Grid_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam){
	gridsnap_settings_t settings = {0};
    getGridSnap(&settings);
	Grid_InitDim(hDlg,IDC_CHECK_HORIZONTAL,HGRID_RADIOS,HGRID_EDITS,&(settings.h));
	Grid_InitDim(hDlg,IDC_CHECK_VERTICAL,VGRID_RADIOS,VGRID_EDITS,&(settings.v));

	return TRUE;	
}

LRESULT Prefs_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam){
	CenterWindow(GetParent(hDlg));
	InitGeneral(hDlg);
	InitSnapTo(hDlg);
	return TRUE;
}





void InitGeneral(HWND hDlg){
	UINT	new_toggle_key		= getToggleKey();

	SetDlgItemInt(hDlg,IDC_WIN_THRESH,getWinThresh(),FALSE);
	SetDlgItemInt(hDlg,IDC_SCREEN_THRESH,getScreenThresh(),FALSE);
	SetDlgItemInt(hDlg,IDC_WINDOW_SPACING,getWindowSpacing(),FALSE);
	SendDlgItemMessage(hDlg,IDC_WINDOW_SPACING,EM_SETLIMITTEXT,4,0);
	
	Edit_SetSel(GetDlgItem(hDlg,IDC_WIN_THRESH),0,-1);
	Edit_SetSel(GetDlgItem(hDlg,IDC_SCREEN_THRESH),0,-1);

	SendDlgItemMessage(
		hDlg,
		IDC_WIN_THRESH_SPIN,
		UDM_SETRANGE,
        (WPARAM)0,
        (LPARAM) MAKELONG((short) MAX_THRESH, (short) MIN_THRESH));

	SendDlgItemMessage(
		hDlg,
		IDC_SCREEN_THRESH_SPIN,
		UDM_SETRANGE,
        (WPARAM)0,
        (LPARAM) MAKELONG((short) MAX_THRESH, (short) MIN_THRESH));


	InitToggleKeys(GetDlgItem(hDlg,IDC_TOGGLEKEYS),
		g_aToggleKeys,g_num_toggle_keys,
		new_toggle_key);

}


void InitSnapTo(HWND hDlg){
	BOOL new_is_enabled	= isEnabled();
	UINT new_snap_type  = getSnapType();
	

	Set_Checked(hDlg,IDC_ENABLED,new_is_enabled);

	InitSnapTypeBoxes(hDlg,new_snap_type);
}




void	InitToggleKeys(HWND hCbCtl,CB_VK_ITEM toggleKeys[], int num_toggle_keys,UINT selected_key){
	int i;
	int select_index=0;
	
	ComboBox_ResetContent(hCbCtl);
	
	for (i = 0; i<num_toggle_keys; i++ ){		
		ComboBox_AddString(hCbCtl,toggleKeys[i].string);
		ComboBox_SetItemData(hCbCtl,i,toggleKeys[i].data);

		if(toggleKeys[i].data == selected_key){
			select_index=i;
		}
	}
	ComboBox_SelectString(hCbCtl, 0, toggleKeys[select_index].string);
}

void InitSnapTypeBoxes(HWND hDlg,UINT snap_type){
	int i=0;

	for(i=0;i<num_snap_type_boxes;i++){
		Button_SetCheck(
			GetDlgItem(hDlg,snap_type_boxes[i].box_id),
			(((snap_type & snap_type_boxes[i].snap_type) == 0)?
				BST_UNCHECKED:BST_CHECKED));
	}
}

BOOL Prefs_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify){
	BOOL changed = FALSE;


	switch (id) {
		case IDC_TOGGLEKEYS:
			changed = (codeNotify == CBN_SELCHANGE);
			break;

		case IDC_SCREEN_THRESH:
		case IDC_WIN_THRESH:
		case IDC_WINDOW_SPACING:
			changed = (codeNotify == EN_UPDATE);
			break;

		case IDC_DESKTOP:
		case IDC_OTHERS:
		case IDC_SELF:
		case IDC_VCENTER:
		case IDC_HCENTER:
			changed = TRUE;
			break;

		case IDC_ENABLED:
			changed = TRUE;
			break;
	}

	if (changed){
		PropSheet_Changed(GetParent(hDlg),hDlg);
	}
	
	return FALSE;
}



BOOL Adv_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify){
	BOOL changed = FALSE;
	switch(id){
		case IDC_CROPTOP:
			changed = (codeNotify == EN_UPDATE);
			break;
		case IDC_CROPTOPC:
			EnableWindow (GetDlgItem(hDlg,IDC_CROPTOP),Is_Checked(hDlg,IDC_CROPTOPC));
		case IDC_SNAPMDI:
		case IDC_INVTOGGLE:
		case IDC_INSIDES:
		case IDC_AUTOLINEUP:
		case IDC_HIDEICON:
		case IDC_KEPT:
		case IDC_CROP_RGN:
			changed = TRUE;
			break;
	}
	if (changed){
		PropSheet_Changed(GetParent(hDlg),hDlg);
	}
	return FALSE;
}

BOOL in_array(const int array[],int size,int val){
	int i=0;
	for (i=0;i<size;i++){
		if (array[i] == val)
			return TRUE;
	}
	return FALSE;
}

BOOL Was_Checked(const int radios[],int num_radios,int id,UINT codeNotify){
	return in_array(radios,num_radios,id);
}
BOOL Was_Edited(const int edits[],int num_edits,int id,UINT codeNotify){
	return ( (codeNotify == EN_UPDATE)
		&& in_array(edits,num_edits,id)
	);
}

BOOL Grid_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify){
	BOOL changed = FALSE;
	if (	   
		   Was_Edited(HGRID_EDITS,NUM_HGRID_EDITS,id,codeNotify)
		|| Was_Checked(HGRID_RADIOS,NUM_HGRID_RADIOS,id,codeNotify)
		|| Was_Edited(VGRID_EDITS,NUM_VGRID_EDITS,id,codeNotify)
		|| Was_Checked(VGRID_RADIOS,NUM_VGRID_RADIOS,id,codeNotify)
	){
		changed = TRUE;
	}

	if ( id == IDC_CHECK_HORIZONTAL){
		Grid_UpdateEnabledDim(hDlg,IDC_CHECK_HORIZONTAL,HGRID_RADIOS,HGRID_EDITS);
		changed = TRUE;
	}

	if ( id == IDC_CHECK_VERTICAL){
		Grid_UpdateEnabledDim(hDlg,IDC_CHECK_VERTICAL,VGRID_RADIOS,VGRID_EDITS);
		changed = TRUE;
	}
	
	if (changed){
		PropSheet_Changed(GetParent(hDlg),hDlg);
	}
	return FALSE;
}

int find_checked_index(HWND hDlg,int array[],int size){
	int i =0;
	for (i=0;i<size;i++){
		if(Is_Checked(hDlg,array[i])){
			return i;
		}
	}
	return -1;
}

void get_grid_selections
(
	HWND hDlg,
	gridsnap_setting_t * p_setting,
	int enabled_id,
	int edits[],
	int radios[],
	int num_types
)
{
	int type_checked;
	BOOL ignored;

	if ( !Is_Checked(hDlg,enabled_id) ){
		p_setting->enabled = FALSE;
		return;
	}
	type_checked = find_checked_index(hDlg,radios,NUM_HGRID_RADIOS);
	p_setting->enabled = TRUE;
	if (type_checked == -1){
		return;
	}
	ignored = TRUE;
	p_setting->type = type_checked;
	p_setting->val = GetDlgItemInt(hDlg,edits[type_checked],&ignored,FALSE);
}

void ApplyGrid(HWND hDlg){
	gridsnap_settings_t new_grid_snap;
	getGridSnap(&new_grid_snap);
	get_grid_selections
	(
		hDlg,
		&(new_grid_snap.h),
		IDC_CHECK_HORIZONTAL,
		HGRID_EDITS,
		HGRID_RADIOS,
		NUM_HGRID_EDITS
	);
	
	get_grid_selections(
		hDlg,
		&(new_grid_snap.v),
		IDC_CHECK_VERTICAL,
		VGRID_EDITS,
		VGRID_RADIOS,
		NUM_VGRID_EDITS
	);
	setGridSnap(&new_grid_snap);
}

void ApplySnapTo(HWND hDlg){
	
	UINT new_snap_type = getNewSnapType(hDlg);
	BOOL is_enabled_checked = Is_Checked(hDlg,IDC_ENABLED);
    
	setSnapType(new_snap_type);

	setEnabled(is_enabled_checked);
	ResetTaskbarIcon();
}


UINT getNewSnapType(HWND hDlg){
	UINT  new_snap_type = SNAPT_NONE;
	int i=0;
	for(i=0;i<num_snap_type_boxes;i++){
		if (Is_Checked(hDlg,snap_type_boxes[i].box_id)){
			new_snap_type |= snap_type_boxes[i].snap_type;
		}
	}
	return new_snap_type;
}

INLINE void ApplyThresh(HWND hDlg){
	setWinThresh(	GetDlgItemInt(	hDlg,	IDC_WIN_THRESH,		NULL,FALSE));
	setScreenThresh(GetDlgItemInt(	hDlg,	IDC_SCREEN_THRESH,	NULL,FALSE));
}

INLINE void ApplyAdv(HWND hDlg){
	BOOL valid;
	int new_crop_val = (int)GetDlgItemInt(hDlg,IDC_CROPTOP,&valid,TRUE);
	if (!valid || new_crop_val < 0)
		new_crop_val = getCropTop();
	setSnapMdi			( Is_Checked(hDlg,IDC_SNAPMDI));
	setDisableToggle	(!Is_Checked(hDlg,IDC_INVTOGGLE));
	setSnappingInsides	(!Is_Checked(hDlg,IDC_INSIDES));
	setCropTop			(new_crop_val);
	setIconHidden		( Is_Checked(hDlg,IDC_HIDEICON));
	setCroppingTop		( Is_Checked(hDlg,IDC_CROPTOPC) && (new_crop_val != 0) );
	setKeptToScreen		( Is_Checked(hDlg,IDC_KEPT));
	setCroppingRgn			( !Is_Checked(hDlg,IDC_CROP_RGN));

	EnableWindow(
		GetDlgItem(hDlg,IDC_CROPTOP), 
		(getCropTop()!=0) && Is_Checked(hDlg,IDC_CROPTOPC)
	);

	Set_Checked(hDlg,IDC_CROPTOPC,isCroppingTop());
}

INLINE void ApplyToggleKey(HWND hDlg){
	setToggleKey(getSelectedToggleKey(hDlg));


}

INLINE void ApplyPrefs(HWND hDlg){
	ApplySnapTo(hDlg);
	ApplyThresh(hDlg);
	ApplyToggleKey(hDlg);
	setWindowSpacing((int)GetDlgItemInt(hDlg,IDC_WINDOW_SPACING,NULL,FALSE));
	
}






INLINE UINT getSelectedToggleKey(HWND hDlg){
	HWND hCtl = GetDlgItem(hDlg,IDC_TOGGLEKEYS);
	return (UINT)ComboBox_GetItemData(hCtl,ComboBox_GetCurSel(hCtl));
}

//this was borrowed from MFC CenterWindow
void CenterWindow(HWND hwnd){
	MONITORINFO mi;
	int xLeft,yTop;
	int Dlg_height,Dlg_width;

	RECT rcCenter,rcArea,rcDlg;

	GetWindowRect(hwnd,&rcDlg);
	Dlg_height=(rcDlg.bottom - rcDlg.top);
	Dlg_width=(rcDlg.right - rcDlg.left);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(
				MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
	rcCenter = mi.rcWork;
	rcArea = mi.rcWork;

	xLeft = (rcCenter.left + rcCenter.right) / 2 - Dlg_width / 2;
	yTop = (rcCenter.top + rcCenter.bottom) / 2 -  Dlg_height/ 2;

	// if the dialog is outside the screen, move it inside
	if (xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if (xLeft + Dlg_width > rcArea.right)
		xLeft = rcArea.right - Dlg_width;

	if (yTop < rcArea.top)
		yTop = rcArea.top;
	else if (yTop + Dlg_height > rcArea.bottom)
		yTop = rcArea.bottom - Dlg_height;

	SetWindowPos(hwnd, NULL,xLeft, yTop, -1, -1,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
