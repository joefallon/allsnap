#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <CppUnitTest.h>
#include <memory>
#include <string>
#include <vector>
#include "../process/ProcessProtocol.h"

namespace AllSnapTests
{
    using namespace Microsoft::VisualStudio::CppUnitTestFramework;

    namespace ProcessSupport
    {
        class Handle
        {
            HANDLE value;
        public:
            explicit Handle(HANDLE handle = nullptr) : value(handle) {}
            ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
            Handle(const Handle &) = delete;
            Handle & operator=(const Handle &) = delete;
            HANDLE Get() const { return value; }
            void Reset(HANDLE handle) { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); value = handle; }
        };

        inline std::wstring UniqueName()
        {
            static volatile LONG serial = 0;
            return std::to_wstring(GetCurrentProcessId()) + L"-" +
                std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(InterlockedIncrement(&serial));
        }

        inline std::wstring BinaryDirectory()
        {
            HMODULE module = nullptr;
            Assert::IsTrue(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&BinaryDirectory), &module) != FALSE);
            WCHAR path[MAX_PATH] = {};
            DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
            Assert::IsTrue(length != 0 && length < MAX_PATH);
            std::wstring directory(path);
            return directory.substr(0, directory.find_last_of(L"\\/"));
        }

        struct PipeOperation
        {
            OVERLAPPED operation = {};
            Handle event;
            BYTE buffer[sizeof(ProcessResult)] = {};
            PipeOperation() : event(CreateEventW(nullptr, TRUE, FALSE, nullptr))
            {
                Assert::IsNotNull(event.Get());
                operation.hEvent = event.Get();
            }
        };

        struct OwnedProcess
        {
            Handle job;
            Handle process;
            OwnedProcess() : job(CreateJobObjectW(nullptr, nullptr)) {}
            ~OwnedProcess()
            {
                if (process.Get()) {
                    if (WaitForSingleObject(process.Get(), 0) == WAIT_TIMEOUT)
                        TerminateJobObject(job.Get(), 91);
                    WaitForSingleObject(process.Get(), PROCESS_TEST_TIMEOUT);
                }
            }
        };

        inline void CompleteIo(HANDLE pipe, std::unique_ptr<PipeOperation> & context,
            BOOL immediate, DWORD expected, HANDLE process, HANDLE job)
        {
            OVERLAPPED & operation = context->operation;
            DWORD error = immediate ? ERROR_SUCCESS : GetLastError();
            Assert::IsTrue(immediate || error == ERROR_IO_PENDING, L"Child process pipe operation failed.");
            if (!immediate && WaitForSingleObject(operation.hEvent, PROCESS_TEST_TIMEOUT) != WAIT_OBJECT_0) {
                TerminateJobObject(job, 92);
                CancelIoEx(pipe, &operation);
                if (WaitForSingleObject(operation.hEvent, PROCESS_TEST_TIMEOUT) != WAIT_OBJECT_0) {
                    // The kernel may still reference all three objects. Retain
                    // this failed operation until host exit rather than free
                    // pending OVERLAPPED/buffer/event storage or wait forever.
                    context.release();
                }
                Assert::Fail(L"Child process pipe operation timed out.");
            }
            DWORD transferred = 0;
            BOOL completed = GetOverlappedResult(pipe, &operation, &transferred, FALSE);
            std::wstring detail = L"Pipe completion error=" + std::to_wstring(GetLastError()) +
                L" expected bytes=" + std::to_wstring(expected) + L" actual bytes=" + std::to_wstring(transferred);
            if (!completed && process) {
                WaitForSingleObject(process, 100);
                DWORD exitCode = STILL_ACTIVE;
                GetExitCodeProcess(process, &exitCode);
                detail += L" helper exit=" + std::to_wstring(exitCode);
            }
            Assert::IsTrue(completed != FALSE, detail.c_str());
            Assert::AreEqual(expected, transferred, detail.c_str());
        }
    }

    class ProcessChild
    {
        std::wstring name;
        ProcessSupport::Handle pipe;
        ProcessSupport::Handle entered;
        ProcessSupport::Handle release;
        ProcessSupport::OwnedProcess child;
        bool pending = false;
    public:
        ProcessChild(const std::wstring & executable, const std::wstring & engine) :
            name(ProcessSupport::UniqueName()),
            pipe(CreateNamedPipeW((L"\\\\.\\pipe\\allsnap-process-" +
                name).c_str(),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                1, 4096, 4096, 0, nullptr)),
            entered(CreateEventW(nullptr, TRUE, FALSE, (L"Local\\allsnap-entered-" + name).c_str())),
            release(CreateEventW(nullptr, TRUE, FALSE, (L"Local\\allsnap-release-" + name).c_str()))
        {
            Assert::IsTrue(pipe.Get() != INVALID_HANDLE_VALUE);
            Assert::IsNotNull(entered.Get());
            Assert::IsNotNull(release.Get());
            Assert::IsNotNull(child.job.Get());
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
            limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            Assert::IsTrue(SetInformationJobObject(child.job.Get(), JobObjectExtendedLimitInformation,
                &limits, sizeof(limits)) != FALSE);
            std::wstring arguments = L"\"" + executable + L"\" \"\\\\.\\pipe\\allsnap-process-" + name +
                L"\" \"" + engine + L"\" \"Local\\allsnap-entered-" + name +
                L"\" \"Local\\allsnap-release-" + name + L"\"";
            STARTUPINFOW startup = {};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION information = {};
            Assert::IsTrue(CreateProcessW(executable.c_str(), &arguments[0], nullptr, nullptr, FALSE,
                CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &information) != FALSE);
            child.process.Reset(information.hProcess);
            ProcessSupport::Handle thread(information.hThread);
            if (!AssignProcessToJobObject(child.job.Get(), child.process.Get())) {
                TerminateProcess(child.process.Get(), 90);
                WaitForSingleObject(child.process.Get(), PROCESS_TEST_TIMEOUT);
                Assert::Fail(L"Could not place the isolated helper in its owned cleanup job.");
            }
            Assert::IsTrue(ResumeThread(thread.Get()) != static_cast<DWORD>(-1));
            std::unique_ptr<ProcessSupport::PipeOperation> connect(new ProcessSupport::PipeOperation);
            BOOL synchronous = ConnectNamedPipe(pipe.Get(), &connect->operation);
            const DWORD connectError = synchronous ? ERROR_SUCCESS : GetLastError();
            Assert::IsTrue(synchronous || connectError == ERROR_IO_PENDING || connectError == ERROR_PIPE_CONNECTED);
            if (connectError != ERROR_PIPE_CONNECTED) {
                SetLastError(connectError);
                ProcessSupport::CompleteIo(pipe.Get(), connect, synchronous, 0, child.process.Get(), child.job.Get());
            }
            Assert::IsTrue(Receive().success != FALSE, L"Isolated engine initialization failed.");
        }

        ProcessChild(const ProcessChild &) = delete;
        ProcessChild & operator=(const ProcessChild &) = delete;

        void Begin(DWORD command)
        {
            Assert::IsFalse(pending, L"Only one request may be pending per helper.");
            std::unique_ptr<ProcessSupport::PipeOperation> operation(new ProcessSupport::PipeOperation);
            ProcessRequest request = {command};
            CopyMemory(operation->buffer, &request, sizeof(request));
            BOOL immediate = WriteFile(pipe.Get(), operation->buffer, sizeof(request), nullptr, &operation->operation);
            ProcessSupport::CompleteIo(pipe.Get(), operation, immediate, sizeof(request), child.process.Get(), child.job.Get());
            pending = true;
        }

        ProcessResult Receive()
        {
            ProcessResult result = {};
            std::unique_ptr<ProcessSupport::PipeOperation> operation(new ProcessSupport::PipeOperation);
            BOOL immediate = ReadFile(pipe.Get(), operation->buffer, sizeof(result), nullptr, &operation->operation);
            ProcessSupport::CompleteIo(pipe.Get(), operation, immediate, sizeof(result), child.process.Get(), child.job.Get());
            CopyMemory(&result, operation->buffer, sizeof(result));
            pending = false;
            return result;
        }

        ProcessResult Call(DWORD command) { Begin(command); return Receive(); }
        void WaitForReservation()
        {
            Assert::AreEqual<DWORD>(WAIT_OBJECT_0, WaitForSingleObject(entered.Get(), PROCESS_TEST_TIMEOUT));
        }
        void ReleaseReservation() { Assert::IsTrue(SetEvent(release.Get()) != FALSE); }
        void WaitForExit(DWORD expected = 0)
        {
            Assert::AreEqual<DWORD>(WAIT_OBJECT_0, WaitForSingleObject(child.process.Get(), PROCESS_TEST_TIMEOUT));
            DWORD actual = STILL_ACTIVE;
            Assert::IsTrue(GetExitCodeProcess(child.process.Get(), &actual) != FALSE);
            Assert::AreEqual(expected, actual);
        }
        void ExitNormally() { Begin(ProcessExitNormally); WaitForExit(); pending = false; }
        void Terminate()
        {
            Assert::IsTrue(TerminateProcess(child.process.Get(), 42) != FALSE);
            WaitForExit(42);
        }
    };

    class ProcessFixture
    {
        std::wstring directory;
        std::wstring engine;
        std::wstring helper;
        std::vector<std::unique_ptr<ProcessChild>> children;
    public:
        ProcessFixture()
        {
            const std::wstring binary = ProcessSupport::BinaryDirectory();
            directory = binary + L"\\process-" + ProcessSupport::UniqueName();
            Assert::IsTrue(CreateDirectoryW(directory.c_str(), nullptr) != FALSE);
            engine = directory + L"\\allsnap_process_engine.dll";
            helper = binary + L"\\allsnap_process_helper.exe";
            Assert::IsTrue(CopyFileW((binary + L"\\allsnap_process_engine.dll").c_str(), engine.c_str(), TRUE) != FALSE);
        }
        ~ProcessFixture()
        {
            children.clear();
            DeleteFileW(engine.c_str());
            RemoveDirectoryW(directory.c_str());
        }
        ProcessChild & Child()
        {
            std::unique_ptr<ProcessChild> child(new ProcessChild(helper, engine));
            children.push_back(std::move(child));
            return *children.back();
        }
    };
}
