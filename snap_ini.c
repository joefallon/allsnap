//snap_registry.c
//Ivan Heckman (c) 2002
//
// handles saving/loading settings from the registry.

#include "stdafx.h"
#include "snap_registry.h"
#include "snap_lib.h"
#include "snap_sounds.h"
#include "snap_taskbar.h"
#include "snap_lineup.h"
#include "snap_grid.h"
#include <errno.h>
#include <limits.h>

TCHAR INI_NAME[] = _T("allsnap.ini");
TCHAR INI_PATH[MAX_PATH];
TCHAR INI_SECTION[]             = _T("settings");
#ifdef ALLSNAP_TESTING
static BOOL g_test_ini_path = FALSE;
static TCHAR g_test_ini_filename[MAX_PATH];
void SnapIni_TestSetPath(LPCTSTR path){
	g_test_ini_path = path != NULL;
	if (path){
		lstrcpyn(g_test_ini_filename,path,MAX_PATH);
	}
}
#endif
#define MAX_STRING_SIZE 8000

#define BEGIN_SREG_MAP() REG_ITEM ini_items[] = {
#define SREG_ENTRY(type,setter,getter,val_name) \
	{type,(void *)setter,(void *)getter,_T(val_name)},
#define END_SREG_MAP() \
	{REGT_EMPTY,(void*)0,(void*)0,_T("")}};\

typedef enum REGENUM {
	REGT_EMPTY,
	REGT_UINT,
	REGT_INT,
	REGT_SZ,
	REGT_BOOL,
	REGT_MULTI
}REGTYPE;

typedef void * PFV_GET;
typedef void * PFV_SET;

typedef void (WINAPI *PF_SET_UINT)	(UINT);
typedef void (WINAPI *PF_SET_INT)	(int);
typedef void (WINAPI *PF_SET_SZ)	(LPCTSTR,int);
typedef void (WINAPI *PF_SET_BOOL)	(BOOL);

typedef UINT (WINAPI *PF_GET_UINT)	    (void);
typedef int		(WINAPI *PF_GET_INT)	(void);
typedef LPCTSTR (WINAPI *PF_GET_SZ)		(void);
typedef BOOL	(WINAPI *PF_GET_BOOL)	(void);

typedef struct REGITEMTAG {
	REGTYPE	rt;	//type of the item
	PFV_GET pfv_setter;	//void pointer to setter function
	PFV_SET pfv_getter; //void pointer to getter function
	LPCTSTR value_name; //value name used in registry.
}REG_ITEM;



// Table of all registry entries with type and setter/getter function pointers
// macro expands to a empty terminated list of structs
BEGIN_SREG_MAP()
	SREG_ENTRY(REGT_BOOL, setEnabled, isEnabled, "enabled")
	SREG_ENTRY(REGT_INT, setWindowSpacing, getWindowSpacing, "window_spacing")
	SREG_ENTRY(REGT_INT,	setWinThresh,			getWinThresh,		"win_tresh")
	SREG_ENTRY(REGT_INT,	setScreenThresh,		getScreenThresh,	"screen_tresh")	
	SREG_ENTRY(REGT_UINT,	setSnapType,			getSnapType,		"snap_type")
	SREG_ENTRY(REGT_BOOL,	setNoisy,				isNoisy,			"play_sounds")
	SREG_ENTRY(REGT_SZ,		setSnapSound,			getSnapSound,		"snap_sound_file")
	SREG_ENTRY(REGT_SZ,		setUnsnapSound,			getUnsnapSound,		"unsnap_sound_file")
	SREG_ENTRY(REGT_UINT,	setToggleKey,			getToggleKey,		"toggle_key")
	SREG_ENTRY(REGT_UINT,	setCenterKey,			getCenterKey,		"center_key")
	SREG_ENTRY(REGT_UINT,   setEqualKey,			getEqualKey,	    "equal_key")
	SREG_ENTRY(REGT_BOOL,	setSnapMdi,				isSnapMdi,			"snap_mdi")
	SREG_ENTRY(REGT_BOOL,	setDisableToggle,		isDisableToggle,	"disable_toggle")
	SREG_ENTRY(REGT_BOOL,	setSnappingInsides,		isSnappingInsides,	"snapping_insides")
	SREG_ENTRY(REGT_INT,	setCropTop,				getCropTop,			"crop_top")
	SREG_ENTRY(REGT_BOOL,	setCroppingTop,			isCroppingTop,		"is_cropping_top")
	SREG_ENTRY(REGT_BOOL,	setIconHidden,			isIconHidden,		"icon_hidden")
	SREG_ENTRY(REGT_BOOL,	setKeptToScreen,		isKeptToScreen,		"kept_to_screen")
	SREG_ENTRY(REGT_SZ,	setSkinnedClasses,		getSkinnedClasses,	"skinned_classes")
	SREG_ENTRY(REGT_SZ,	setIgnoredClasses,		getIgnoredClasses,	"ignored_classes")
	SREG_ENTRY(REGT_BOOL,	setCroppingRgn,			isCroppingRgn,		"cropping_rgn")
	SREG_ENTRY(REGT_BOOL,	setHGridEnabled,		isHGridEnabled,		"hgridenabled")
	SREG_ENTRY(REGT_INT,	setHGridType,			getHGridType,		"hgridtype")
	SREG_ENTRY(REGT_INT,	setHGridVal,			getHGridVal,		"hgridval")
	SREG_ENTRY(REGT_BOOL,	setVGridEnabled,		isVGridEnabled,		"vgridenabled")
	SREG_ENTRY(REGT_INT,	setVGridType,			getVGridType,		"vgridtype")
	SREG_ENTRY(REGT_INT,	setVGridVal,			getVGridVal,		"vgridval")
