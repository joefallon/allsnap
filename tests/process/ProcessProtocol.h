#pragma once

#include <windows.h>

enum ProcessCommand
{
    ProcessReady = 1,
    ProcessStartHook,
    ProcessRegister,
    ProcessFinishMove,
    ProcessQuery,
    ProcessStopHook,
    ProcessArmReservation,
    ProcessExitNormally,
    ProcessReleaseModule,
    ProcessReleaseDuringCallback,
    ProcessEnterHook
};

struct ProcessRequest
{
    DWORD command;
};

struct ProcessResult
{
    DWORD success;
    DWORD error;
    LONG active;
    DWORD canStop;
    DWORD localOwner;
    DWORD moduleMapped;
};

typedef BOOL (__cdecl *ProcessDispatch)(DWORD, HWND, HANDLE, HANDLE, struct ProcessResult *);

#define PROCESS_TEST_TIMEOUT 10000
