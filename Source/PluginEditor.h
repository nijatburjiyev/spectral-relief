#pragma once

#include "Rendering/SpectralSurfaceComponent.h"
#include "Diagnostics/RuntimeDiagnostics.h"
#include "UI/ControlStrip.h"

#include <juce_audio_processors/juce_audio_processors.h>

class SpectralReliefAudioProcessor;

class SpectralReliefAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SpectralReliefAudioProcessorEditor (SpectralReliefAudioProcessor&);
    ~SpectralReliefAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SpectralReliefAudioProcessor& ownerProcessor;
    SpectralSurfaceComponent spectralSurface;
    ControlStrip controlStrip;
    RuntimeDiagnosticsLogger diagnosticsLogger;
};