END_SREG_MAP()


BOOL setVal(LPCTSTR value_name,
			DWORD dwValType,
			LPBYTE lpbData,
			DWORD dwSize,
			LPCTSTR inipath
			);

BOOL INIsetStringVal(LPCTSTR value_name,LPCTSTR szString,LPCTSTR inipath);

BOOL INIgetStringVal(LPCTSTR value_name,TCHAR * string,int * p_ret_len,int max_len,LPCTSTR inipath);


BOOL getInt(LPCTSTR value_name,int * p_int,LPCTSTR inipath){
	TCHAR buffer[64], * end, * prefix;
	long value;
	int base = 10;
	DWORD length = GetPrivateProfileString(INI_SECTION,value_name,NULL,buffer,chDIMOF(buffer),inipath);
	if (length == 0 || length >= chDIMOF(buffer)-1) return FALSE;
	prefix = buffer;
	while (_istspace((unsigned short)*prefix)) ++prefix;
	if (*prefix == _T('+') || *prefix == _T('-')) ++prefix;
	/* Preserve legacy 0x values without interpreting leading-zero decimals as octal. */
	if (prefix[0] == _T('0') && (prefix[1] == _T('x') || prefix[1] == _T('X'))) base = 16;
	errno = 0;
	value = _tcstol(buffer,&end,base);
	if (end == buffer || errno == ERANGE || value < INT_MIN || value > INT_MAX) return FALSE;
	while (_istspace((unsigned short)*end)) ++end;
	if (*end != 0) return FALSE;
	*p_int = (int)value;
	return TRUE;
}


BOOL INIgetStringVal(LPCTSTR value_name,TCHAR * string,int * p_ret_len,int max_len,LPCTSTR inipath){
	BOOL fOK = FALSE;
	int returned_length=0;

	int result;
	result = GetPrivateProfileString(INI_SECTION,value_name,NULL,string,max_len,inipath);
	*p_ret_len = result;

	if (result != 0) return TRUE;
	/* Only class lists use an explicit empty string as a value. Probe again
	   with a nonempty default to distinguish absence without a magic sentinel
	   collision for nonempty values. Keep legacy audio-path behavior unchanged. */
	if (_tcscmp(value_name,_T("ignored_classes")) == 0
		|| _tcscmp(value_name,_T("skinned_classes")) == 0){
		TCHAR missing[2];
		return GetPrivateProfileString(INI_SECTION,value_name,_T("\1"),missing,2,inipath) == 0;
	}
	return FALSE;
}

BOOL INIsetStringVal(LPCTSTR value_name,LPCTSTR szString,LPCTSTR inipath){
	return WritePrivateProfileString(INI_SECTION,value_name,szString,inipath);
}

BOOL setInt(LPCTSTR value_name, int val,LPCTSTR inipath){
	TCHAR buffer[32];
	wsprintf(buffer,_T("%d"),val);
	return INIsetStringVal(value_name,buffer,inipath);
}

