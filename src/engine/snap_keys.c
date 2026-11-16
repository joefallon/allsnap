#include "stdafx.h"
#include "snap_keys.h"

#ifdef ALLSNAP_TESTING
static SNAP_KEY_STATE_READER key_state_reader = GetAsyncKeyState;
void SnapKeys_TestSetReader(SNAP_KEY_STATE_READER reader)
{
    key_state_reader = reader ? reader : GetAsyncKeyState;
}
#define ReadKeyState key_state_reader
#else
#define ReadKeyState GetAsyncKeyState
#endif

BOOL SnapKeys_IsDown(UINT virtual_key)
{
    /* The low bit reports a recent press, not whether the key is still held. */
    return (ReadKeyState((int)virtual_key) & 0x8000) != 0;
}
