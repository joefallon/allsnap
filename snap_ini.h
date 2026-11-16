//snap_ini.h

#ifndef SNAP_INI_INCLUDE

#define SNAP_INI_INCLUDE


BOOL LoadSettingsFromINI (void);
BOOL SaveSettingsToINI (void);

#ifdef ALLSNAP_TESTING
void SnapIni_TestSetPath(LPCTSTR path);
#endif

#endif //SNAP_INI_INCLUDE
