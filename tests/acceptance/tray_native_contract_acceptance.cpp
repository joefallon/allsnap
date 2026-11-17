#include "stdafx.h"
#include "../support/SettingsFixture.h"
#include <shellapi.h>
#include <algorithm>
#include <cstring>
#include <iterator>

extern "C" {
extern HWND g_hWnd;
int TestApp_NotifyCount(void);
DWORD TestApp_NotifyOperation(int);
const NOTIFYICONDATA * TestApp_NotifyData(int);
void TestApp_SetNotifyFailure(DWORD);
void TestApp_ClearNotifyFailure(void);
void SnapTaskbar_TestSetResourceOps(
    int (WINAPI *)(HINSTANCE,UINT,LPTSTR,int),
    HANDLE (WINAPI *)(HINSTANCE,LPCTSTR,UINT,int,int,UINT),
    BOOL (WINAPI *)(HICON));
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        std::wstring tooltip;
        bool missingTooltip, missingIcon, changeCleanupError;
        int requestedTooltipCapacity, imageLoads, destroyedIcons;
        HICON testIcon = reinterpret_cast<HICON>(static_cast<ULONG_PTR>(0x1234));

        int WINAPI LoadTooltip(HINSTANCE, UINT, LPTSTR text, int capacity)
        {
            requestedTooltipCapacity = capacity;
            if (missingTooltip) {
                if (capacity > 0) text[0] = L'\0';
                SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
                return 0;
            }
            lstrcpynW(text, tooltip.c_str(), capacity);
            return lstrlenW(text);
        }

        HANDLE WINAPI LoadTrayImage(HINSTANCE, LPCTSTR, UINT, int, int, UINT)
        {
            ++imageLoads;
            if (missingIcon) {
                SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
                return nullptr;
            }
            return testIcon;
        }

        BOOL WINAPI ReleaseTrayImage(HICON icon)
        {
            Assert::IsTrue(icon == testIcon, L"Release only the owned tray image.");
            ++destroyedIcons;
            if (changeCleanupError) SetLastError(ERROR_INVALID_HANDLE);
            return TRUE;
        }

        class TrayContractFixture
        {
            SettingsFixture settings;
        public:
            TrayContractFixture()
            {
                tooltip = L"allSnap enabled";
                missingTooltip = missingIcon = changeCleanupError = false;
                requestedTooltipCapacity = imageLoads = destroyedIcons = 0;
                SnapTaskbar_TestSetResourceOps(LoadTooltip,LoadTrayImage,ReleaseTrayImage);
            }
            ~TrayContractFixture() { SnapTaskbar_TestSetResourceOps(nullptr,nullptr,nullptr); }
        };

        template <size_t Count>
        void AssertEmptyText(const WCHAR (&text)[Count])
        {
            Assert::IsTrue(std::all_of(std::begin(text),std::end(text),
                [](WCHAR character) { return character == L'\0'; }));
        }

        void AssertUnusedNotificationFieldsAreEmpty(const NOTIFYICONDATA & data)
        {
            Assert::AreEqual<DWORD>(sizeof(data),data.cbSize);
            Assert::IsTrue(data.hWnd == g_hWnd);
            Assert::AreEqual<UINT>(IDT_MYTRAY,data.uID);
            Assert::AreEqual<DWORD>(0,data.dwState);
            Assert::AreEqual<DWORD>(0,data.dwStateMask);
            AssertEmptyText(data.szInfo);
            Assert::AreEqual<UINT>(0,data.uTimeout);
            AssertEmptyText(data.szInfoTitle);
            Assert::AreEqual<DWORD>(0,data.dwInfoFlags);
            GUID emptyGuid = {};
            Assert::IsTrue(std::memcmp(&emptyGuid,&data.guidItem,sizeof(emptyGuid)) == 0);
        }
    }

    TEST_CLASS(TrayNativeContractAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        // TR-005: Explorer receives only fields valid for each operation.
        TEST_METHOD(NativePacketsContainOnlyOperationData)
        {
            TrayContractFixture fixture;
            Assert::IsTrue(AddEmptyTaskbarIcon() != FALSE);
            Assert::IsTrue(ResetTaskbarIcon() != FALSE);
            Assert::IsTrue(DeleteTaskbarIcon() != FALSE);
            Assert::AreEqual(3,TestApp_NotifyCount());
            for (int index = 0; index < 3; ++index)
                AssertUnusedNotificationFieldsAreEmpty(*TestApp_NotifyData(index));
            const NOTIFYICONDATA & added = *TestApp_NotifyData(0);
            Assert::AreEqual<UINT>(NIF_MESSAGE,added.uFlags);
            Assert::AreEqual<UINT>(MYWM_NOTIFYICON,added.uCallbackMessage);
            Assert::IsNull(added.hIcon);
            AssertEmptyText(added.szTip);
            const NOTIFYICONDATA & refreshed = *TestApp_NotifyData(1);
            Assert::AreEqual<UINT>(NIF_ICON | NIF_TIP,refreshed.uFlags);
            Assert::AreEqual<UINT>(0,refreshed.uCallbackMessage);
            Assert::IsTrue(refreshed.hIcon == testIcon);
            const NOTIFYICONDATA & removed = *TestApp_NotifyData(2);
            Assert::AreEqual<UINT>(0,removed.uFlags);
            Assert::AreEqual<UINT>(0,removed.uCallbackMessage);
            Assert::IsNull(removed.hIcon);
            AssertEmptyText(removed.szTip);
        }

        // TR-006: a long localized string fills the native character capacity safely.
        TEST_METHOD(LongLocalizedTooltipUsesFullCharacterCapacity)
        {
            TrayContractFixture fixture;
            tooltip.assign(200,L'X');
            Assert::IsTrue(ResetTaskbarIcon() != FALSE);
            const NOTIFYICONDATA & packet = *TestApp_NotifyData(0);
            const size_t capacity = sizeof(packet.szTip) / sizeof(packet.szTip[0]);
            Assert::AreEqual(static_cast<int>(capacity),requestedTooltipCapacity);
            Assert::AreEqual(tooltip.substr(0,capacity - 1).c_str(),packet.szTip);
            Assert::AreEqual<WCHAR>(L'\0',packet.szTip[capacity - 1]);
        }

        // TR-006: failed resources must not publish an invalid icon or tooltip.
        TEST_METHOD(MissingTooltipDoesNotPublishAnInvalidRefresh)
        {
            TrayContractFixture fixture;
            missingTooltip = true;
            Assert::IsFalse(ResetTaskbarIcon() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_RESOURCE_NAME_NOT_FOUND,GetLastError());
            Assert::AreEqual(0,TestApp_NotifyCount());
            Assert::AreEqual(0,imageLoads);
            missingTooltip = false;
            Assert::IsTrue(ResetTaskbarIcon() != FALSE);
            Assert::AreEqual(1,TestApp_NotifyCount());
        }

        TEST_METHOD(MissingIconDoesNotPublishAnInvalidRefresh)
        {
            TrayContractFixture fixture;
            missingIcon = true;
            Assert::IsFalse(ResetTaskbarIcon() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_RESOURCE_DATA_NOT_FOUND,GetLastError());
            Assert::AreEqual(0,TestApp_NotifyCount());
            Assert::AreEqual(0,destroyedIcons);
            missingIcon = false;
            Assert::IsTrue(ResetTaskbarIcon() != FALSE);
            Assert::AreEqual(1,TestApp_NotifyCount());
            Assert::AreEqual(1,destroyedIcons);
        }

        // TR-007: cleanup must preserve the failure that caused refresh to fail.
        TEST_METHOD(FailedModifyReleasesIconWithoutLosingTheShellError)
        {
            TrayContractFixture fixture;
            changeCleanupError = true;
            TestApp_SetNotifyFailure(NIM_MODIFY);
            Assert::IsFalse(ResetTaskbarIcon() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_ACCESS_DENIED,GetLastError());
            Assert::AreEqual(1,destroyedIcons);
            Assert::AreEqual(1,TestApp_NotifyCount());
        }

        TEST_METHOD(FailedInitialRefreshRemovesEmptyIconAndCanRetry)
        {
            TrayContractFixture fixture;
            changeCleanupError = true;
            TestApp_SetNotifyFailure(NIM_MODIFY);
            Assert::IsFalse(AddTaskbarIcon() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_ACCESS_DENIED,GetLastError());
            Assert::AreEqual(3,TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_ADD,TestApp_NotifyOperation(0));
            Assert::AreEqual<DWORD>(NIM_MODIFY,TestApp_NotifyOperation(1));
            Assert::AreEqual<DWORD>(NIM_DELETE,TestApp_NotifyOperation(2));
            Assert::IsFalse(isIconHidden() != FALSE);
            TestApp_ClearNotifyFailure();
            Assert::IsTrue(AddTaskbarIcon() != FALSE);
            Assert::AreEqual(5,TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_ADD,TestApp_NotifyOperation(3));
            Assert::AreEqual<DWORD>(NIM_MODIFY,TestApp_NotifyOperation(4));
        }

        TEST_METHOD(FailedRollbackKeepsTheOriginalResourceError)
        {
            TrayContractFixture fixture;
            missingTooltip = true;
            TestApp_SetNotifyFailure(NIM_DELETE);
            Assert::IsFalse(AddTaskbarIcon() != FALSE);
            Assert::AreEqual<DWORD>(ERROR_RESOURCE_NAME_NOT_FOUND,GetLastError());
            Assert::AreEqual(2,TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_DELETE,TestApp_NotifyOperation(1));
            Assert::IsFalse(isIconHidden() != FALSE);
        }

        TEST_METHOD(HidePreferenceRemainsIndependentOfShellFailures)
        {
            TrayContractFixture fixture;
            TestApp_SetNotifyFailure(NIM_DELETE);
            setIconHidden(TRUE);
            Assert::IsTrue(isIconHidden() != FALSE);
            RestartTaskbarIcon();
            Assert::AreEqual(1,TestApp_NotifyCount());
            TestApp_ClearNotifyFailure();
            missingIcon = true;
            setIconHidden(FALSE);
            Assert::IsFalse(isIconHidden() != FALSE);
            Assert::AreEqual(3,TestApp_NotifyCount());
            missingIcon = false;
            RestartTaskbarIcon();
            Assert::AreEqual(5,TestApp_NotifyCount());
            Assert::AreEqual<DWORD>(NIM_MODIFY,TestApp_NotifyOperation(4));
        }
    };
}
