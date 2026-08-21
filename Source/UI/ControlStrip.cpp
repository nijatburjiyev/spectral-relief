#include "ControlStrip.h"

#include "Source/Parameters.h"

#include <algorithm>
#include <array>
#include <utility>

ControlStrip::ControlStrip (juce::AudioProcessorValueTreeState& state)
{
    titleLabel.setText ("SPECTRAL RELIEF", juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.9f));
    titleLabel.setFont (juce::FontOptions { 13.0f, juce::Font::bold });
    addAndMakeVisible (titleLabel);

    liveLabel.setText (juce::String::fromUTF8 ("●  LIVE"), juce::dontSendNotification);
    liveLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (77, 229, 138));
    liveLabel.setFont (juce::FontOptions { 11.0f, juce::Font::bold });
    addAndMakeVisible (liveLabel);

    configureLabel (heightLabel, "heightLabel", "HEIGHT");
    configureLabel (lensLabel, "lensLabel", "LENS");
    configureLabel (depthLabel, "depthLabel", "DEPTH");
    configureLabel (tiltLabel, "tiltLabel", "TILT");
    configureLabel (orbitLabel, "orbitLabel", "ORBIT");
    configureLabel (zoomLabel, "zoomLabel", "ZOOM");
    configureLabel (contrastLabel, "contrastLabel", "CONTRAST");
    configureLabel (smoothLabel, "smoothLabel", "SMOOTH");
    configureLabel (averageLabel, "averageLabel", "AVERAGE");
    configureLabel (historyLabel, "historyLabel", "HISTORY");
    configureLabel (viewLabel, "viewLabel", "VIEW");
    configureLabel (resolutionLabel, "resolutionLabel", "RESOLUTION");
    configureLabel (rangeLabel, "rangeLabel", "RANGE");

    configureSlider (heightSlider, "heightControl", "Height");
    configureSlider (lensSlider, "lensControl", "Lens");
    configureSlider (depthSlider, "depthControl", "Depth");
    configureSlider (tiltSlider, "tiltControl", "Tilt");
    configureSlider (orbitSlider, "orbitControl", "Orbit");
    configureSlider (zoomSlider, "zoomControl", "Zoom");
    configureSlider (contrastSlider, "contrastControl", "Contrast");
    configureSlider (smoothSlider, "smoothControl", "Smooth");
    configureSlider (averageSlider, "averageControl", "Average");
    configureSlider (historySlider, "historyControl", "History");

    heightSlider.setTextValueSuffix (juce::String::fromUTF8 ("×"));
    lensSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + "%";
    };
    depthSlider.setTextValueSuffix (juce::String::fromUTF8 ("×"));
    tiltSlider.setTextValueSuffix (juce::String::fromUTF8 ("°"));
    orbitSlider.setTextValueSuffix (juce::String::fromUTF8 ("°"));
    zoomSlider.setTextValueSuffix (juce::String::fromUTF8 ("×"));
    contrastSlider.setTextValueSuffix (juce::String::fromUTF8 ("×"));
    smoothSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + "%";
    };
    averageSlider.setTooltip ("Auditory-style temporal integration of spectral power");
    historySlider.setTextValueSuffix (" s");

    heightSlider.setDoubleClickReturnValue (true, 1.4);
    lensSlider.setDoubleClickReturnValue (true, 0.35);
    depthSlider.setDoubleClickReturnValue (true, 1.0);
    tiltSlider.setDoubleClickReturnValue (true, 55.0);
    orbitSlider.setDoubleClickReturnValue (true, 0.0);
    zoomSlider.setDoubleClickReturnValue (true, 1.0);
    contrastSlider.setDoubleClickReturnValue (true, 1.0);
    averageSlider.setDoubleClickReturnValue (true, 80.0);

    heightAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::height, heightSlider);
    lensAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::curve, lensSlider);
    depthAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::depth, depthSlider);
    tiltAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::tilt, tiltSlider);
    orbitAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::orbit, orbitSlider);
    zoomAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::zoom, zoomSlider);
    contrastAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::contrast, contrastSlider);
    smoothAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::smooth, smoothSlider);
    averageAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::averageMs, averageSlider);
    averageSlider.textFromValueFunction = [] (double value)
    {
        return value < 0.5 ? juce::String ("OFF")
                           : juce::String (juce::roundToInt (value)) + " ms";
    };
    historyAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::history, historySlider);

    for (auto* button : { &viewButton, &resolutionButton, &rangeButton,
                          &holdButton, &resetButton })
    {
        button->setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (43, 45, 52));
        button->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.82f));
        addAndMakeVisible (*button);
    }
    viewButton.setComponentID ("viewControl");
    viewButton.setTitle ("Switch between 3D and exact 2D view");
    viewButton.setClickingTogglesState (true);
    viewButton.onStateChange = [this] { updateViewState(); };
    viewAttachment = std::make_unique<ButtonAttachment> (state, ParameterIDs::view2d, viewButton);

    resolutionButton.setComponentID ("resolutionControl");
    resolutionButton.setTitle ("Analysis resolution");
    rangeButton.setComponentID ("rangeControl");
    rangeButton.setTitle ("Displayed frequency range");
    auto& resolutionParameter = *state.getParameter (ParameterIDs::resolution);
    auto& rangeParameter = *state.getParameter (ParameterIDs::range);
    resolutionAttachment = std::make_unique<juce::ParameterAttachment> (
        resolutionParameter,
        [this] (float value)
        {
            currentResolution = std::clamp (juce::roundToInt (value), 0, 2);
            updateChoiceText();
        }, state.undoManager);
    rangeAttachment = std::make_unique<juce::ParameterAttachment> (
        rangeParameter,
        [this] (float value)
        {
            currentRange = std::clamp (juce::roundToInt (value), 0, 3);
            updateChoiceText();
        }, state.undoManager);
    resolutionButton.onClick = [this]
    {
        resolutionAttachment->setValueAsCompleteGesture (static_cast<float> ((currentResolution + 1) % 3));
    };
    rangeButton.onClick = [this]
    {
        rangeAttachment->setValueAsCompleteGesture (static_cast<float> ((currentRange + 1) % 4));
    };
    resolutionAttachment->sendInitialUpdate();
    rangeAttachment->sendInitialUpdate();
    updateViewState();

    holdButton.setComponentID ("holdControl");
    holdButton.setTitle ("Hold spectral history");
    holdButton.setClickingTogglesState (true);
    holdButton.onClick = [this] { if (onHoldChanged) onHoldChanged (holdButton.getToggleState()); };
    resetButton.setComponentID ("resetControl");
    resetButton.setTitle ("Reset spectral history");
    resetButton.onClick = [this] { if (onReset) onReset(); };
}

