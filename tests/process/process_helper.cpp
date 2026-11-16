#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include "ProcessProtocol.h"

namespace
{
    const UINT CommandMessage = WM_APP + 1;
    const UINT CallbackMessage = WM_APP + 2;
    HMODULE engine;
    ProcessDispatch dispatch;
    HWND window;
    HANDLE pipeHandle, enteredEvent, releaseEvent;

    bool WriteResult(const ProcessResult & result)
    {
        DWORD written = 0;
        return WriteFile(pipeHandle, &result, sizeof(result), &written, nullptr) && written == sizeof(result);
    }

    LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM command, LPARAM parameter)
    {
        if (message == CallbackMessage) {
            SetEvent(enteredEvent);
            return WaitForSingleObject(releaseEvent, PROCESS_TEST_TIMEOUT) == WAIT_OBJECT_0;
        }
        if (message == CommandMessage) {
            ProcessResult * result = reinterpret_cast<ProcessResult *>(parameter);
            dispatch(static_cast<DWORD>(command), hwnd, enteredEvent, releaseEvent, result);
            return 0;
        }
        return DefWindowProcW(hwnd, message, command, parameter);
    }

    DWORD WINAPI RunCallback(void *)
    {
        return static_cast<DWORD>(SendMessageW(window, CallbackMessage, 0, 0));
    }

    DWORD WINAPI Commands(void *)
    {
        ProcessRequest request = {};
        DWORD received = 0;
        while (ReadFile(pipeHandle, &request, sizeof(request), &received, nullptr) && received == sizeof(request)) {
            if (request.command == ProcessExitNormally)
                ExitProcess(0); // Exercise actual process detach with the registration still owned.
            ProcessResult result = {};
            if (request.command == ProcessReleaseModule || request.command == ProcessReleaseDuringCallback) {
                HANDLE callback = nullptr;
                if (request.command == ProcessReleaseDuringCallback) {
                    ResetEvent(enteredEvent);
                    ResetEvent(releaseEvent);
                    callback = CreateThread(nullptr, 0, RunCallback, nullptr, 0, nullptr);
                    if (!callback || WaitForSingleObject(enteredEvent, PROCESS_TEST_TIMEOUT) != WAIT_OBJECT_0) {
                        result.error = ERROR_TIMEOUT;
                        WriteResult(result);
                        ExitProcess(12);
                    }
                }
                FreeLibrary(engine);
                engine = nullptr;
                HMODULE retained = nullptr;
                result.moduleMapped = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(dispatch), &retained);
                if (result.moduleMapped) {
                    if (callback) {
                        SetEvent(releaseEvent);
                        if (WaitForSingleObject(callback, PROCESS_TEST_TIMEOUT) != WAIT_OBJECT_0) {
                            result.error = ERROR_TIMEOUT;
                            WriteResult(result);
                            ExitProcess(13);
                        }
                        DWORD completed = FALSE;
                        if (!GetExitCodeThread(callback, &completed) || completed != TRUE) {
                            result.error = ERROR_TIMEOUT;
                            WriteResult(result);
                            ExitProcess(14);
                        }
                        CloseHandle(callback);
                    }
                    SendMessageW(window, CommandMessage, ProcessFinishMove, reinterpret_cast<LPARAM>(&result));
                } else {
                    result.error = ERROR_MOD_NOT_FOUND;
                }
                WriteResult(result);
                if (!result.moduleMapped)
                    ExitProcess(0); // Do not dispatch through a now-unmapped native subclass.
            } else {
                SendMessageW(window, CommandMessage, request.command, reinterpret_cast<LPARAM>(&result));
                if (!WriteResult(result)) break;
            }
        }
        ExitProcess(0);
    }
}

int wmain(int argc, wchar_t ** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    if (argc != 5) return 2;
    pipeHandle = CreateFileW(argv[1], GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipeHandle == INVALID_HANDLE_VALUE) return 3;
    enteredEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, argv[3]);
    releaseEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, argv[4]);
    if (!enteredEvent || !releaseEvent) return 4;
    engine = LoadLibraryW(argv[2]);
    if (!engine) return 5;
    dispatch = reinterpret_cast<ProcessDispatch>(GetProcAddress(engine, "SnapProcessDispatch"));
    if (!dispatch) return 6;
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_WIN95_CLASSES};
    if (!InitCommonControlsEx(&controls)) return 7;
    WNDCLASSW type = {};
    type.lpfnWndProc = WindowProcedure;
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpszClassName = L"allSnap isolated process fixture";
    if (!RegisterClassW(&type)) return 8;
    window = CreateWindowExW(0, type.lpszClassName, L"allSnap owned process window", WS_POPUP,
        -30000, -30000, 320, 240, nullptr, nullptr, type.hInstance, nullptr);
    if (!window) return 9;
    ProcessResult ready = {};
    dispatch(ProcessReady, window, enteredEvent, releaseEvent, &ready);
    if (!WriteResult(ready)) return 10;
    HANDLE reader = CreateThread(nullptr, 0, Commands, nullptr, 0, nullptr);
    if (!reader) return 11;
    CloseHandle(reader);
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}
