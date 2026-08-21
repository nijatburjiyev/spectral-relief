#include "Source/Rendering/DisplayRowProcessor.h"
#include "Source/Rendering/SpectrumFrameAggregator.h"

#include <juce_core/juce_core.h>

namespace
{
class DisplayRowProcessorTests final : public juce::UnitTest
{
public:
    DisplayRowProcessorTests() : juce::UnitTest ("Display row processing", "rendering") {}

    void runTest() override
    {
        SpectrumRow first;
        SpectrumRow second;
        for (std::size_t i = 0; i < first.size(); ++i)
        {
            first[i] = static_cast<float> (i) / 255.0f;
            second[i] = 1.0f - first[i];
        }

        beginTest ("zero smoothing is an identity operation");
        DisplayRowProcessor processor;
        processor.setSmoothing (0.0f);
        const auto identity = processor.process (first);
        expect (identity == first);

        beginTest ("maximum smoothing has a 1.5 second time constant");
        SpectrumFrame zeroFrame;
        zeroFrame.bandCount = 256;
        SpectrumFrame oneFrame = zeroFrame;
        std::fill_n (oneFrame.magnitudes.begin(), oneFrame.bandCount, 1.0f);
        processor.reset();
        processor.setSmoothing (1.0f);
        processor.process (zeroFrame, 0.0);
        const auto oneTimeConstant = processor.process (oneFrame, 1.5);
        expectWithinAbsoluteError (oneTimeConstant.magnitudes[100],
                                   static_cast<float> (1.0 - std::exp (-1.0)), 1.0e-5f);

        beginTest ("two half intervals equal one full smoothing interval");
        processor.reset();
        processor.setSmoothing (1.0f);
        processor.process (zeroFrame, 0.0);
        processor.process (oneFrame, 0.75);
        const auto twoHalves = processor.process (oneFrame, 0.75);
        expectWithinAbsoluteError (twoHalves.magnitudes[100],
                                   oneTimeConstant.magnitudes[100], 1.0e-5f);

        beginTest ("temporal smoothing never spills into neighbouring frequencies");
        processor.reset();
        processor.setSmoothing (0.8f);
        processor.process (zeroFrame, 0.0);
        SpectrumFrame spike = zeroFrame;
        spike.magnitudes[120] = 1.0f;
        const auto smoothedSpike = processor.process (spike, 0.1);
        expect (smoothedSpike.magnitudes[120] > 0.0f);
        expectWithinAbsoluteError (smoothedSpike.magnitudes[119], 0.0f, 1.0e-7f);
        expectWithinAbsoluteError (smoothedSpike.magnitudes[121], 0.0f, 1.0e-7f);

        beginTest ("reset removes previous-row influence");
        processor.reset();
        expect (processor.process (second) == second);

        beginTest ("smoothing is clamped to the approved range");
        processor.setSmoothing (1.0f);
        expectWithinAbsoluteError (processor.getSmoothing(), 1.0f, 1.0e-7f);
        expectWithinAbsoluteError (static_cast<float> (processor.getTimeConstantSeconds()),
                                   1.5f, 1.0e-7f);
        processor.setSmoothing (-1.0f);
        expectWithinAbsoluteError (processor.getSmoothing(), 0.0f, 1.0e-7f);

        beginTest ("frame aggregation retains a transient until upload");
        SpectrumFrameAggregator aggregator;
        SpectrumFrame silence;
        silence.bandCount = 256;
        SpectrumFrame singleBandSpike = silence;
        singleBandSpike.magnitudes[120] = 1.0f;
        aggregator.reset (256);
        aggregator.push (silence);
        aggregator.push (singleBandSpike);
        aggregator.push (silence);
        SpectrumFrame peakFrame;
        expect (aggregator.takePeakFrame (peakFrame));
        expectEquals (peakFrame.magnitudes[120], 1.0f);
        expect (! aggregator.takePeakFrame (peakFrame));

        beginTest ("zero smoothing preserves all 512 High bands");
        SpectrumFrame highFrame;
        highFrame.bandCount = 512;
        for (std::size_t band = 0; band < highFrame.magnitudes.size(); ++band)
            highFrame.magnitudes[band] = static_cast<float> (band) / 511.0f;
        processor.reset();
        processor.setSmoothing (0.0f);
        const auto highIdentity = processor.process (highFrame);
        expectEquals (highIdentity.bandCount, static_cast<std::uint16_t> (512));
        expect (highIdentity.magnitudes == highFrame.magnitudes);

        beginTest ("five seconds schedules 256 history rows");
        HistoryCadence cadence;
        cadence.setDuration (5.0);
        int totalRows = 0;
        for (int tick = 0; tick < 300; ++tick)
            totalRows += cadence.rowsDue (5.0 / 300.0);
        expectEquals (totalRows, 256);

        beginTest ("two-second history can upload multiple rows per render");
        cadence.reset();
        cadence.setDuration (2.0);
        expect (cadence.rowsDue (1.0 / 60.0) >= 2);

        beginTest ("a long stall never requests more than eight rows");
        cadence.reset();
        expectEquals (cadence.rowsDue (100.0), 8);
        expect (cadence.rowsDue (0.0) == 0);
    }
};

DisplayRowProcessorTests displayRowProcessorTests;
} // namespace