int ControlStrip::getPreferredHeight (int availableWidth) noexcept
{
    return availableWidth < 1100 ? 166 : 92;
}

void ControlStrip::configureSlider (juce::Slider& slider,
                                    const juce::String& id,
                                    const juce::String& title)
{
    slider.setComponentID (id);
    slider.setTitle (title);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
    slider.setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (77, 229, 138));
    slider.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (48, 50, 58));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.84f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
}

void ControlStrip::configureLabel (juce::Label& label,
                                   const juce::String& id,
                                   const juce::String& text)
{
    label.setComponentID (id);
    label.setText (text, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.48f));
    label.setFont (juce::FontOptions { 9.0f, juce::Font::bold });
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void ControlStrip::updateChoiceText()
{
    constexpr std::array<const char*, 3> resolutionNames { "NORMAL", "HIGH", "ULTRA" };
    constexpr std::array<const char*, 4> rangeNames { "FULL", "LOW", "MID", "HIGH" };
    resolutionButton.setButtonText (resolutionNames[static_cast<std::size_t> (currentResolution)]);
    rangeButton.setButtonText (rangeNames[static_cast<std::size_t> (currentRange)]);
}

void ControlStrip::updateViewState()
{
    const auto is2d = viewButton.getToggleState();
    viewButton.setButtonText (is2d ? "2D" : "3D");
    const auto alpha = is2d ? 0.42f : 1.0f;
    for (auto* component : std::array<juce::Component*, 12> {
             &heightLabel, &heightSlider, &lensLabel, &lensSlider,
             &depthLabel, &depthSlider, &tiltLabel, &tiltSlider,
             &orbitLabel, &orbitSlider, &zoomLabel, &zoomSlider })
        component->setAlpha (alpha);
}

void ControlStrip::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour::fromRGB (23, 24, 28));
    graphics.setColour (juce::Colour::fromRGB (42, 44, 51));
    graphics.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
}

