#include "../support/EngineFixture.h"
#include "snap_lifecycle_test.h"

extern "C" { void Subclass(HWND hwnd); }

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        bool installHookSucceeds, removeHookSucceeds, installSubclassSucceeds, removeSubclassSucceeds;
        bool tryShutdownDuringInstall, shutdownDuringInstall;
        int installCalls, removeCalls;
        const HHOOK fakeHook = reinterpret_cast<HHOOK>(static_cast<UINT_PTR>(123));

        HHOOK WINAPI InstallHook(int, HOOKPROC, HINSTANCE, DWORD)
        {
            if (!installHookSucceeds) { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
            return fakeHook;
        }
        BOOL WINAPI RemoveHook(HHOOK)
        {
            if (!removeHookSucceeds) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
            return TRUE;
        }
        BOOL WINAPI InstallSubclass(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR)
        {
            ++installCalls;
            if (tryShutdownDuringInstall) shutdownDuringInstall = SnapUnHookAll() != FALSE;
            if (!installSubclassSucceeds) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
            return TRUE;
        }
        BOOL WINAPI RemoveSubclass(HWND, SUBCLASSPROC, UINT_PTR)
        {
            ++removeCalls;
            if (!removeSubclassSucceeds) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
            return TRUE;
        }
        DWORD WINAPI WrongThreadCleanup(void * window)
        {
            return SnapTest_UnSubclass(static_cast<HWND>(window));
        }
        BOOL WINAPI PinFails()
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
        int WINAPI OwnerAccessUnknown(DWORD,DWORD,ULONGLONG,ULONGLONG,BOOL)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return SNAP_OWNER_UNKNOWN;
        }
        DWORD WINAPI FinishRegistrationThread(void *)
        {
            HWND owned = CreateWindowEx(0,L"STATIC",L"allSnap finished-thread fixture",WS_POPUP,
                -30000,-30000,320,240,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
            if (!owned) return FALSE;
            BOOL registered = SnapTest_Subclass(owned);
            // The fake native operation intentionally has no WM_NCDESTROY callback.
            DestroyWindow(owned);
            return registered;
        }
    }

    TEST_CLASS(LifecycleUnitTests)
    {
        HWND window = nullptr;
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            SnapTest_LifecycleReset();
            installHookSucceeds = removeHookSucceeds = installSubclassSucceeds = removeSubclassSucceeds = true;
            tryShutdownDuringInstall = shutdownDuringInstall = false;
            installCalls = removeCalls = 0;
            SNAP_LIFECYCLE_TEST_OPS operations = {InstallHook, RemoveHook, InstallSubclass, RemoveSubclass};
            SnapTest_SetLifecycleOps(&operations);
            window = CreateWindowEx(0, L"STATIC", L"allSnap lifecycle unit fixture", WS_POPUP,
                -30000, -30000, 320, 240, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
            Assert::IsNotNull(window);
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            removeSubclassSucceeds = true;
            SnapTest_UnSubclass(window);
            if (window) DestroyWindow(window);
            SnapTest_LifecycleReset();
        }
        bool StartHook()
        {
            OSVERSIONINFO version = {};
            version.dwOSVersionInfoSize = sizeof(version);
            return SnapHookAll(window, 0, version) != FALSE;
        }

        TEST_METHOD(FailedSubclassInstallationDoesNotBlockExit)
        {
            // This failed against the original production Subclass implementation.
            Subclass(nullptr);
            Assert::IsTrue(SnapCanUnHook() != FALSE,
                L"A failed subclass installation must not leave the application busy.");
        }
        TEST_METHOD(FailedSetWindowSubclassRollsBackReservation)
        {
            installSubclassSucceeds = false;
            Assert::IsFalse(SnapTest_Subclass(window) != FALSE);
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED), GetLastError());
            Assert::IsTrue(SnapCanUnHook() != FALSE);
            Assert::AreEqual(0L, SnapTest_ActiveSubclassCount());
            Assert::IsNull(SnapTest_SubclassWindow());
        }
        TEST_METHOD(DuplicateRegistrationCountsOnlyOnce)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::AreEqual(1, installCalls);
            Assert::AreEqual(1L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(FailedRemovalRetainsActiveOwnership)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            removeSubclassSucceeds = false;
            Assert::IsFalse(SnapTest_UnSubclass(window) != FALSE);
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED), GetLastError());
            Assert::IsFalse(SnapCanUnHook() != FALSE);
            Assert::IsTrue(SnapTest_SubclassWindow() == window);
            Assert::AreEqual(1L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(UnrelatedWindowCannotReleaseOwnedSubclass)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsFalse(SnapTest_UnSubclass(GetDesktopWindow()) != FALSE);
            Assert::AreEqual(0, removeCalls);
            Assert::AreEqual(1L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(RepeatedCleanupDoesNotDecrementTwice)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsTrue(SnapTest_UnSubclass(window) != FALSE);
            Assert::IsTrue(SnapTest_UnSubclass(window) != FALSE);
            Assert::AreEqual(1, removeCalls);
            Assert::AreEqual(0L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(DetachWithoutOwnershipPreservesOtherRegistrations)
        {
            SnapTest_SetExternalSubclassCount(2);
            SnapTest_ProcessDetach();
            Assert::AreEqual(2L, SnapTest_ActiveSubclassCount());
            Assert::IsFalse(SnapCanUnHook() != FALSE);
            Assert::AreEqual(0, removeCalls);
        }
        TEST_METHOD(DetachReleasesOnlyLocalRegistration)
        {
            SnapTest_SetExternalSubclassCount(2);
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            SnapTest_ProcessDetach();
            SnapTest_ProcessDetach();
            Assert::AreEqual(2L, SnapTest_ActiveSubclassCount());
            Assert::AreEqual(0, removeCalls, L"Detach bookkeeping must not call Comctl32 under loader lock.");
        }
        TEST_METHOD(DestroyedWindowReleasesOwnership)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            SnapTest_WindowDestroyed(window);
            Assert::AreEqual(0L, SnapTest_ActiveSubclassCount());
            Assert::IsTrue(SnapCanUnHook() != FALSE);
        }
        TEST_METHOD(WrongThreadCannotRemoveOwnedSubclass)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            HANDLE thread = CreateThread(nullptr, 0, WrongThreadCleanup, window, 0, nullptr);
            Assert::IsNotNull(thread);
            DWORD wait = WaitForSingleObject(thread, 5000);
            DWORD result = TRUE;
            GetExitCodeThread(thread, &result);
            CloseHandle(thread);
            Assert::AreEqual(static_cast<DWORD>(WAIT_OBJECT_0), wait);
            Assert::AreEqual(static_cast<DWORD>(FALSE), result);
            Assert::AreEqual(0, removeCalls);
            Assert::AreEqual(1L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(NegativeHookCodeForwardsWithoutReadingPayload)
        {
            SnapTest_CallHook(-1, 0, 0);
            Assert::AreEqual(0, installCalls);
            Assert::AreEqual(0L, SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(DefaultTestOperationsCannotInstallGlobalHook)
        {
            SnapTest_SetLifecycleOps(nullptr);
            Assert::IsFalse(StartHook());
            Assert::AreEqual(static_cast<DWORD>(ERROR_NOT_SUPPORTED), GetLastError());
            Assert::IsNull(SnapTest_HookHandle());
        }
        TEST_METHOD(FailedHookInstallPreservesErrorWithoutHandle)
        {
            installHookSucceeds = false;
            Assert::IsFalse(StartHook());
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED), GetLastError());
            Assert::IsNull(SnapTest_HookHandle());
            Assert::IsTrue(SnapCanUnHook() != FALSE);
        }
        TEST_METHOD(FailedUnhookRetainsHandleForRetry)
        {
            Assert::IsTrue(StartHook());
            removeHookSucceeds = false;
            Assert::IsFalse(SnapUnHookAll() != FALSE);
            Assert::IsTrue(SnapTest_HookHandle() == fakeHook);
            removeHookSucceeds = true;
            Assert::IsTrue(SnapUnHookAll() != FALSE);
            Assert::IsNull(SnapTest_HookHandle());
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
        TEST_METHOD(ActiveRegistrationBlocksUnhook)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsFalse(SnapUnHookAll() != FALSE);
            Assert::AreEqual(static_cast<DWORD>(ERROR_BUSY), GetLastError());
            Assert::IsTrue(SnapTest_HookHandle() == fakeHook);
        }
        TEST_METHOD(InFlightRegistrationBlocksUnhook)
        {
            Assert::IsTrue(StartHook());
            tryShutdownDuringInstall = true;
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsFalse(shutdownDuringInstall);
            Assert::IsTrue(SnapTest_HookHandle() == fakeHook);
        }
        TEST_METHOD(StoppingGatePreventsNewSubclass)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapUnHookAll() != FALSE);
            Assert::IsFalse(SnapTest_Subclass(window) != FALSE);
            Assert::AreEqual(0, installCalls);
        }
        TEST_METHOD(NewHookSessionReopensRegistrationGate)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapUnHookAll() != FALSE);
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
        }
        TEST_METHOD(PinFailureCannotAdmitAnUnsafeSubclass)
        {
            SNAP_LIFECYCLE_TEST_OPS operations = {
                InstallHook,RemoveHook,InstallSubclass,RemoveSubclass,PinFails,nullptr};
            SnapTest_SetLifecycleOps(&operations);
            Assert::IsFalse(SnapTest_Subclass(window) != FALSE);
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED),GetLastError());
            Assert::AreEqual(0,installCalls);
            Assert::AreEqual(0L,SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(UnknownOwnerAccessDoesNotAuthorizeReaping)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            SNAP_LIFECYCLE_TEST_OPS operations = {
                InstallHook,RemoveHook,InstallSubclass,RemoveSubclass,nullptr,OwnerAccessUnknown};
            SnapTest_SetLifecycleOps(&operations);
            Assert::IsFalse(SnapCanUnHook() != FALSE);
            Assert::IsFalse(SnapUnHookAll() != FALSE);
            Assert::AreEqual(1L,SnapTest_ActiveSubclassCount());
            Assert::IsTrue(SnapTest_SubclassWindow() == window);
            Assert::IsTrue(SnapTest_UnSubclass(window) != FALSE);
        }
        TEST_METHOD(ProcessBirthMismatchReapsOldIdentityAndReconcilesLocalAdmission)
        {
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            SnapTest_CorruptLocalOwnerBirth();
            Assert::IsTrue(SnapCanUnHook() != FALSE);
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::AreEqual(2,installCalls);
            Assert::AreEqual(1L,SnapTest_ActiveSubclassCount());
        }
        TEST_METHOD(FinishedOwnerThreadDoesNotStrandLocalAdmission)
        {
            HANDLE thread = CreateThread(nullptr,0,FinishRegistrationThread,nullptr,0,nullptr);
            Assert::IsNotNull(thread);
            DWORD wait = WaitForSingleObject(thread,5000),result = FALSE;
            GetExitCodeThread(thread,&result);
            Assert::AreEqual(static_cast<DWORD>(WAIT_OBJECT_0),wait);
            Assert::AreEqual(static_cast<DWORD>(TRUE),result);
            // Keep the terminated thread's handle open while the real liveness probe runs.
            BOOL admitted = SnapTest_Subclass(window);
            CloseHandle(thread);
            Assert::IsTrue(admitted != FALSE);
            Assert::AreEqual(2,installCalls);
            Assert::AreEqual(0,removeCalls,L"A different thread must not attempt native removal for the dead owner.");
            Assert::AreEqual(1L,SnapTest_ActiveSubclassCount());
        }
    };
}
