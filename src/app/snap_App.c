// snap_App.c : runs client to load and hook snap_lib.dll,
//				provides a property sheet through the notify taskbar
//				and unhooks the dll when closed.
#include "stdafx.h"

#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <Shellapi.h>
#include "snap_lib.h"

#include "snap_App.h"
#include "allsnap_version.h"
#include "snap_settings.h"
#include "snap_ini.h"
#include "snap_mywm_msg.h"
#include "snap_taskbar.h"
#include "snap_debug.h"
#include "snap_lineup.h"
#include "snap_diagnostics.h"

/**************************************************************************
   Global Variables
**************************************************************************/

HINSTANCE	g_hInst;							// current instance
HWND		g_hWnd;								//main window



HANDLE			g_hMutex = NULL;					//ensures only one running
HCURSOR			g_hcHand;							//pointer cursor
HANDLE			g_hiPaypal;

OSVERSIONINFO	g_os;
BOOL g_isXP;
HWND			g_hwndAbout = NULL;


/* Human-readable release version; edit the adjacent allsnap_version.h. */
static const TCHAR g_version[] = _T(ALLSNAP_VERSION_TEXT);
static const UINT g_instance_version = ALLSNAP_INSTANCE_VERSION;




#ifdef JUST_MOVING
TCHAR g_szClassName[] = TEXT("allSnapClassMoving");	
TCHAR g_szTitleName[] = TEXT("allSnapMoving");
TCHAR g_szMutexName[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEX_MOVING");
TCHAR g_szMutexNameB[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEXB_MOVING");
#else
#ifdef _WIN64
TCHAR g_szClassName[] = TEXT("allSnapClassSizing64");	
TCHAR g_szTitleName[] = TEXT("allSnapSizing64");
TCHAR g_szMutexName[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEX_SIZING64");
TCHAR g_szMutexNameB[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEXB_SIZING64");
#else
TCHAR g_szClassName[] = TEXT("allSnapClassSizing");	
TCHAR g_szTitleName[] = TEXT("allSnapSizing");
TCHAR g_szMutexName[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEX_SIZING");
TCHAR g_szMutexNameB[] =_T("IVAN_HECKMAN_ALLSNAP_MUTEXB_SIZING");
#endif
#endif

/**************************************************************************
   Local Function Prototypes
**************************************************************************/
int APIENTRY	 _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR lpCmdLine,int nCmdShow);
BOOL			 CheckCharSet(void);
BOOL			 OneInstanceOnly(void);

BOOL			 InitApplication(HINSTANCE hInstance);
BOOL			 InitInstance(HINSTANCE hInstance);
BOOL			 InitMyStuff(HINSTANCE hInstance);
BOOL			 UnloadMyStuff(void);

LRESULT CALLBACK WndProc		(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL			 OnCommand		(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);
VOID			 OnNotifyIcon	(HWND hWnd, UINT uID, UINT event);

INT_PTR CALLBACK	 AboutProc		(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MyButtonProc	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID			 ContextMenu(HWND hWnd);
VOID			 SetHandCursor( HWND hButton);


/**************************************************************************

   WinMain(void)

**************************************************************************/
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow){
	MSG msg = {0};
	BOOL message_result;

	g_hInst = hInstance;

	if (
			!CheckCharSet()
			||
			!OneInstanceOnly()){
		return FALSE;
	}

	

	if(!hPrevInstance){
		if(!InitApplication(hInstance)){
			return FALSE;
		}
	}

	if (!InitInstance(hInstance)){
		return FALSE;
	}

	if (!SnapHookAll(g_hWnd,g_os)){
		DWORD error = GetLastError();
		TCHAR error_message[160];
		SnapDiagnostics_Record(SNAP_DIAG_HOOK_INSTALL,error);
		wsprintf(error_message,_T("allSnap could not install its window hook (error %lu)."),error);
		MessageBox(g_hWnd,error_message,_T("allSnap - Startup failed"),MB_OK|MB_ICONERROR);
		DestroyWindow(g_hWnd);
		return 1;
	}

	InitMyStuff(hInstance);
	SnapDiagnostics_Record(SNAP_DIAG_HOOK_INSTALL,ERROR_SUCCESS);



	while((message_result = GetMessage(&msg, NULL, 0x00, 0x00)) > 0)
	{
		// If the modeless guy is up and is ready to be destroyed
		// (PropSheet_GetCurrentPageHwnd returns NULL) then destroy the dialog.
		   
		// PropSheet_GetCurrentPageHwnd will return NULL after the OK or Cancel 
		// button has been pressed and all of the pages have been notified. The 
		// Apply button doesn't cause this to happen.
		if(g_hwndPropSheet && (NULL == PropSheet_GetCurrentPageHwnd(g_hwndPropSheet))){
		//enable the parent first to prevent another window from becoming the foreground window
		EnableWindow(g_hWnd, TRUE);
		DestroyWindow(g_hwndPropSheet);
		g_hwndPropSheet = NULL;
		}

		//use PropSheet_IsDialogMessage instead of IsDialogMessage
		if(		(!IsWindow(g_hwndPropSheet) 
				|| !PropSheet_IsDialogMessage(g_hwndPropSheet, &msg))
			&& 
				(!IsWindow(g_hwndAbout) 
				|| !PropSheet_IsDialogMessage(g_hwndAbout, &msg))

#ifdef _DEBUG
			&&
				(!IsWindow(g_hwndDebug) 
				|| !PropSheet_IsDialogMessage(g_hwndDebug, &msg))
#endif

		){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}
	}
	
	if (message_result == -1){
		SnapDiagnostics_Record(SNAP_DIAG_MESSAGE_LOOP,GetLastError());
	}

	UnloadMyStuff();
	if (!SnapUnHookAll()){
		SnapDiagnostics_Record(SNAP_DIAG_HOOK_REMOVE,GetLastError());
	}else{
		SnapDiagnostics_Record(SNAP_DIAG_HOOK_REMOVE,ERROR_SUCCESS);
	}
#ifdef ASNAP_AUTO_LINEUP
	LINEUP_Cleanup();
#endif
	/* Let the OS remove the instance mutex on process exit. */

	return message_result == -1 ? 1 : (int)msg.wParam;
}

void InitCmnCtls(void){
	static int already_did_this = 0;
	if (!already_did_this){
		INITCOMMONCONTROLSEX iccex = {sizeof(INITCOMMONCONTROLSEX),ICC_WIN95_CLASSES};
		//don't forget this
		InitCommonControlsEx(&iccex);
		already_did_this = 1;
	}
}

/**************************************************************************

   OneInstanceOnly(void)
   
   check if another instance of this program is running
   by using a Mutex
**************************************************************************/
BOOL OneInstanceOnly(void){
	DWORD dwLastError;

	g_hMutex = CreateMutex(NULL,TRUE,(LPCTSTR)g_szMutexNameB);
	dwLastError = GetLastError();
	
	if (g_hMutex == NULL){
		MessageBox(g_hWnd,_T("Sorry but there was an error trying to create a Mutex"),
			_T("Sorry... Application Closing"),MB_OK);
		return FALSE;
	}
	else if (dwLastError == ERROR_ALREADY_EXISTS){
		HWND prevAppWnd = FindWindow(g_szClassName,g_szTitleName);
		if (prevAppWnd != NULL){
			SendMessage(prevAppWnd,MYWM_UNHIDEICON,g_instance_version,0L);
		}
		return FALSE;
	}
	else{
		HANDLE previous_version =  CreateMutex(NULL,TRUE,(LPCTSTR)g_szMutexName);
		dwLastError = GetLastError();

		if (previous_version == NULL){
			MessageBox(g_hWnd,_T("Sorry but there was an error trying to create a Mutex"),
				_T("Sorry... Application Closing"),MB_OK);
			return FALSE;
		}
		else if (dwLastError == ERROR_ALREADY_EXISTS){
			MessageBox(g_hWnd,_T("This version can't be run at the same time\nas an earlier version of allSnap"),
			_T("Sorry... Application Closing"),MB_OK);
			return FALSE;
		}
		else{
			CloseHandle(previous_version);
		}
		return TRUE;
	}
}
/**************************************************************************

   CheckCharSet(void)
   
   does the UNICODE def state match the platform support
**************************************************************************/
BOOL CheckCharSet(void){
	char sz9xError[] = "This file needs a platform with UNICODE support to run.\n";
				
	wchar_t sz2000Error[]=
					L"This file is meant to run only on win9x.\n";
	
	#ifdef UNICODE
		if(!Can_Handle_Unicode){
			MessageBoxA(NULL,sz9xError,"incorrect file",MB_ICONSTOP |MB_OK);
		return FALSE;
		}
	#else
		if(Can_Handle_Unicode){
			MessageBoxW(NULL,sz2000Error,L"incorrect file",MB_ICONSTOP |MB_OK);
		return FALSE;
		}
	#endif  //!UNICODE
	return TRUE;
}




/**************************************************************************
   InitApplication(void)
**************************************************************************/
BOOL InitApplication(HINSTANCE hInstance){

	OSVERSIONINFO  os;

	ZeroMemory(&os, sizeof(os));
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);

	g_os = os;
	g_isXP = ((g_os.dwMajorVersion>=5) && (g_os.dwMinorVersion>=1));

	if(os.dwMajorVersion >= 4){
		WNDCLASSEX  wcex;

		ZeroMemory(&wcex, sizeof(wcex));
		   
		wcex.cbSize          = sizeof(wcex);
		wcex.style           = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc     = (WNDPROC)WndProc;
		wcex.cbClsExtra      = 0;
		wcex.cbWndExtra      = 0;
		wcex.hInstance       = hInstance;
		wcex.hIcon			 = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_SNAPIT));
		wcex.hCursor         = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground   = GetStockObject(WHITE_BRUSH);
		wcex.lpszClassName   = g_szClassName;

		return RegisterClassEx(&wcex);
	}
	else{
		WNDCLASS  wc;

		ZeroMemory(&wc, sizeof(wc));
		
		wc.style          = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc    = (WNDPROC)WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = hInstance;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground  = GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = g_szClassName;
		wc.hIcon		  = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_SNAPIT));

		return RegisterClass(&wc);
	}
}



