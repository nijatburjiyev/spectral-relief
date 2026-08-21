#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <memory>

class ControlStrip final : public juce::Component
{
public:
    explicit ControlStrip (juce::AudioProcessorValueTreeState&);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] static int getPreferredHeight (int availableWidth) noexcept;

    std::function<void (bool)> onHoldChanged;
    std::function<void()> onReset;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void configureSlider (juce::Slider&, const juce::String& id, const juce::String& title);
    void configureLabel (juce::Label&, const juce::String& id, const juce::String& text);
    void updateViewState();
    void updateChoiceText();

    juce::Label titleLabel;
    juce::Label liveLabel;
    juce::Label heightLabel;
    juce::Label lensLabel;
    juce::Label depthLabel;
    juce::Label tiltLabel;
    juce::Label orbitLabel;
    juce::Label zoomLabel;
    juce::Label contrastLabel;
    juce::Label smoothLabel;
    juce::Label averageLabel;
    juce::Label historyLabel;
    juce::Label viewLabel;
    juce::Label resolutionLabel;
    juce::Label rangeLabel;
    juce::Slider heightSlider;
    juce::Slider lensSlider;
    juce::Slider depthSlider;
    juce::Slider tiltSlider;
    juce::Slider orbitSlider;
    juce::Slider zoomSlider;
    juce::Slider contrastSlider;
    juce::Slider smoothSlider;
    juce::Slider averageSlider;
    juce::Slider historySlider;
    juce::TextButton viewButton { "3D" };
    juce::TextButton resolutionButton { "NORMAL" };
    juce::TextButton rangeButton { "FULL" };
    juce::TextButton holdButton { "HOLD" };
    juce::TextButton resetButton { "RESET" };
    std::unique_ptr<SliderAttachment> heightAttachment;
    std::unique_ptr<SliderAttachment> lensAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> tiltAttachment;
    std::unique_ptr<SliderAttachment> orbitAttachment;
    std::unique_ptr<SliderAttachment> zoomAttachment;
    std::unique_ptr<SliderAttachment> contrastAttachment;
    std::unique_ptr<SliderAttachment> smoothAttachment;
    std::unique_ptr<SliderAttachment> averageAttachment;
    std::unique_ptr<SliderAttachment> historyAttachment;
    std::unique_ptr<ButtonAttachment> viewAttachment;
    std::unique_ptr<juce::ParameterAttachment> resolutionAttachment;
    std::unique_ptr<juce::ParameterAttachment> rangeAttachment;
    int currentResolution = 0;
    int currentRange = 0;
};
