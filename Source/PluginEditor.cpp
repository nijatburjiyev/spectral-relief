#include "PluginEditor.h"
#include "PluginProcessor.h"

SpectralReliefAudioProcessorEditor::SpectralReliefAudioProcessorEditor (
    SpectralReliefAudioProcessor& owner)
    : juce::AudioProcessorEditor (owner),
      ownerProcessor (owner),
      spectralSurface (owner.getFrameQueue(), owner.getRuntimeDiagnostics(),
                       owner.getParameterState()),
      controlStrip (owner.getParameterState()),
      diagnosticsLogger (owner.getRuntimeDiagnostics())
{
    ownerProcessor.setVisualizationActive (true);
    spectralSurface.setComponentID ("spectralSurface");
    spectralSurface.setTitle ("Real-time three-dimensional spectrogram");
    addAndMakeVisible (spectralSurface);
    addAndMakeVisible (controlStrip);

    controlStrip.onHoldChanged = [this] (bool held) { spectralSurface.setHeld (held); };
    controlStrip.onReset = [this]
    {
        ownerProcessor.resetAnalysis();
        spectralSurface.clearHistory();
    };

    const auto width = juce::jlimit (640, 1800,
                                    static_cast<int> (owner.getParameterState().state.getProperty (
                                        "editorWidth", 900)));
    const auto height = juce::jlimit (400, 1200,
                                     static_cast<int> (owner.getParameterState().state.getProperty (
                                         "editorHeight", 560)));
    setResizable (true, true);
    setResizeLimits (640, 400, 1800, 1200);
    setSize (width, height);
}

SpectralReliefAudioProcessorEditor::~SpectralReliefAudioProcessorEditor()
{
    ownerProcessor.setVisualizationActive (false);
}

void SpectralReliefAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour::fromRGB (0x10, 0x11, 0x14));
}

void SpectralReliefAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    controlStrip.setBounds (area.removeFromTop (ControlStrip::getPreferredHeight (getWidth())));
    spectralSurface.setBounds (area);
    ownerProcessor.getParameterState().state.setProperty ("editorWidth", getWidth(), nullptr);
    ownerProcessor.getParameterState().state.setProperty ("editorHeight", getHeight(), nullptr);
}
