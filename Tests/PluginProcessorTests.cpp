#include "Source/PluginProcessor.h"

#include <algorithm>
#include <cmath>

namespace
{
class PluginProcessorTests final : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("Plugin processor", "processor") {}

    void runTest() override
    {
        beginTest ("stereo audio is unchanged");

        SpectralReliefAudioProcessor processor;
        processor.prepareToPlay (48000.0, 512);
        auto* average = processor.getParameterState().getParameter (ParameterIDs::averageMs);
        average->setValueNotifyingHost (average->convertTo0to1 (1000.0f));

        juce::AudioBuffer<float> audio (2, 512);
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                audio.setSample (channel,
                                 sample,
                                 std::sin (0.013f * static_cast<float> (sample + 3 * channel)));

        const auto expected = audio;
        juce::MidiBuffer midi;
        processor.processBlock (audio, midi);

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                expectEquals (audio.getSample (channel, sample), expected.getSample (channel, sample));

        expectEquals (processor.getLatencySamples(), 0);

        beginTest ("mono and stereo layouts are supported");
        expect (processor.isBusesLayoutSupported (
            juce::AudioProcessor::BusesLayout { juce::AudioChannelSet::mono(),
                                                juce::AudioChannelSet::mono() }));
        expect (processor.isBusesLayoutSupported (
            juce::AudioProcessor::BusesLayout { juce::AudioChannelSet::stereo(),
                                                juce::AudioChannelSet::stereo() }));

        beginTest ("processing publishes spectrum rows without changing audio");
        processor.setVisualizationActive (true);
        juce::AudioBuffer<float> longAudio (2, 4096);
        for (int channel = 0; channel < longAudio.getNumChannels(); ++channel)
            for (int sample = 0; sample < longAudio.getNumSamples(); ++sample)
                longAudio.setSample (
                    channel,
                    sample,
                    0.5f * std::sin (juce::MathConstants<float>::twoPi
                                     * 440.0f * static_cast<float> (sample) / 48000.0f));
        const auto longExpected = longAudio;
        processor.processBlock (longAudio, midi);

        for (int channel = 0; channel < longAudio.getNumChannels(); ++channel)
            for (int sample = 0; sample < longAudio.getNumSamples(); ++sample)
                expectEquals (longAudio.getSample (channel, sample),
                              longExpected.getSample (channel, sample));

        SpectrumRow spectrum;
        bool receivedSpectrum = false;
        for (int attempt = 0; attempt < 250 && ! receivedSpectrum; ++attempt)
        {
            receivedSpectrum = processor.getFrameQueue().tryPop (spectrum);
            if (! receivedSpectrum)
                juce::Thread::sleep (2);
        }
        expect (receivedSpectrum);
        expect (*std::max_element (spectrum.begin(), spectrum.end()) > 0.8f);
        processor.setVisualizationActive (false);
    }
};

PluginProcessorTests pluginProcessorTests;
} // namespace