/**************************************************************************

   InitInstance(void)

**************************************************************************/
BOOL InitInstance(   HINSTANCE hInstance){
	g_hWnd	= CreateWindowEx(  0,
								g_szClassName,
								g_szTitleName,
								WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								NULL,
								NULL,
								hInstance,
								NULL);

	if (!g_hWnd){
		return FALSE;
	}

	ShowWindow(g_hWnd, SW_HIDE); //Hiden only see taskbar notify icon 
	UpdateWindow(g_hWnd);

	return TRUE;
}

BOOL InitMyStuff(HINSTANCE hInstance){
	g_hcHand = LoadCursor(hInstance,MAKEINTRESOURCE(IDC_MYHAND));
	
	g_hiPaypal = LoadImage(
		hInstance,
		MAKEINTRESOURCE(IDB_PAYPAL),
		IMAGE_BITMAP,110,23,LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
	

	LoadSettingsFromINI();


	RegisterTaskbarCreatedMsg();
	AddTaskbarIcon();


	
	return TRUE;
}

BOOL UnloadMyStuff(void){
	DeleteObject(g_hcHand);
	DeleteObject(g_hiPaypal);
	return TRUE;
}

void OnNotifyIcon(HWND hWnd, UINT uID, UINT event){

	switch(event){
		case WM_LBUTTONDBLCLK:
			SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(IDM_SETTINGS,0),0);
			break;
		case WM_RBUTTONUP:
			ContextMenu(hWnd);			
			break;

	}
}

