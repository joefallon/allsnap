#include "../support/EngineFixture.h"
#include "snap_lifecycle_test.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
namespace {
    int silentInstalls, silentRemovals;
    HHOOK WINAPI SilentInstall(int, HOOKPROC, HINSTANCE, DWORD) {
        ++silentInstalls; return reinterpret_cast<HHOOK>(static_cast<UINT_PTR>(123));
    }
    BOOL WINAPI SilentRemove(HHOOK) { ++silentRemovals; return TRUE; }
}
TEST_CLASS(SilentLifecycleAcceptanceTests) {
    HWND window = nullptr;
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        ResetEngine(); SnapTest_LifecycleReset(); silentInstalls = silentRemovals = 0;
        SNAP_LIFECYCLE_TEST_OPS operations = {SilentInstall, SilentRemove};
        SnapTest_SetLifecycleOps(&operations);
        window = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"allSnap silent lifecycle fixture",
            WS_POPUP, -30000, -30000, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        Assert::IsNotNull(window);
    }
    TEST_METHOD_CLEANUP(Cleanup) {
        SnapTest_UnSubclass(window); SnapUnHookAll();
        if (IsWindow(window)) DestroyWindow(window);
        SnapTest_LifecycleReset(); ResetEngine();
    }
    TEST_METHOD(RetiredAudioSettingsAreAbsentFromTheEngineApi) {
        HMODULE module = nullptr;
        Assert::IsTrue(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&TrackRslts_GetLast), &module) != FALSE);
        for (const char * name : {"setNoisy", "isNoisy", "_setNoisy@4", "_isNoisy@0"})
            Assert::IsTrue(GetProcAddress(module, name) == nullptr, L"The audio API must be removed.");
    }
    TEST_METHOD(RepeatedSilentSessionsRetainNativeOwnershipUntilCleanup) {
        OSVERSIONINFO version = {}; version.dwOSVersionInfoSize = sizeof(version);
        for (int cycle = 0; cycle < 8; ++cycle) {
            Assert::IsTrue(SnapHookAll(window, version) != FALSE);
            Assert::IsTrue(SnapTest_Subclass(window) != FALSE);
            Assert::IsFalse(SnapCanUnHook() != FALSE);
            Assert::IsFalse(SnapUnHookAll() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_BUSY, GetLastError());
            Assert::IsTrue(SnapTest_UnSubclass(window) != FALSE);
            Assert::IsTrue(SnapCanUnHook() != FALSE);
            Assert::IsTrue(SnapUnHookAll() != FALSE);
        }
        Assert::AreEqual(8, silentInstalls); Assert::AreEqual(8, silentRemovals);
    }
};
}
