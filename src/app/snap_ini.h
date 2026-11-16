//snap_ini.h

#ifndef SNAP_INI_INCLUDE

#define SNAP_INI_INCLUDE


BOOL LoadSettingsFromINI (void);
/* Reload persisted preferences without undoing live tray enable/visibility state. */
BOOL LoadSettingsForDialog (void);
BOOL SaveSettingsToINI (void);

#ifdef ALLSNAP_TESTING
void SnapIni_TestSetPath(LPCTSTR path);
#endif

#endif //SNAP_INI_INCLUDE