BOOL OnCommand (HWND hWnd, int id, HWND hwndCtl, UINT codeNotify){
	switch (id){
		case ID_ENABLEWINDOWSNAPPING:
			setEnabled(!isEnabled());
			ResetTaskbarIcon();

			if( isEnabled() && getSnapType() == SNAPT_NONE){
				SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(IDM_SETTINGS,0),0);
			}

			break;
	
		case IDM_SETTINGS:
			if (!IsWindow(g_hwndPropSheet)){
				g_hwndPropSheet = DoPropertySheet(g_hWnd);
			}
			SetActiveWindow(g_hWnd);
			SetForegroundWindow(g_hwndPropSheet);
			SetFocus(g_hwndPropSheet);
			break;
#ifdef _DEBUG
		case IDM_DEBUG:

			if (!IsWindow(g_hwndDebug)){
				g_hwndDebug = CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_DEBUG),NULL,DebugProc);
				ShowWindow(g_hwndDebug,SW_SHOW);
			}
			else{
				SetForegroundWindow(g_hwndDebug);
				SetFocus(g_hwndDebug); 
			}
			break;
#endif
		case IDM_ABOUT:
			if (!IsWindow(g_hwndAbout)){
				InitCmnCtls();
				g_hwndAbout = CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_ABOUT),NULL,AboutProc);
				ShowWindow(g_hwndAbout,SW_SHOW);
			}
			else{
				SetForegroundWindow(g_hwndAbout);
				SetFocus(g_hwndAbout); 
			}
			break;
		case IDHELP:
			ShellExecute(NULL, _T("open"), _T("allSnap.chm"),
              NULL, NULL, SW_SHOWNORMAL);
			break;

		case IDM_EXIT:
			if (!SnapCanUnHook()){
				int msg_ret;
				
				do{
					msg_ret = MessageBox(NULL,
						_T("allSnap could not release properly from another process.")
						_T("\nTry closing all programs and retry."),
						_T("allSnap - Error closing"),
						MB_RETRYCANCEL | MB_ICONSTOP);
					if (msg_ret == IDCANCEL){
						return TRUE;	//cancel closing
					}

				}while(!SnapCanUnHook());
			}
			if (!SnapUnHookAll()){
				DWORD error = GetLastError();
				SnapDiagnostics_Record(SNAP_DIAG_HOOK_REMOVE,error);
				MessageBox(hWnd,_T("allSnap could not release its window hook. Try exiting again."),
					_T("allSnap - Error closing"),MB_OK|MB_ICONERROR);
				return TRUE;
			}
			DeleteTaskbarIcon();
			DestroyWindow(hWnd);
	}
	return TRUE;
} 

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{	
		case MYWM_DIAGNOSTIC:
			if (wParam >= SNAP_DIAG_HOOK_INSTALL && wParam <= SNAP_DIAG_HOOK_REMOVE){
				SnapDiagnostics_Record((enum SNAP_DIAGNOSTIC_STAGE)wParam,(DWORD)lParam);
			}
			return 0;

#ifdef _DEBUG
		case WM_SETTEXT:
			if (IsWindow(g_hwndDebug)){
				SendMessage(g_hwndDebug,MY_DEBUG_STR,wParam,lParam);
			}
			break;

		case MYWM_DEBUG1:
			if (IsWindow(g_hwndDebug)){
				SendMessage(g_hwndDebug,MYWM_DEBUG1,wParam,lParam);
			}
			break;
#endif
		case MYWM_UNHIDEICON:
			if (wParam != g_instance_version){
				MessageBox(hWnd,_T("allSnap did not start because this version was previously running in a hidden state."),
					_T("allSnap: blocked from starting by previous version"),MB_OK|MB_ICONEXCLAMATION);

			}
			if (isIconHidden()){
                setIconHidden(FALSE);
			}
			break;

		case MYWM_NOTIFYICON:
			OnNotifyIcon(hWnd,(UINT)wParam,(UINT)lParam);
			break;

		HANDLE_MSG(hWnd,WM_COMMAND,OnCommand);

		case WM_QUERYENDSESSION:
		case WM_ENDSESSION:
			return 1;

		case WM_CLOSE:
			return 0;



		case WM_DESTROY:
			DeleteTaskbarIcon();
			PostQuitMessage(0);
			return 0;

		default:
			
			if(message == g_uTaskbarRestart){
				RestartTaskbarIcon();
			}
        
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void ContextMenu(HWND hWnd){
	HMENU hContextMenu = GetSubMenu(LoadMenu(g_hInst,  MAKEINTRESOURCE(IDR_SYSMENU)),0);
	POINT pt;
	SetForegroundWindow(hWnd);

	if(IsWindow(g_hwndPropSheet)){
		SetForegroundWindow(g_hwndPropSheet);
		SetFocus(g_hwndPropSheet);
	}
	// Display the menu
	GetCursorPos(&pt);
	CheckMenuItem(hContextMenu,ID_ENABLEWINDOWSNAPPING, 
		(isEnabled())?MF_CHECKED:MF_UNCHECKED);
	

	SetMenuDefaultItem(hContextMenu,IDM_SETTINGS,FALSE);

	TrackPopupMenu(   hContextMenu,
		 TPM_LEFTALIGN | TPM_BOTTOMALIGN,
		 pt.x,
		 pt.y,
		 0,
		 hWnd,
		 NULL);

	PostMessage(hWnd, WM_NULL, 0, 0);
}

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	LOGBRUSH logbrush;
	TCHAR caption[MAX_LOADSTRING];
	static COLORREF BkColor, FgColor;
	static HBRUSH hBkBrush;
	
	switch(uMsg){
		
		case WM_INITDIALOG:
			_sntprintf_s(caption,chDIMOF(caption),_TRUNCATE,_T("About allSnap %s"),g_version);
			SetWindowText(hDlg,caption);
			SetClassLongPtr(hDlg,GCLP_HICON,(LONG_PTR)NULL);
			// initializing dialog
			// create background brush and initialize colors
			logbrush.lbStyle = BS_NULL;
			logbrush.lbHatch = 0;
			BkColor = RGB (150, 200, 250);
			logbrush.lbColor = BkColor;
			hBkBrush = CreateBrushIndirect (&logbrush);
			FgColor = RGB (0, 0, 255);

			SendDlgItemMessage(hDlg,IDC_PAYPAL,STM_SETIMAGE,
				(WPARAM)IMAGE_BITMAP,(LPARAM)g_hiPaypal);
			SetHandCursor(GetDlgItem(hDlg,IDC_LINK));
			SetHandCursor(GetDlgItem(hDlg,IDC_EMAIL));
			SetHandCursor(GetDlgItem(hDlg,IDC_PAYPAL));
			break;

		
		case WM_CTLCOLORSTATIC:
			// process this message to set STATIC and READONLY EDIT control colors
			// lParam holds hwnd of individual control to be painted
			if (GetDlgCtrlID((HWND)lParam) == IDC_LINK
				||GetDlgCtrlID((HWND)lParam) == IDC_EMAIL){
				SetBkMode ((HDC) wParam, TRANSPARENT);
				SetTextColor ((HDC) wParam, FgColor);
				return (INT_PTR) hBkBrush;
			}
			else{
				return FALSE;
			}


		case WM_CTLCOLOREDIT:
			// process this message to set EDIT control colors
			// lParam holds hwnd of individual control to be painted
			SetTextColor ((HDC) wParam, FgColor);
			return (INT_PTR) hBkBrush;

		case WM_COMMAND:
			if(LOWORD(wParam)!=IDOK){
				return FALSE;
			}
		case WM_CLOSE:
			// destroy brushes
			DeleteObject (hBkBrush);
			// closing dialog
			DestroyWindow(g_hwndAbout);
			g_hwndAbout = NULL;
			return TRUE;
	}
	return FALSE;
}



void SetHandCursor( HWND hButton)
{  
    LONG_PTR ulOriginalProc;

    // Get the original window procedure address and replace
    // it by our own
    ulOriginalProc = SetWindowLongPtr( 
        hButton, 
        GWLP_WNDPROC, 
        (LONG_PTR)MyButtonProc );

    // Store the original window procedure address in
    // the user data of the button
    SetWindowLongPtr(
        hButton,
        GWLP_USERDATA,
        ulOriginalProc );
}

LRESULT CALLBACK MyButtonProc(
    HWND    hWnd, 
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam )
{
	WNDPROC pfProc;
	TCHAR   szLink[MAX_LOADSTRING];


	switch (uMsg){
		case WM_SETCURSOR:
			SetCursor(g_hcHand);
			return FALSE;
		case WM_LBUTTONUP:
			
			if (LoadString(g_hInst,GetDlgCtrlID(hWnd),szLink,MAX_LOADSTRING)!=0){					
				ShellExecute(NULL, _T("open"), szLink,NULL, NULL, SW_SHOWNORMAL);
			}

			return TRUE;
		default:
            pfProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA );
			return CallWindowProc( pfProc, hWnd,uMsg, wParam, lParam );
	}
} 


