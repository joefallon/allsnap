#include "stdafx.h"
#include <stdio.h>
#include "snap_diagnostics.h"

/* Only the application message thread writes files; hooks post diagnostic messages. */
static BOOL RecordAtPath(LPCTSTR path, enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error){
	HANDLE file;
	SYSTEMTIME time;
	char line[160];
	DWORD written;
	int length;
	BOOL result;
	file = CreateFile(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	GetSystemTime(&time);
	length = sprintf_s(line, sizeof(line),
		"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ pid=%lu stage=%u error=%lu\r\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
		time.wMilliseconds, GetCurrentProcessId(), (unsigned)stage, error);
	result = length > 0 && WriteFile(file, line, (DWORD)length, &written, NULL)
		&& written == (DWORD)length;
	CloseHandle(file);
	return result;
}

void SnapDiagnostics_Record(enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error){
	DWORD saved_error = GetLastError();
	TCHAR directory[MAX_PATH];
	TCHAR path[MAX_PATH];
	DWORD length = GetEnvironmentVariable(_T("LOCALAPPDATA"), directory, MAX_PATH);
	if (length && length < MAX_PATH - 32){
		_tcscat_s(directory, MAX_PATH, _T("\\allSnap"));
		CreateDirectory(directory, NULL);
		_sntprintf_s(path, MAX_PATH, _TRUNCATE, _T("%s\\diagnostics%s.log"), directory,
#ifdef _WIN64
			_T("64")
#else
			_T("32")
#endif
		);
		RecordAtPath(path, stage, error);
	}
	SetLastError(saved_error);
}

#ifdef ALLSNAP_TESTING
BOOL SnapDiagnostics_TestRecord(LPCTSTR path, enum SNAP_DIAGNOSTIC_STAGE stage, DWORD error){
	return RecordAtPath(path, stage, error);
}
#endif
