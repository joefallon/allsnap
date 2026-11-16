#include "../support/EngineFixture.h"
#include <sstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AllSnapTests {
namespace {
    class OracleRegion {
    public:
        HRGN handle;
        explicit OracleRegion(const RECT & rectangle) : handle(CreateRectRgnIndirect(&rectangle)) {
            Assert::IsNotNull(handle,L"Cannot create the independent GDI reference region.");
        }
        ~OracleRegion() { DeleteObject(handle); }
        OracleRegion(const OracleRegion &) = delete;
        OracleRegion & operator=(const OracleRegion &) = delete;
    };

    // GDI region union/difference is the independent requirement oracle. It does not call
    // the production point, corner, exposed-segment, or feasible-position calculations.
    class DesktopOracle {
        OracleRegion desktop;
    public:
        RECT bounds = {};
        explicit DesktopOracle(std::initializer_list<RECT> workAreas) : desktop({0,0,0,0}) {
            bool first = true;
            for (const RECT & area : workAreas) {
                Assert::IsTrue(rectWidth(area) >= 216 && rectHeight(area) >= 216,
                    L"Every oracle layout must contain a complete 200px window plus its 8px margins.");
                OracleRegion monitor(area);
                Assert::IsTrue(CombineRgn(desktop.handle,desktop.handle,monitor.handle,RGN_OR) != ERROR,
                    L"Cannot construct the reference desktop union.");
                if (first) {
                    bounds = area;
                    first = false;
                } else {
                    if (area.left < bounds.left) bounds.left = area.left;
                    if (area.top < bounds.top) bounds.top = area.top;
                    if (area.right > bounds.right) bounds.right = area.right;
                    if (area.bottom > bounds.bottom) bounds.bottom = area.bottom;
                }
            }
            Assert::IsFalse(first,L"An oracle desktop must have at least one work area.");
        }

        bool CoversWithMargin(RECT rectangle, int spacing) const {
            InflateRect(&rectangle,spacing,spacing);
            OracleRegion expandedWindow(rectangle);
            OracleRegion difference({0,0,0,0});
            int result = CombineRgn(difference.handle,expandedWindow.handle,desktop.handle,RGN_DIFF);
            Assert::IsTrue(result != ERROR,L"Reference desktop difference calculation failed.");
            return result == NULLREGION;
        }
    };

    RECT CheckConfinement(const DesktopOracle & oracle, const RECT & proposed, int spacing,
        const wchar_t * layout) {
        const bool alreadyValid = oracle.CoversWithMargin(proposed,spacing);
        const RECT actual = Move(proposed);
        std::wostringstream description;
        description << layout << L", S=" << spacing << L", input=("
            << proposed.left << L"," << proposed.top << L"," << proposed.right << L"," << proposed.bottom
            << L"), output=(" << actual.left << L"," << actual.top << L"," << actual.right << L"," << actual.bottom << L")";
        const std::wstring message = description.str();
        Assert::AreEqual(200L,rectWidth(actual),message.c_str());
        Assert::AreEqual(200L,rectHeight(actual),message.c_str());
        Assert::IsTrue(oracle.CoversWithMargin(actual,spacing),message.c_str());
        if (alreadyValid) {
            Assert::AreEqual(proposed.left,actual.left,message.c_str());
            Assert::AreEqual(proposed.top,actual.top,message.c_str());
            Assert::AreEqual(proposed.right,actual.right,message.c_str());
            Assert::AreEqual(proposed.bottom,actual.bottom,message.c_str());
        }
        return actual;
    }

    void SweepLayout(std::initializer_list<RECT> workAreas, const wchar_t * name) {
        DesktopOracle oracle(workAreas);
        SetScreens(workAreas);
        for (int spacing : {0,8}) {
            setWindowSpacing(spacing);
            for (int row = 0; row < 5; ++row) {
                for (int column = 0; column < 5; ++column) {
                    const LONG left = oracle.bounds.left - 100 + column * rectWidth(oracle.bounds) / 4;
                    const LONG top = oracle.bounds.top - 100 + row * rectHeight(oracle.bounds) / 4;
                    CheckConfinement(oracle,{left,top,left+200,top+200},spacing,name);
                }
            }
        }
    }
}

TEST_CLASS(ConfinementOracleAcceptanceTests) {
public:
    BEGIN_TEST_CLASS_ATTRIBUTE()
        TEST_CLASS_ATTRIBUTE(L"Category", L"Acceptance")
    END_TEST_CLASS_ATTRIBUTE()
    TEST_METHOD_INITIALIZE(Initialize) {
        ResetEngine();
        setSnapType(SNAPT_NONE);
        setKeptToScreen(TRUE);
    }
    TEST_METHOD_CLEANUP(Cleanup) { ResetEngine(); }

    // DCO-001: a fitting rectangle plus its complete margin belongs to the desktop union.
    TEST_METHOD(AdjacentMonitorSweepSatisfiesIndependentRegionOracle) {
        SweepLayout({{0,0,800,600},{800,0,1600,600}},L"adjacent monitors");
    }
    TEST_METHOD(StaggeredMonitorSweepSatisfiesIndependentRegionOracle) {
        SweepLayout({{-800,0,0,600},{0,200,800,800}},L"staggered monitors");
    }
    TEST_METHOD(DisjointMonitorSweepSatisfiesIndependentRegionOracle) {
        SweepLayout({{-1000,-300,-200,300},{200,-300,1000,300}},L"disjoint monitors");
    }
    TEST_METHOD(FourMonitorHoleSweepSatisfiesIndependentRegionOracle) {
        SweepLayout({{-600,-600,600,-200},{-600,200,600,600},
            {-600,-200,-200,200},{200,-200,600,200}},L"four-monitor ring with uncovered hole");
    }

    // DCO-003: boundary points do not make the unavailable rectangle between them valid.
    TEST_METHOD(WindowExactlyCoincidentWithAGapMustLeaveTheGap) {
        SetScreens({{0,0,800,1000},{1000,0,1800,1000}});
        DesktopOracle oracle({{0,0,800,1000},{1000,0,1800,1000}});
        CheckConfinement(oracle,{800,100,1000,300},0,L"rectangle exactly filling an unavailable gap");
    }

    // DCO-002: confinement is invariant under translation into negative desktop coordinates.
    TEST_METHOD(TranslatedNegativeLayoutPreservesConfinementDecisions) {
        const std::initializer_list<RECT> originalAreas = {{0,0,800,600},{800,200,1600,800}};
        const std::initializer_list<RECT> translatedAreas = {{-4200,-2300,-3400,-1700},{-3400,-2100,-2600,-1500}};
        DesktopOracle originalOracle(originalAreas), translatedOracle(translatedAreas);
        for (int spacing : {0,8}) {
            setWindowSpacing(spacing);
            for (LONG top : {-100L,300L,700L}) {
                for (LONG left : {-100L,700L,1500L}) {
                    RECT proposed = {left,top,left+200,top+200};
                    SetScreens(originalAreas);
                    RECT originalResult = CheckConfinement(originalOracle,proposed,spacing,L"translation base");
                    OffsetRect(&proposed,-4200,-2300);
                    SetScreens(translatedAreas);
                    RECT translatedResult = CheckConfinement(translatedOracle,proposed,spacing,L"negative translation");
                    OffsetRect(&originalResult,-4200,-2300);
                    AssertRect(originalResult,translatedResult);
                }
            }
        }
    }
};
}
