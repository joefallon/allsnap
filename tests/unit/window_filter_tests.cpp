#include "../support/EngineFixture.h"
#include <string>
extern "C" {
#include "snap_MultiSz.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AllSnapTests {
TEST_CLASS(WindowFilterUnitTests) {
    std::wstring ignored, skinned;
    int cropTop;
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        ignored = getIgnoredClasses(); skinned = getSkinnedClasses(); cropTop = getCropTop();
    }
    TEST_METHOD_CLEANUP(Cleanup) {
        setIgnoredClasses(&ignored[0], static_cast<int>(ignored.size()));
        setSkinnedClasses(&skinned[0], static_cast<int>(skinned.size()));
        setCropTop(cropTop);
    }
    TEST_METHOD(ClassListsMatchCompleteSemicolonDelimitedTokens) {
        Assert::IsTrue(is_name_in_classlist(L"Second", L";First;;Second;Third;") != FALSE);
        Assert::IsFalse(is_name_in_classlist(L"Sec", L"First;Second;Third") != FALSE);
        Assert::IsFalse(is_name_in_classlist(L"SecondExtra", L"First;Second;Third") != FALSE);
        Assert::IsFalse(is_name_in_classlist(L"Missing", L"") != FALSE);
    }
    TEST_METHOD(NegativeIgnoredListLengthCannotReplaceExistingSetting) {
        WCHAR initial[] = L"Retained"; WCHAR invalid[] = L"Replacement";
        setIgnoredClasses(initial, 8); setIgnoredClasses(invalid, -1);
        Assert::AreEqual(L"Retained", getIgnoredClasses());
    }
    TEST_METHOD(NegativeSkinnedListLengthCannotReplaceExistingSetting) {
        WCHAR initial[] = L"Retained"; WCHAR invalid[] = L"Replacement";
        setSkinnedClasses(initial, 8); setSkinnedClasses(invalid, -1);
        Assert::AreEqual(L"Retained", getSkinnedClasses());
    }
    TEST_METHOD(EmptyClassSettingsClearBothLists) {
        WCHAR empty[] = L"";
        setIgnoredClasses(empty, 0); setSkinnedClasses(empty, 0);
        Assert::AreEqual(L"", getIgnoredClasses());
        Assert::AreEqual(L"", getSkinnedClasses());
    }
    TEST_METHOD(NegativeManualCropCannotReplaceANonnegativeSetting) {
        setCropTop(30); setCropTop(-1);
        Assert::AreEqual(30, getCropTop());
    }
};
}
