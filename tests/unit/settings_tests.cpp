#include "../support/SettingsFixture.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests
{
    TEST_CLASS(SettingsUnitTests)
    {
    public:
        BEGIN_TEST_CLASS_ATTRIBUTE()
            TEST_CLASS_ATTRIBUTE(L"Category", L"Unit")
        END_TEST_CLASS_ATTRIBUTE()

        TEST_METHOD(IniRoundTripPreservesThresholdsAndSnapDestinations)
        {
            SettingsFixture fixture;
            setWinThresh(23);
            setScreenThresh(31);
            setWindowSpacing(8);
            setEnabled(FALSE);
            const UINT destinations = SNAPT_DESKTOP | SNAPT_OTHERS | SNAPT_SELF | SNAPT_HCENTER | SNAPT_VCENTER;
            setSnapType(destinations);
            Assert::IsTrue(SaveSettingsToINI());
            setWinThresh(2);
            setScreenThresh(2);
            setSnapType(SNAPT_NONE);
            setWindowSpacing(0);
            setEnabled(TRUE);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(23, getWinThresh());
            Assert::AreEqual(31, getScreenThresh());
            Assert::AreEqual(8, getWindowSpacing());
            Assert::IsFalse(isEnabled());
            Assert::AreEqual(destinations, getSnapType());
        }

        TEST_METHOD(IniRoundTripPreservesModifiersAndAdvancedSettings)
        {
            SettingsFixture fixture;
            setToggleKey(VK_TAB);
            setCenterKey(VK_LWIN);
            setEqualKey(VK_RWIN);
            setDisableToggle(FALSE);
            setSnapMdi(TRUE);
            setSnappingInsides(FALSE);
            setKeptToScreen(TRUE);
            setCropTop(7);
            setCroppingTop(TRUE);
            setCroppingRgn(FALSE);
            Assert::IsTrue(SaveSettingsToINI());
            ResetEngine();
            setCenterKey(VK_CONTROL);
            setEqualKey(VK_SHIFT);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(static_cast<UINT>(VK_TAB), getToggleKey());
            Assert::AreEqual(static_cast<UINT>(VK_LWIN), getCenterKey());
            Assert::AreEqual(static_cast<UINT>(VK_RWIN), getEqualKey());
            Assert::IsFalse(isDisableToggle());
            Assert::IsTrue(isSnapMdi());
            Assert::IsFalse(isSnappingInsides());
            Assert::IsTrue(isKeptToScreen());
            Assert::AreEqual(7, getCropTop());
            Assert::IsTrue(isCroppingTop());
            Assert::IsFalse(isCroppingRgn());
        }

        TEST_METHOD(IniRoundTripPreservesIntegerGridValues)
        {
            SettingsFixture fixture;
            gridsnap_settings_t grid = {{TRUE, GRIDTYPE_PIXELS, 37}, {TRUE, GRIDTYPE_SINGLE, 240}};
            setGridSnap(&grid);
            Assert::IsTrue(SaveSettingsToINI());
            grid = {};
            setGridSnap(&grid);
            Assert::IsTrue(LoadSettingsFromINI());
            getGridSnap(&grid);
            Assert::IsTrue(grid.h.enabled && grid.v.enabled);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_PIXELS), static_cast<int>(grid.h.type));
            Assert::AreEqual(37, grid.h.val);
            Assert::AreEqual(static_cast<int>(GRIDTYPE_SINGLE), static_cast<int>(grid.v.type));
            Assert::AreEqual(240, grid.v.val);
        }

        TEST_METHOD(MissingIniKeysPreserveExistingValues)
        {
            SettingsFixture fixture;
            setWinThresh(27);
            setToggleKey(VK_SHIFT);
            setKeptToScreen(TRUE);
            setWindowSpacing(9);
            setEnabled(FALSE);
            Assert::IsTrue(LoadSettingsFromINI());
            Assert::AreEqual(27, getWinThresh());
            Assert::AreEqual(static_cast<UINT>(VK_SHIFT), getToggleKey());
            Assert::IsTrue(isKeptToScreen());
            Assert::AreEqual(9, getWindowSpacing());
            Assert::IsFalse(isEnabled());
        }

        TEST_METHOD(SaveReportsFailureForDirectoryInsteadOfIniFile)
        {
            SettingsFixture fixture;
            WCHAR directory[MAX_PATH] = {};
            Assert::IsTrue(GetTempPathW(MAX_PATH, directory) != 0);
            SnapIni_TestSetPath(directory);
            Assert::IsFalse(SaveSettingsToINI());
        }
    };
}
