#ifndef SNAP_DIAGNOSTICS_H
#define SNAP_DIAGNOSTICS_H

#include <windows.h>
#include "snap_mywm_msg.h"

void SnapDiagnostics_Record(enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error);

#ifdef ALLSNAP_TESTING
BOOL SnapDiagnostics_TestRecord(LPCTSTR path, enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error);
#endif

#endif
