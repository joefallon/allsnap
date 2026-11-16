#include "../support/EngineFixture.h"
#include <fstream>
#include <iterator>
#include <string>
extern "C" {
#include "snap_diagnostics.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
TEST_CLASS(DiagnosticsUnitTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD(FailureEventsAreAppendedWithTheirCapturedErrorCodes) {
        WCHAR directory[MAX_PATH], path[MAX_PATH];
        Assert::IsTrue(GetTempPathW(MAX_PATH,directory) != 0);
        Assert::IsTrue(GetTempFileNameW(directory,L"asd",0,path) != 0);
        BOOL first = SnapDiagnostics_TestRecord(path,SNAP_DIAG_HOOK_INSTALL,5);
        BOOL second = SnapDiagnostics_TestRecord(path,SNAP_DIAG_SUBCLASS_REMOVE,1400);
        std::ifstream input(path);
        std::string contents((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
        input.close(); DeleteFileW(path);
        Assert::IsTrue(first && second);
        Assert::IsTrue(contents.find("stage=1 error=5") != std::string::npos);
        Assert::IsTrue(contents.find("error=1400") != std::string::npos);
    }
    TEST_METHOD(UnwritableDiagnosticDestinationIsReportedAsFailure) {
        WCHAR directory[MAX_PATH];
        Assert::IsTrue(GetTempPathW(MAX_PATH,directory) != 0);
        Assert::IsFalse(SnapDiagnostics_TestRecord(directory,SNAP_DIAG_HOOK_INSTALL,5) != FALSE);
    }
};
}
