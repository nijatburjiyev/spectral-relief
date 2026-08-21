#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralReliefAudioProcessor::SpectralReliefAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameterState (*this, nullptr, "PARAMETERS", createParameterLayout()),
      analysisWorker (frameQueue, diagnostics),
      resolutionParameter (parameterState.getRawParameterValue (ParameterIDs::resolution)),
      rangeParameter (parameterState.getRawParameterValue (ParameterIDs::range)),
      averageParameter (parameterState.getRawParameterValue (ParameterIDs::averageMs))
{
    setLatencySamples (0);
}

void SpectralReliefAudioProcessor::prepareToPlay (double sampleRate, int)
{
    analysisWorker.prepare (sampleRate);
    frameQueue.requestDiscard();
}

void SpectralReliefAudioProcessor::releaseResources()
{
}

bool SpectralReliefAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == output;
}

void SpectralReliefAudioProcessor::processBlock (juce::AudioBuffer<float>& audio,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    diagnostics.processBlocks.fetch_add (1, std::memory_order_relaxed);
    diagnostics.processedSamples.fetch_add (static_cast<std::uint64_t> (audio.getNumSamples()),
                                            std::memory_order_relaxed);
    float inputPeak = 0.0f;
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        inputPeak = std::max (inputPeak, audio.getMagnitude (channel, 0, audio.getNumSamples()));
    diagnostics.recordInputPeak (inputPeak);
    analysisWorker.setAnalysisSelection (
        static_cast<AnalysisQuality> (juce::jlimit (0, 2, juce::roundToInt (resolutionParameter->load()))),
        static_cast<FrequencyRange> (juce::jlimit (0, 3, juce::roundToInt (rangeParameter->load()))),
        averageParameter->load());
    analysisWorker.capture (audio);
}

juce::AudioProcessorEditor* SpectralReliefAudioProcessor::createEditor()
{
    return new SpectralReliefAudioProcessorEditor (*this);
}

void SpectralReliefAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (const auto xml = parameterState.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void SpectralReliefAudioProcessor::setStateInformation (const void* data, int size)
{
    const auto xml = getXmlFromBinary (data, size);
    if (xml != nullptr && xml->hasTagName (parameterState.state.getType()))
    {
        auto restored = juce::ValueTree::fromXml (*xml);
        constexpr auto parameterIdProperty = "id";
        constexpr auto parameterValueProperty = "value";
        const auto resolutionNode = restored.getChildWithProperty (
            parameterIdProperty, ParameterIDs::resolution);
        const auto legacyQualityNode = restored.getChildWithProperty (
            parameterIdProperty, ParameterIDs::qualityHigh);
        if (! resolutionNode.isValid() && legacyQualityNode.isValid())
        {
            const auto legacyHigh = static_cast<float> (
                legacyQualityNode.getProperty (parameterValueProperty, 0.0f));
            juce::ValueTree migratedResolution { legacyQualityNode.getType() };
            migratedResolution.setProperty (parameterIdProperty, ParameterIDs::resolution, nullptr);
            migratedResolution.setProperty (parameterValueProperty,
                                            legacyHigh >= 0.5f ? 1.0f : 0.0f,
                                            nullptr);
            restored.appendChild (migratedResolution, nullptr);
        }
        parameterState.replaceState (restored);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralReliefAudioProcessor();
}
