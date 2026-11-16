#ifndef SNAP_KEYS_H
#define SNAP_KEYS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL SnapKeys_IsDown(UINT virtual_key);

#ifdef ALLSNAP_TESTING
typedef SHORT (WINAPI *SNAP_KEY_STATE_READER)(int virtual_key);
void SnapKeys_TestSetReader(SNAP_KEY_STATE_READER reader);
#endif

#ifdef __cplusplus
}
#endif
#endif