void ControlStrip::resized()
{
    auto area = getLocalBounds().reduced (10, 5);
    const auto setSliderGroup = [] (juce::Rectangle<int> group,
                                    juce::Label& label,
                                    juce::Slider& slider)
    {
        group = group.reduced (3, 0);
        label.setBounds (group.removeFromTop (13));
        slider.setBounds (group);
    };
    const auto setButtonGroup = [] (juce::Rectangle<int> group,
                                    juce::Label& label,
                                    juce::TextButton& button)
    {
        group = group.reduced (3, 0);
        label.setBounds (group.removeFromTop (13));
        button.setBounds (group.reduced (2, 1));
    };
    const auto setFiveGroups = [&setSliderGroup] (
                                   juce::Rectangle<int> row,
                                   std::array<std::pair<juce::Label*, juce::Slider*>, 5> groups)
    {
        auto remainingGroups = static_cast<int> (groups.size());
        for (const auto& [label, slider] : groups)
        {
            const auto groupWidth = row.getWidth() / remainingGroups--;
            setSliderGroup (row.removeFromLeft (groupWidth), *label, *slider);
        }
    };
    const auto setHeader = [this] (juce::Rectangle<int> header)
    {
        titleLabel.setBounds (header.removeFromLeft (126));
        liveLabel.setBounds (header.removeFromLeft (64));
        resetButton.setBounds (header.removeFromRight (58).reduced (2));
        holdButton.setBounds (header.removeFromRight (58).reduced (2));
        return header;
    };

    if (getWidth() >= 1100)
    {
        auto primaryRow = area.removeFromTop (area.getHeight() / 2);
        auto primaryControls = setHeader (primaryRow);
        setFiveGroups (primaryControls,
                       { std::pair { &heightLabel, &heightSlider },
                         std::pair { &lensLabel, &lensSlider },
                         std::pair { &depthLabel, &depthSlider },
                         std::pair { &tiltLabel, &tiltSlider },
                         std::pair { &orbitLabel, &orbitSlider } });
        auto secondaryRow = area;
        for (int remaining = 8; remaining > 0; --remaining)
        {
            const auto group = secondaryRow.removeFromLeft (secondaryRow.getWidth() / remaining);
            switch (8 - remaining)
            {
                case 0: setSliderGroup (group, zoomLabel, zoomSlider); break;
                case 1: setSliderGroup (group, contrastLabel, contrastSlider); break;
                case 2: setSliderGroup (group, smoothLabel, smoothSlider); break;
                case 3: setSliderGroup (group, averageLabel, averageSlider); break;
                case 4: setSliderGroup (group, historyLabel, historySlider); break;
                case 5: setButtonGroup (group, viewLabel, viewButton); break;
                case 6: setButtonGroup (group, resolutionLabel, resolutionButton); break;
                case 7: setButtonGroup (group, rangeLabel, rangeButton); break;
            }
        }
        return;
    }

    setHeader (area.removeFromTop (32));
    auto primaryRow = area.removeFromTop (area.getHeight() / 3);
    setFiveGroups (primaryRow,
                   { std::pair { &heightLabel, &heightSlider },
                     std::pair { &lensLabel, &lensSlider },
                     std::pair { &depthLabel, &depthSlider },
                     std::pair { &tiltLabel, &tiltSlider },
                     std::pair { &orbitLabel, &orbitSlider } });
    auto secondaryRow = area.removeFromTop (area.getHeight() / 2);
    const auto secondaryWidth = secondaryRow.getWidth() / 5;
    setSliderGroup (secondaryRow.removeFromLeft (secondaryWidth), zoomLabel, zoomSlider);
    setSliderGroup (secondaryRow.removeFromLeft (secondaryWidth), contrastLabel, contrastSlider);
    setSliderGroup (secondaryRow.removeFromLeft (secondaryWidth), smoothLabel, smoothSlider);
    setSliderGroup (secondaryRow.removeFromLeft (secondaryWidth), averageLabel, averageSlider);
    setSliderGroup (secondaryRow, historyLabel, historySlider);

    const auto compactWidth = area.getWidth() / 3;
    setButtonGroup (area.removeFromLeft (compactWidth), viewLabel, viewButton);
    setButtonGroup (area.removeFromLeft (compactWidth), resolutionLabel, resolutionButton);
    setButtonGroup (area, rangeLabel, rangeButton);
}
