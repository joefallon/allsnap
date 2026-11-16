#include "../support/EngineFixture.h"
#include "snap_keys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    namespace
    {
        SHORT suppliedState;
        int requestedKey;
        SHORT WINAPI ReadSuppliedState(int key)
        {
            requestedKey = key;
            return suppliedState;
        }
    }

    TEST_CLASS(InputUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD_INITIALIZE(Initialize)
        {
            suppliedState = 0;
            requestedKey = 0;
            SnapKeys_TestSetReader(ReadSuppliedState);
        }
        TEST_METHOD_CLEANUP(Cleanup) { SnapKeys_TestSetReader(nullptr); }

        TEST_METHOD(ReleasedKeyIsNotDown)
        {
            Assert::IsFalse(SnapKeys_IsDown(VK_MENU) != FALSE);
        }
        TEST_METHOD(HighBitIdentifiesHeldKey)
        {
            suppliedState = static_cast<SHORT>(0x8000);
            Assert::IsTrue(SnapKeys_IsDown(VK_MENU) != FALSE);
        }
        TEST_METHOD(RecentPressBitAloneDoesNotMeanKeyIsStillHeld)
        {
            suppliedState = 0x0001;
            Assert::IsFalse(SnapKeys_IsDown(VK_MENU) != FALSE);
        }
        TEST_METHOD(HighBitRemainsHeldWhenRecentPressBitIsAlsoSet)
        {
            suppliedState = static_cast<SHORT>(0x8001);
            Assert::IsTrue(SnapKeys_IsDown(VK_CONTROL) != FALSE);
        }
        TEST_METHOD(ConfiguredVirtualKeyIsPassedToReader)
        {
            SnapKeys_IsDown(VK_TAB);
            Assert::AreEqual(static_cast<int>(VK_TAB), requestedKey);
        }
    };
}