LPCTSTR get_ini_path()
{
	static BOOL loaded_it = FALSE;
#ifdef ALLSNAP_TESTING
	if (g_test_ini_path){
		return g_test_ini_filename;
	}
#endif

	if(loaded_it){
		return INI_PATH;
	}
	else{

		TCHAR workdir[MAX_PATH];
		int dirlen = GetCurrentDirectory(MAX_PATH,workdir);
		if (!dirlen || (int)(dirlen + chDIMOF(INI_NAME) + 2) > MAX_PATH ){
			return 0;
		}
		wsprintf(INI_PATH,_T("%s\\%s"),workdir,INI_NAME);
		loaded_it = TRUE;
		return INI_PATH;
	}
}

static BOOL LoadSettings(BOOL preserve_runtime){
	int		temp = 0;
	DWORD	dwSize=sizeof(DWORD);
	int		reg_index = 0;
	BOOL	fOK = FALSE;
	LPCTSTR inipath = get_ini_path();

	if (!inipath){
		return FALSE;
	}

	for (reg_index = 0; ini_items[reg_index].rt!=REGT_EMPTY; reg_index++){
		LPCTSTR name = ini_items[reg_index].value_name;
		if (preserve_runtime && (_tcscmp(name,_T("enabled")) == 0
			|| _tcscmp(name,_T("icon_hidden")) == 0)) continue;
		temp = 0;
		switch(ini_items[reg_index].rt){

		case REGT_BOOL:
            if (getInt(ini_items[reg_index].value_name,&temp,inipath)){
				((PF_SET_BOOL)(ini_items[reg_index].pfv_setter))(temp != 0);
			}
			break;

		case REGT_INT:
			if (getInt(ini_items[reg_index].value_name,&temp,inipath)){
				if ((_tcscmp(name,_T("win_tresh")) == 0 || _tcscmp(name,_T("screen_tresh")) == 0)
					&& (temp < 2 || temp > 100)) break;
				if (_tcscmp(name,_T("crop_top")) == 0 && temp < 0) break;
				((PF_SET_INT)(ini_items[reg_index].pfv_setter))(temp);
			}
			break;

		case REGT_UINT:
			if (getInt(ini_items[reg_index].value_name,&temp,inipath)){
				((PF_SET_UINT)(ini_items[reg_index].pfv_setter))((UINT)temp);
			}
			break;

		case REGT_SZ:
			{
				TCHAR	path_buffer[MAX_STRING_SIZE];
				int ret_len = 0;

				if (INIgetStringVal(ini_items[reg_index].value_name,path_buffer,&ret_len,MAX_STRING_SIZE,inipath)){
					(*(PF_SET_SZ)(ini_items[reg_index].pfv_setter))((LPCTSTR)path_buffer,ret_len);
				}
			}
			break;
		case REGT_MULTI:
			break;

		case REGT_EMPTY:
		default:
			goto LoadSettingsFromRegistry_END;
		}
	}
	fOK = TRUE;
LoadSettingsFromRegistry_END:
	return TRUE;
}

BOOL LoadSettingsFromINI(void){ return LoadSettings(FALSE); }
BOOL LoadSettingsForDialog(void){ return LoadSettings(TRUE); }


BOOL SaveSettingsToINI (void){
	int	  temp_int;
	BOOL fOK=FALSE;
	int		reg_index = 0;

	LPCTSTR inipath = get_ini_path();

	if (!inipath){
		return FALSE;
	}
	
	while (ini_items[reg_index].rt!=REGT_EMPTY){
		LPCTSTR val_name = ini_items[reg_index].value_name;
        PFV_GET pfvGet =   ini_items[reg_index].pfv_getter;

		switch(ini_items[reg_index].rt){
			case REGT_BOOL:
				temp_int = (int) ((PF_GET_BOOL)(pfvGet))();
				goto SAVE_INT;
			case REGT_INT:
				temp_int = (int) ((PF_GET_INT)(pfvGet))();
				goto SAVE_INT;
			case REGT_UINT:
				temp_int = (int) ((PF_GET_UINT)(pfvGet))();
SAVE_INT:
				if (!setInt(val_name,temp_int,inipath)){
					goto SSTR_ERROR;
				}
				break;


			case REGT_MULTI:
				break;

			case REGT_SZ:
				if (!INIsetStringVal (val_name,((PF_GET_SZ)pfvGet)(),inipath)){
					goto SSTR_ERROR;
				}
				break;
			


			case REGT_EMPTY:
			default:
				_RPT0(_CRT_ERROR,"bad regitem");
				goto SSTR_ERROR;

		}//switch
		reg_index++;
	}//while

	fOK = TRUE;
SSTR_ERROR:
    //RegCloseKey(myKey);
	return fOK;
}
