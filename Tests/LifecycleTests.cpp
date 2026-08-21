#include "Source/PluginProcessor.h"
#include "Source/Analysis/AudioSampleQueue.h"
#include "Source/Rendering/SpectralSurfaceComponent.h"
#include "Source/Rendering/SurfaceMesh.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <memory>

namespace
{
class LifecycleTests final : public juce::UnitTest
{
public:
    LifecycleTests() : juce::UnitTest ("Lifecycle and resources", "lifecycle") {}

    void runTest() override
    {
        beginTest ("fixed resource payloads stay within budget");
        expect (SpectrumFrameQueue::payloadBytes + AudioSampleQueue::payloadBytes
                < 2 * 1024 * 1024);
        expectEquals (static_cast<int> (SpectralSurfaceComponent::historyTextureBytes), 1024 * 256 * 2);
        const SurfaceMesh ultra (1024, 256);
        expect (ultra.payloadBytes()
                    + SpectralSurfaceComponent::historyTextureBytes
                < 12 * 1024 * 1024);

        beginTest ("repeated sample-rate lifecycle preserves transparent audio");
        SpectralReliefAudioProcessor processor;
        for (int cycle = 0; cycle < 100; ++cycle)
        {
            const auto rate = std::array { 44100.0, 48000.0, 96000.0 }[
                static_cast<std::size_t> (cycle % 3)];
            processor.prepareToPlay (rate, 257);
            juce::AudioBuffer<float> audio (2, 257);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    audio.setSample (channel, sample,
                                     0.25f * std::sin (0.017f * static_cast<float> (sample + cycle)));
            const auto expected = audio;
            juce::MidiBuffer midi;
            processor.processBlock (audio, midi);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    expectEquals (audio.getSample (channel, sample), expected.getSample (channel, sample));
            processor.releaseResources();
        }

        beginTest ("editor can be repeatedly constructed and destroyed");
        for (int iteration = 0; iteration < 50; ++iteration)
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
            expect (editor != nullptr);
        }

        beginTest ("a full visualization queue never affects audio");
        processor.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> silence (2, 8192);
        silence.clear();
        juce::MidiBuffer midi;
        processor.processBlock (silence, midi);
        processor.processBlock (silence, midi);
        for (int channel = 0; channel < silence.getNumChannels(); ++channel)
            for (int sample = 0; sample < silence.getNumSamples(); ++sample)
                expect (std::isfinite (silence.getSample (channel, sample))
                        && silence.getSample (channel, sample) == 0.0f);

        SpectrumRow row;
        while (processor.getFrameQueue().tryPop (row))
            for (const auto value : row)
                expect (std::isfinite (value));
    }
};

LifecycleTests lifecycleTests;
} // namespace
