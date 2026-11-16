#include "../support/EngineFixture.h"
#include "snap_lifecycle_test.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        HHOOK WINAPI AcceptanceInstallHook(int, HOOKPROC, HINSTANCE, DWORD)
        {
            return reinterpret_cast<HHOOK>(static_cast<UINT_PTR>(321));
        }
        BOOL WINAPI AcceptanceRemoveHook(HHOOK) { return TRUE; }
        HHOOK WINAPI AcceptanceFailedHook(int, HOOKPROC, HINSTANCE, DWORD)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return nullptr;
        }
        BOOL WINAPI AcceptanceFailedSubclass(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    TEST_CLASS(LifecycleAcceptanceTests)
    {
        HWND window = nullptr;
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()
        TEST_METHOD_INITIALIZE(Initialize)
        {
            ResetEngine();
            SnapTest_LifecycleReset();
            window = CreateWindowEx(0, L"STATIC", L"allSnap lifecycle acceptance fixture", WS_POPUP,
                -30000, -30000, 320, 240, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
            Assert::IsNotNull(window);
            // Only hook installation is substituted; real subclass callbacks run on our hidden window.
            SNAP_LIFECYCLE_TEST_OPS operations = {AcceptanceInstallHook, AcceptanceRemoveHook, nullptr, nullptr};
            SnapTest_SetLifecycleOps(&operations);
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            if (window) {
                SnapTest_UnSubclass(window);
                DestroyWindow(window);
            }
            SnapTest_LifecycleReset();
        }
        bool StartHook()
        {
            OSVERSIONINFO version = {};
            version.dwOSVersionInfoSize = sizeof(version);
            return SnapHookAll(window, version) != FALSE;
        }
        TEST_METHOD(HL001HookFailureIsReturnedToStartupCaller)
        {
            SNAP_LIFECYCLE_TEST_OPS operations = {AcceptanceFailedHook, nullptr, nullptr, nullptr};
            SnapTest_SetLifecycleOps(&operations);
            Assert::IsFalse(StartHook());
            Assert::AreEqual(static_cast<DWORD>(ERROR_ACCESS_DENIED), GetLastError());
            Assert::IsNull(SnapTest_HookHandle());
            Assert::IsTrue(SnapCanUnHook() != FALSE);
        }
        TEST_METHOD(HL002ConsecutiveMoveSessionsReleaseNativeSubclass)
        {
            Assert::IsTrue(StartHook());
            for (int gesture = 0; gesture < 5; ++gesture) {
                Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
                Assert::IsFalse(SnapCanUnHook() != FALSE);
                SendMessage(window, WM_EXITSIZEMOVE, 0, 0);
                Assert::IsTrue(SnapCanUnHook() != FALSE);
            }
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
        TEST_METHOD(HL003DestroyedWindowReleasesNativeSubclass)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            DestroyWindow(window);
            window = nullptr;
            Assert::IsTrue(SnapCanUnHook() != FALSE);
            Assert::AreEqual(0L, SnapTest_ActiveSubclassCount());
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
        TEST_METHOD(HL004ShutdownWaitsForActiveMoveSession)
        {
            Assert::IsTrue(StartHook());
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsFalse(SnapUnHookAll() != FALSE);
            SendMessage(window, WM_EXITSIZEMOVE, 0, 0);
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
        TEST_METHOD(HL005FailedSubclassDoesNotBlockLaterShutdown)
        {
            SNAP_LIFECYCLE_TEST_OPS operations = {
                AcceptanceInstallHook, AcceptanceRemoveHook, AcceptanceFailedSubclass, nullptr};
            SnapTest_SetLifecycleOps(&operations);
            Assert::IsTrue(StartHook());
            Assert::IsFalse(SnapTest_Subclass(window) != FALSE);
            Assert::IsTrue(SnapCanUnHook() != FALSE);
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
    };
}
