#include "Source/Analysis/AnalysisQuality.h"
#include "Source/Analysis/FrequencyRange.h"
#include "Source/Parameters.h"
#include "Source/PluginProcessor.h"

#include <juce_core/juce_core.h>

#include <memory>

namespace
{
class ParametersTests final : public juce::UnitTest
{
public:
    ParametersTests() : juce::UnitTest ("Parameters", "state") {}

    void runTest() override
    {
        beginTest ("quality profiles expose approved fixed resources");
        expectEquals (getQualityProfile (AnalysisQuality::normal).fftOrder, 11);
        expectEquals (getQualityProfile (AnalysisQuality::normal).bandCount, 256);
        expectEquals (getQualityProfile (AnalysisQuality::high).fftOrder, 12);
        expectEquals (getQualityProfile (AnalysisQuality::high).bandCount, 512);
        expectEquals (getQualityProfile (AnalysisQuality::ultra).fftOrder, 13);
        expectEquals (getQualityProfile (AnalysisQuality::ultra).bandCount, 1024);
        expectEquals (getQualityProfile (AnalysisQuality::ultra).meshWidth, 1024);

        beginTest ("frequency ranges expose approved overlapping bounds");
        expectEquals (getFrequencyRangeProfile (FrequencyRange::full).minimumHz, 20.0);
        expectEquals (getFrequencyRangeProfile (FrequencyRange::full).maximumHz, 20000.0);
        expectEquals (getFrequencyRangeProfile (FrequencyRange::low).maximumHz, 500.0);
        expectEquals (getFrequencyRangeProfile (FrequencyRange::mid).minimumHz, 200.0);
        expectEquals (getFrequencyRangeProfile (FrequencyRange::mid).maximumHz, 5000.0);
        expectEquals (getFrequencyRangeProfile (FrequencyRange::high).minimumHz, 2000.0);

        beginTest ("display parameters have detail-preserving defaults");
        auto processor = std::make_unique<SpectralReliefAudioProcessor>();
        auto& state = processor->getParameterState();
        expectEquals (juce::String (ParameterIDs::curve), juce::String ("curve"));
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::height)->load(), 1.4f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::curve)->load(), 0.35f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::depth)->load(), 1.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::tilt)->load(), 55.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::orbit)->load(), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::zoom)->load(), 1.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::contrast)->load(), 1.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::resolution)->load(), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::range)->load(), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::view2d)->load(), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::smooth)->load(), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::averageMs)->load(), 80.0f, 0.0001f);
        expectWithinAbsoluteError (state.getRawParameterValue (ParameterIDs::history)->load(), 5.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::tilt)->convertFrom0to1 (1.0f), 90.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::orbit)->convertFrom0to1 (0.0f), -180.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::orbit)->convertFrom0to1 (1.0f), 180.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::smooth)->convertFrom0to1 (1.0f), 1.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::averageMs)->convertFrom0to1 (0.0f), 0.0f, 0.0001f);
        expectWithinAbsoluteError (state.getParameter (ParameterIDs::averageMs)->convertFrom0to1 (1.0f), 1000.0f, 0.0001f);
        expectEquals (state.getParameter (ParameterIDs::averageMs)->getText (
                          state.getParameter (ParameterIDs::averageMs)->convertTo0to1 (0.0f), 32),
                      juce::String ("OFF"));
        expectEquals (state.getParameter (ParameterIDs::averageMs)->getText (
                          state.getParameter (ParameterIDs::averageMs)->convertTo0to1 (80.0f), 32),
                      juce::String ("80 ms"));
        expectEquals (state.getParameter (ParameterIDs::curve)->getName (32), juce::String ("Lens"));

        beginTest ("parameter state round-trips");
        auto* height = state.getParameter (ParameterIDs::height);
        auto* curve = state.getParameter (ParameterIDs::curve);
        auto* depth = state.getParameter (ParameterIDs::depth);
        auto* tilt = state.getParameter (ParameterIDs::tilt);
        auto* orbit = state.getParameter (ParameterIDs::orbit);
        auto* zoom = state.getParameter (ParameterIDs::zoom);
        auto* contrast = state.getParameter (ParameterIDs::contrast);
        auto* smooth = state.getParameter (ParameterIDs::smooth);
        auto* average = state.getParameter (ParameterIDs::averageMs);
        auto* history = state.getParameter (ParameterIDs::history);
        auto* resolution = state.getParameter (ParameterIDs::resolution);
        auto* range = state.getParameter (ParameterIDs::range);
        auto* view2d = state.getParameter (ParameterIDs::view2d);
        height->setValueNotifyingHost (height->convertTo0to1 (2.25f));
        curve->setValueNotifyingHost (curve->convertTo0to1 (0.8f));
        depth->setValueNotifyingHost (depth->convertTo0to1 (1.7f));
        tilt->setValueNotifyingHost (tilt->convertTo0to1 (72.0f));
        orbit->setValueNotifyingHost (orbit->convertTo0to1 (31.0f));
        zoom->setValueNotifyingHost (zoom->convertTo0to1 (1.35f));
        contrast->setValueNotifyingHost (contrast->convertTo0to1 (2.1f));
        smooth->setValueNotifyingHost (smooth->convertTo0to1 (0.35f));
        average->setValueNotifyingHost (average->convertTo0to1 (640.0f));
        history->setValueNotifyingHost (history->convertTo0to1 (7.0f));
        resolution->setValueNotifyingHost (resolution->convertTo0to1 (2.0f));
        range->setValueNotifyingHost (range->convertTo0to1 (3.0f));
        view2d->setValueNotifyingHost (1.0f);

        juce::MemoryBlock saved;
        processor->getStateInformation (saved);
        expect (saved.getSize() > 0);

        auto restored = std::make_unique<SpectralReliefAudioProcessor>();
        restored->setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));
        auto& restoredState = restored->getParameterState();
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::height)->load(), 2.25f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::curve)->load(), 0.8f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::depth)->load(), 1.7f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::tilt)->load(), 72.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::orbit)->load(), 31.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::zoom)->load(), 1.35f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::contrast)->load(), 2.1f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::smooth)->load(), 0.35f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::averageMs)->load(), 640.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::history)->load(), 7.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::resolution)->load(), 2.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::range)->load(), 3.0f, 0.001f);
        expectWithinAbsoluteError (restoredState.getRawParameterValue (ParameterIDs::view2d)->load(), 1.0f, 0.001f);

        beginTest ("legacy High Quality state migrates to High resolution");
        auto legacyTree = state.copyState();
        constexpr auto parameterIdProperty = "id";
        constexpr auto parameterValueProperty = "value";
        if (const auto resolutionNode = legacyTree.getChildWithProperty (
                parameterIdProperty, ParameterIDs::resolution); resolutionNode.isValid())
            legacyTree.removeChild (resolutionNode, nullptr);
        auto legacyQualityNode = legacyTree.getChildWithProperty (
            parameterIdProperty, ParameterIDs::qualityHigh);
        if (! legacyQualityNode.isValid())
        {
            legacyQualityNode = juce::ValueTree { legacyTree.getChild (0).getType() };
            legacyQualityNode.setProperty (parameterIdProperty, ParameterIDs::qualityHigh, nullptr);
            legacyTree.appendChild (legacyQualityNode, nullptr);
        }
        legacyQualityNode.setProperty (parameterValueProperty, 1.0f, nullptr);
        juce::MemoryBlock legacyBlock;
        if (const auto legacyXml = legacyTree.createXml())
            juce::AudioProcessor::copyXmlToBinary (*legacyXml, legacyBlock);
        auto migrated = std::make_unique<SpectralReliefAudioProcessor>();
        migrated->setStateInformation (legacyBlock.getData(), static_cast<int> (legacyBlock.getSize()));
        expectWithinAbsoluteError (
            migrated->getParameterState().getRawParameterValue (ParameterIDs::resolution)->load(),
            1.0f, 0.001f);

        beginTest ("malformed state leaves defaults intact");
        constexpr char malformed[] = "not plugin state";
        auto malformedTarget = std::make_unique<SpectralReliefAudioProcessor>();
        malformedTarget->setStateInformation (malformed, static_cast<int> (sizeof (malformed)));
        expectWithinAbsoluteError (
            malformedTarget->getParameterState().getRawParameterValue (ParameterIDs::height)->load(),
            1.4f,
            0.0001f);
    }
};

ParametersTests parametersTests;
} // namespace
