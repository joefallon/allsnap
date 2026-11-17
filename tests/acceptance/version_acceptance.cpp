#include "stdafx.h"
#include "CppUnitTest.h"
#include "allsnap_version.h"
#include "resource.h"
#include <winver.h>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

extern "C" {
extern HINSTANCE g_hInst;
void TestApp_Initialize(void);
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        class AppVersionResource
        {
            std::vector<BYTE> bytes;
        public:
            AppVersionResource()
            {
                TestApp_Initialize();
                WCHAR modulePath[MAX_PATH] = {};
                const DWORD pathLength = GetModuleFileNameW(g_hInst,modulePath,chDIMOF(modulePath));
                Assert::IsTrue(pathLength > 0 && pathLength < chDIMOF(modulePath));
                DWORD ignored = 0;
                const DWORD byteCount = GetFileVersionInfoSizeW(modulePath,&ignored);
                Assert::IsTrue(byteCount > 0,L"Read the production app VERSIONINFO embedded in the test DLL.");
                bytes.resize(byteCount);
                Assert::IsTrue(GetFileVersionInfoW(modulePath,0,byteCount,bytes.data()) != FALSE);
            }

            VS_FIXEDFILEINFO Fixed() const
            {
                void * value = nullptr;
                UINT size = 0;
                Assert::IsTrue(VerQueryValueW(bytes.data(),L"\\",&value,&size) != FALSE);
                Assert::IsNotNull(value);
                Assert::IsTrue(size >= sizeof(VS_FIXEDFILEINFO));
                VS_FIXEDFILEINFO result = {};
                std::memcpy(&result,value,sizeof(result));
                return result;
            }

            std::wstring Text(LPCWSTR name) const
            {
                struct Translation { WORD language, codePage; } translation = {};
                void * value = nullptr;
                UINT size = 0;
                Assert::IsTrue(VerQueryValueW(bytes.data(),L"\\VarFileInfo\\Translation",&value,&size) != FALSE);
                Assert::IsTrue(size >= sizeof(translation));
                std::memcpy(&translation,value,sizeof(translation));
                WCHAR query[128] = {};
                swprintf_s(query,chDIMOF(query),L"\\StringFileInfo\\%04x%04x\\%ls",
                    translation.language,translation.codePage,name);
                Assert::IsTrue(VerQueryValueW(bytes.data(),query,&value,&size) != FALSE);
                Assert::IsNotNull(value);
                Assert::IsTrue(size > 0);
                return std::wstring(static_cast<LPCWSTR>(value),size - 1);
            }
        };

        INT_PTR CALLBACK HiddenAboutProcedure(HWND dialog,UINT message,WPARAM,LPARAM)
        {
            if (message != WM_INITDIALOG) return FALSE;
            SetWindowLongPtrW(dialog,GWL_EXSTYLE,GetWindowLongPtrW(dialog,GWL_EXSTYLE) |
                WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
            return TRUE;
        }

        class OwnedAboutDialog
        {
            HWND window;
        public:
            explicit OwnedAboutDialog(LPCWSTR resource)
                : window(CreateDialogParamW(g_hInst,resource,nullptr,HiddenAboutProcedure,0))
            {
                Assert::IsTrue(IsWindow(window) != FALSE,L"The production About template must create.");
                Assert::IsFalse(IsWindowVisible(window) != FALSE);
            }
            ~OwnedAboutDialog() { if (window) DestroyWindow(window); }
            HWND Window() const { return window; }
        };

        BOOL CALLBACK CaptureChildText(HWND child,LPARAM context)
        {
            WCHAR text[256] = {};
            const int length = GetWindowTextW(child,text,chDIMOF(text));
            if (length > 0)
                reinterpret_cast<std::vector<std::wstring> *>(context)->emplace_back(text,length);
            return TRUE;
        }
    }

    TEST_CLASS(VersionAcceptanceTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD(CompiledAppResourceHasTheMatchingFixedFileAndProductVersions)
        {
            AppVersionResource resource;
            const VS_FIXEDFILEINFO fixed = resource.Fixed();
            const WORD expected[] = {ALLSNAP_VERSION_RESOURCE};
            Assert::AreEqual<DWORD>(VS_FFI_SIGNATURE,fixed.dwSignature);
            Assert::AreEqual<DWORD>(MAKELONG(expected[1],expected[0]),fixed.dwFileVersionMS);
            Assert::AreEqual<DWORD>(MAKELONG(expected[3],expected[2]),fixed.dwFileVersionLS);
            Assert::AreEqual(fixed.dwFileVersionMS,fixed.dwProductVersionMS);
            Assert::AreEqual(fixed.dwFileVersionLS,fixed.dwProductVersionLS);
        }

        TEST_METHOD(CompiledAppResourceReportsTheSemanticReleaseText)
        {
            AppVersionResource resource;
            const std::wstring expected = _T(ALLSNAP_VERSION_TEXT);
            Assert::AreEqual(expected.c_str(),resource.Text(L"FileVersion").c_str());
            Assert::AreEqual(expected.c_str(),resource.Text(L"ProductVersion").c_str());
            Assert::AreEqual(L"allSnap",resource.Text(L"ProductName").c_str());
        }

        TEST_METHOD(CompiledAppResourceDoesNotMarkTheReleaseAsBetaOrSpecialBuild)
        {
            AppVersionResource resource;
            const VS_FIXEDFILEINFO fixed = resource.Fixed();
#ifdef _DEBUG
            const DWORD expectedFlags = VS_FF_DEBUG;
#else
            const DWORD expectedFlags = 0;
#endif
            Assert::AreEqual(expectedFlags,fixed.dwFileFlags);
        }

        TEST_METHOD(ProductionAboutTemplatesDisplayTheReleaseWithoutBeta)
        {
            TestApp_Initialize();
            LPCWSTR templates[] = {MAKEINTRESOURCEW(IDD_ABOUT),L"IDD_ABOUT_DEBUG"};
            const std::wstring expected = L"allSnap Version " _T(ALLSNAP_VERSION_TEXT);
            for (LPCWSTR resource : templates) {
                OwnedAboutDialog dialog(resource);
                std::vector<std::wstring> text;
                EnumChildWindows(dialog.Window(),CaptureChildText,reinterpret_cast<LPARAM>(&text));
                bool versionShown = false;
                for (const std::wstring & label : text) {
                    if (label.find(expected) != std::wstring::npos) versionShown = true;
                    Assert::IsTrue(label.find(L"Beta") == std::wstring::npos,
                        L"Release About text must not retain the old Beta label.");
                }
                Assert::IsTrue(versionShown,L"Show the release version in an actual About control.");
            }
        }
    };
}
