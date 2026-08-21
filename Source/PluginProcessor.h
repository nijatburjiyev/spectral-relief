#pragma once

#include "Analysis/SpectrumAnalysisWorker.h"
#include "Diagnostics/RuntimeDiagnostics.h"
#include "Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>

class SpectralReliefAudioProcessor final : public juce::AudioProcessor
{
public:
    SpectralReliefAudioProcessor();

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Spectral Relief"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getParameterState() noexcept { return parameterState; }
    const juce::AudioProcessorValueTreeState& getParameterState() const noexcept { return parameterState; }
    SpectrumFrameQueue& getFrameQueue() noexcept { return frameQueue; }
    RuntimeDiagnostics& getRuntimeDiagnostics() noexcept { return diagnostics; }
    void setVisualizationActive (bool active) { analysisWorker.setVisualizationActive (active); }
    void resetAnalysis() noexcept { analysisWorker.requestReset(); }

private:
    juce::AudioProcessorValueTreeState parameterState;
    SpectrumFrameQueue frameQueue;
    RuntimeDiagnostics diagnostics;
    SpectrumAnalysisWorker analysisWorker;
    std::atomic<float>* resolutionParameter = nullptr;
    std::atomic<float>* rangeParameter = nullptr;
    std::atomic<float>* averageParameter = nullptr;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
