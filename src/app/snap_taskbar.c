#include "stdafx.h"
#include <Shellapi.h>
#include "snap_taskbar.h"
#include "snap_App.h"
#include "snap_lib.h"
#include "snap_mywm_msg.h"

UINT g_uTaskbarRestart;
static BOOL g_isHidden = FALSE;

#ifdef ALLSNAP_TESTING
BOOL SnapTaskbar_TestNotifyIcon(DWORD operation, PNOTIFYICONDATA data);
static int (WINAPI * tray_load_string)(HINSTANCE,UINT,LPTSTR,int) = LoadString;
static HANDLE (WINAPI * tray_load_image)(HINSTANCE,LPCTSTR,UINT,int,int,UINT) = LoadImage;
static BOOL (WINAPI * tray_destroy_icon)(HICON) = DestroyIcon;
void SnapTaskbar_TestSetResourceOps(
	int (WINAPI * load_string)(HINSTANCE,UINT,LPTSTR,int),
	HANDLE (WINAPI * load_image)(HINSTANCE,LPCTSTR,UINT,int,int,UINT),
	BOOL (WINAPI * destroy_icon)(HICON)){
	tray_load_string = load_string ? load_string : LoadString;
	tray_load_image = load_image ? load_image : LoadImage;
	tray_destroy_icon = destroy_icon ? destroy_icon : DestroyIcon;
}
#undef Shell_NotifyIcon
#define Shell_NotifyIcon SnapTaskbar_TestNotifyIcon
#undef LoadString
#define LoadString tray_load_string
#undef LoadImage
#define LoadImage tray_load_image
#define DestroyIcon tray_destroy_icon
void SnapTaskbar_TestReset(void){
	g_isHidden = FALSE;
	g_uTaskbarRestart = 0;
	SnapTaskbar_TestSetResourceOps(NULL,NULL,NULL);
}
#endif

BOOL WINAPI isIconHidden(void){
	return g_isHidden;
}

void RestartTaskbarIcon(void){
	if (!g_isHidden){
		AddTaskbarIcon();
	}
}

void WINAPI setIconHidden(BOOL hidden){
	if (hidden && !g_isHidden){
		DeleteTaskbarIcon();
		g_isHidden = TRUE;
	}
	else if(!hidden && g_isHidden){
		g_isHidden = FALSE;
		AddTaskbarIcon();
	}	
}

VOID RegisterTaskbarCreatedMsg(void){
	g_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
}

BOOL AddTaskbarIcon(void){
	DWORD error;

	if(g_isHidden){
		return FALSE;
	}
	
	if (!AddEmptyTaskbarIcon()){
		return FALSE;
	}
	if (ResetTaskbarIcon()){
		return TRUE;
	}
	/* Do not leave the initial, empty icon behind when its first refresh fails. */
	error = GetLastError();
	DeleteTaskbarIcon();
	SetLastError(error);
	return FALSE;
}

BOOL AddEmptyTaskbarIcon(void){ 
    NOTIFYICONDATA tnid = {0};
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = g_hWnd; 
    tnid.uID = IDT_MYTRAY; 
    tnid.uFlags = NIF_MESSAGE; 
    tnid.uCallbackMessage = MYWM_NOTIFYICON; 
  
    return Shell_NotifyIcon(NIM_ADD, &tnid);
} 

BOOL DeleteTaskbarIcon(void) 
{ 
    NOTIFYICONDATA tnid = {0};

	if (g_isHidden){
		return FALSE;
	}
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = g_hWnd; 
    tnid.uID = IDT_MYTRAY; 
         
    return Shell_NotifyIcon(NIM_DELETE, &tnid);
} 

BOOL ResetTaskbarIcon(void)
{ 
    BOOL res; 
	DWORD error;
    NOTIFYICONDATA tnid = {0};
	HICON hIcon;
	UINT icon_id;
	UINT tip_id;

	if (g_isHidden){
		return FALSE;
	}
	

	if(isEnabled()){
		tip_id	= IDS_ENABLED_TIP;
		icon_id	= 
#ifdef DBG_NO_SIZING
			IDI_XPENABLED1;
#else
			(g_isXP)?IDI_XPENABLED:IDI_ENABLED;
#endif
	}
	else{
		tip_id	= IDS_DISABLED_TIP;
		icon_id	= (g_isXP)?IDI_XPDISABLED:IDI_DISABLED;
	}

	if (!LoadString(g_hInst, tip_id, tnid.szTip, chDIMOF(tnid.szTip))){
		return FALSE;
	}
 	hIcon=LoadImage(g_hInst,MAKEINTRESOURCE(icon_id),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
	if (!hIcon){
		return FALSE;
	}
	
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = g_hWnd; 
    tnid.uID = IDT_MYTRAY; 
    tnid.uFlags = NIF_ICON | NIF_TIP; 
    tnid.hIcon = hIcon; 
 
    res = Shell_NotifyIcon(NIM_MODIFY , &tnid); 
 
	if (!res) error = GetLastError();
    DestroyIcon(hIcon);
	if (!res) SetLastError(error);
 
    return res; 
} 
