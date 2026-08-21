#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
inline constexpr auto height = "height";
inline constexpr auto curve = "curve";
inline constexpr auto depth = "depth";
inline constexpr auto tilt = "tilt";
inline constexpr auto orbit = "orbit";
inline constexpr auto zoom = "zoom";
inline constexpr auto contrast = "contrast";
inline constexpr auto resolution = "resolution";
inline constexpr auto range = "range";
inline constexpr auto view2d = "view2d";
inline constexpr auto smooth = "smooth";
inline constexpr auto averageMs = "averageMs";
inline constexpr auto history = "history";
// Read only while migrating state written by builds before Resolution existed.
inline constexpr auto qualityHigh = "qualityHigh";
} // namespace ParameterIDs

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
