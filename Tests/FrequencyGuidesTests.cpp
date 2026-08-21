#include "Source/Rendering/FrequencyGuides.h"

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>

namespace
{
class FrequencyGuidesTests final : public juce::UnitTest
{
public:
    FrequencyGuidesTests() : juce::UnitTest ("Frequency guides", "rendering") {}

    void runTest() override
    {
        const auto expectFrequencies = [this] (FrequencyRange range,
                                               std::initializer_list<double> expected)
        {
            const auto guides = makeFrequencyGuides (getFrequencyRangeProfile (range), 24000.0);
            expectEquals (static_cast<int> (guides.count), static_cast<int> (expected.size()));
            std::size_t index = 0;
            for (const auto frequency : expected)
                expectWithinAbsoluteError (guides.values[index++].frequencyHz, frequency, 0.001);
        };

        beginTest ("range presets expose only applicable major guides");
        expectFrequencies (FrequencyRange::full,
                           { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 });
        expectFrequencies (FrequencyRange::low, { 20, 50, 100, 200, 500 });
        expectFrequencies (FrequencyRange::mid, { 200, 500, 1000, 2000, 5000 });
        expectFrequencies (FrequencyRange::high, { 2000, 5000, 10000, 20000 });

        beginTest ("guide coordinates use the exact logarithmic range mapping");
        const auto low = makeFrequencyGuides (getFrequencyRangeProfile (FrequencyRange::low), 24000.0);
        expectWithinAbsoluteError (low.values.front().normalisedFrequency, -1.0f, 1.0e-6f);
        expectWithinAbsoluteError (low.values[low.count - 1].normalisedFrequency, 1.0f, 1.0e-6f);
        const auto expected100 = static_cast<float> (
            2.0 * std::log (100.0 / 20.0) / std::log (500.0 / 20.0) - 1.0);
        expectWithinAbsoluteError (low.values[2].normalisedFrequency, expected100, 1.0e-6f);

        beginTest ("guides above Nyquist are omitted");
        const auto limited = makeFrequencyGuides (getFrequencyRangeProfile (FrequencyRange::full), 16000.0);
        expectEquals (static_cast<int> (limited.count), 9);
        expectWithinAbsoluteError (limited.values[limited.count - 1].frequencyHz, 10000.0, 0.001);
    }
};

FrequencyGuidesTests frequencyGuidesTests;
} // namespace
