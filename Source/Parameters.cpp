#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::height, 1 },
        "Height",
        juce::NormalisableRange<float> { 0.25f, 3.0f, 0.01f },
        1.4f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::curve, 1 },
        "Lens",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f },
        0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::depth, 1 },
        "Depth",
        juce::NormalisableRange<float> { 0.5f, 2.0f, 0.01f },
        1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::tilt, 1 },
        "Tilt",
        juce::NormalisableRange<float> { 15.0f, 90.0f, 0.1f },
        55.0f,
        juce::AudioParameterFloatAttributes().withLabel ("deg")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::orbit, 1 },
        "Orbit",
        juce::NormalisableRange<float> { -180.0f, 180.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("deg")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::zoom, 1 },
        "Zoom",
        juce::NormalisableRange<float> { 0.65f, 1.6f, 0.01f },
        1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::contrast, 1 },
        "Contrast",
        juce::NormalisableRange<float> { 0.5f, 2.5f, 0.01f },
        1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::smooth, 1 },
        "Smooth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f },
        0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::averageMs, 1 },
        "Average",
        juce::NormalisableRange<float> { 0.0f, 1000.0f, 1.0f, 0.35f },
        80.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float value, int)
            {
                return value < 0.5f ? juce::String ("OFF")
                                    : juce::String (juce::roundToInt (value)) + " ms";
            })));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::history, 1 },
        "History",
        juce::NormalisableRange<float> { 2.0f, 8.0f, 0.1f },
        5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::resolution, 1 },
        "Resolution",
        juce::StringArray { "Normal", "High", "Ultra" },
        0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::range, 1 },
        "Range",
        juce::StringArray { "Full", "Low", "Mid", "High" },
        0));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParameterIDs::view2d, 1 },
        "View 2D",
        false));
    return layout;
}
