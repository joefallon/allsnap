#include "stdafx.h"
#include "CppUnitTest.h"
#include "allsnap_version.h"
#include <cstdio>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(VersionUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD(SemanticVersionTextMatchesWindowsVersionTuple)
        {
            unsigned major = 0, minor = 0, patch = 0;
            char trailing = 0;
            const int fields = sscanf_s(ALLSNAP_VERSION_TEXT,"%u.%u.%u%c",
                &major,&minor,&patch,&trailing,1u);
            Assert::AreEqual(3,fields,L"Use an exact three-part semantic version.");
            const WORD resourceVersion[] = {ALLSNAP_VERSION_RESOURCE};
            Assert::AreEqual(4,static_cast<int>(chDIMOF(resourceVersion)));
            Assert::AreEqual<unsigned>(resourceVersion[0],major);
            Assert::AreEqual<unsigned>(resourceVersion[1],minor);
            Assert::AreEqual<unsigned>(resourceVersion[2],patch);
            Assert::AreEqual<WORD>(0,resourceVersion[3]);
            Assert::AreEqual("2.0.0",ALLSNAP_VERSION_TEXT);
        }

        TEST_METHOD(InstanceCompatibilityTokenSeparatesTheNewReleaseFromLegacy)
        {
            Assert::AreEqual<UINT>(0x020000,ALLSNAP_INSTANCE_VERSION);
            Assert::IsTrue(ALLSNAP_INSTANCE_VERSION != 0x013100,
                L"A running legacy instance must not be identified as this release.");
        }
    };
}
