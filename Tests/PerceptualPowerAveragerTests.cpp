#include "Source/Analysis/PerceptualPowerAverager.h"

#include <juce_core/juce_core.h>

#include <array>
#include <limits>

namespace
{
class PerceptualPowerAveragerTests final : public juce::UnitTest
{
public:
    PerceptualPowerAveragerTests()
        : juce::UnitTest ("Perceptual power averaging", "analysis")
    {
    }

    void runTest() override
    {
        beginTest ("OFF is an exact identity");
        PerceptualPowerAverager averager;
        averager.setAverageMilliseconds (0.0f);
        std::array<float, 3> off { -12.0f, -48.0f, -100.0f };
        const auto original = off;
        averager.processDecibels (off, 0.1);
        expect (off == original);

        beginTest ("first enabled frame initializes without fading in");
        averager.setAverageMilliseconds (80.0f);
        std::array<float, 2> first { -9.0f, -60.0f };
        averager.processDecibels (first, 0.02);
        expectWithinAbsoluteError (first[0], -9.0f, 1.0e-6f);
        expectWithinAbsoluteError (first[1], -60.0f, 1.0e-6f);

        beginTest ("configured time exposes quarter attack and full release");
        averager.setAverageMilliseconds (400.0f);
        expectWithinAbsoluteError (averager.getAverageMilliseconds(), 400.0f, 1.0e-7f);
        expectWithinAbsoluteError (averager.getAttackSeconds(), 0.1, 1.0e-9);
        expectWithinAbsoluteError (averager.getReleaseSeconds(), 0.4, 1.0e-9);

        beginTest ("one release time constant averages linear power");
        averager.reset();
        std::array<float, 1> loud { 0.0f };
        averager.processDecibels (loud, 0.01);
        std::array<float, 1> quiet { -100.0f };
        averager.processDecibels (quiet, 0.4);
        expectWithinAbsoluteError (quiet[0], -4.3429446f, 1.0e-4f);

        beginTest ("positive calibrated levels retain power above unity");
        averager.reset();
        std::array<float, 1> aboveFullScale { 6.0f };
        averager.processDecibels (aboveFullScale, 0.01);
        std::array<float, 1> unity { 0.0f };
        averager.processDecibels (unity, 0.4);
        expectWithinAbsoluteError (unity[0], 3.2153112f, 1.0e-4f);

        beginTest ("attack reaches one time constant in one quarter of release time");
        averager.reset();
        std::array<float, 1> floor { -100.0f };
        averager.processDecibels (floor, 0.01);
        std::array<float, 1> peak { 0.0f };
        averager.processDecibels (peak, 0.1);
        expectWithinAbsoluteError (peak[0], -1.9920008f, 1.0e-4f);

        beginTest ("two half intervals equal one full release interval");
        PerceptualPowerAverager whole;
        PerceptualPowerAverager halves;
        whole.setAverageMilliseconds (400.0f);
        halves.setAverageMilliseconds (400.0f);
        std::array<float, 1> seedA { 0.0f };
        std::array<float, 1> seedB { 0.0f };
        whole.processDecibels (seedA, 0.01);
        halves.processDecibels (seedB, 0.01);
        std::array<float, 1> full { -100.0f };
        std::array<float, 1> half { -100.0f };
        whole.processDecibels (full, 0.2);
        halves.processDecibels (half, 0.1);
        half[0] = -100.0f;
        halves.processDecibels (half, 0.1);
        expectWithinAbsoluteError (full[0], half[0], 1.0e-4f);

        beginTest ("averaging never spills into neighbouring frequency bands");
        averager.reset();
        std::array<float, 3> silent { -100.0f, -100.0f, -100.0f };
        averager.processDecibels (silent, 0.01);
        std::array<float, 3> centrePeak { -100.0f, 0.0f, -100.0f };
        averager.processDecibels (centrePeak, 0.1);
        expectWithinAbsoluteError (centrePeak[0], -100.0f, 1.0e-5f);
        expect (centrePeak[1] > -3.0f);
        expectWithinAbsoluteError (centrePeak[2], -100.0f, 1.0e-5f);

        beginTest ("a changed band count initializes every band directly");
        averager.reset();
        std::array<float, 1> oneBand { 0.0f };
        averager.processDecibels (oneBand, 0.01);
        std::array<float, 2> twoBands { -20.0f, -40.0f };
        averager.processDecibels (twoBands, 0.1);
        expectWithinAbsoluteError (twoBands[0], -20.0f, 1.0e-6f);
        expectWithinAbsoluteError (twoBands[1], -40.0f, 1.0e-6f);

        beginTest ("reset removes prior power and invalid input uses the floor");
        averager.reset();
        std::array<float, 1> invalid { std::numeric_limits<float>::quiet_NaN() };
        averager.processDecibels (invalid, -1.0);
        expectWithinAbsoluteError (invalid[0], -100.0f, 1.0e-6f);
        averager.reset();
        std::array<float, 1> independent { -36.0f };
        averager.processDecibels (independent, 0.01);
        expectWithinAbsoluteError (independent[0], -36.0f, 1.0e-6f);

        beginTest ("requested milliseconds are clamped to the approved range");
        averager.setAverageMilliseconds (-5.0f);
        expectWithinAbsoluteError (averager.getAverageMilliseconds(), 0.0f, 1.0e-7f);
        averager.setAverageMilliseconds (5000.0f);
        expectWithinAbsoluteError (averager.getAverageMilliseconds(), 1000.0f, 1.0e-7f);
    }
};

PerceptualPowerAveragerTests perceptualPowerAveragerTests;
} // namespace
