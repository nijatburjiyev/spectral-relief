cmake_minimum_required(VERSION 3.25)

project(SpectralRelief VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53)
FetchContent_MakeAvailable(JUCE)

enable_testing()

set(SPECTRAL_RELIEF_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.cpp
    Source/Parameters.h
    Source/Analysis/AnalysisQuality.h
    Source/Analysis/FrequencyRange.h
    Source/Analysis/AudioSampleQueue.h
    Source/Analysis/PerceptualPowerAverager.cpp
    Source/Analysis/PerceptualPowerAverager.h
    Source/Analysis/SpectrumAnalysisWorker.cpp
    Source/Analysis/SpectrumAnalysisWorker.h
    Source/Analysis/SpectrumAnalyzer.cpp
    Source/Analysis/SpectrumAnalyzer.h
    Source/Diagnostics/RuntimeDiagnostics.h
    Source/Rendering/DisplayRowProcessor.cpp
    Source/Rendering/DisplayRowProcessor.h
    Source/Rendering/FrequencyGuides.cpp
    Source/Rendering/FrequencyGuides.h
    Source/Rendering/CameraProjection.cpp
    Source/Rendering/CameraProjection.h
    Source/Rendering/RenderRepaintDriver.h
    Source/Rendering/SurfaceMesh.cpp
    Source/Rendering/SurfaceMesh.h
    Source/Rendering/SurfaceResources.h
    Source/Rendering/SpectralSurfaceComponent.cpp
    Source/Rendering/SpectralSurfaceComponent.h
    Source/Rendering/SpectralShaders.h
    Source/Rendering/SpectrumFrameAggregator.h
    Source/UI/ControlStrip.cpp
    Source/UI/ControlStrip.h)

juce_add_plugin(SpectralRelief
    COMPANY_NAME "Nijat Burjiyev"
    BUNDLE_ID com.nijatburjiyev.spectralrelief
    PLUGIN_MANUFACTURER_CODE Nbsp
    PLUGIN_CODE SpRl
    FORMATS VST3
    PRODUCT_NAME "Spectral Relief"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(SpectralRelief PRIVATE ${SPECTRAL_RELIEF_SOURCES})
target_include_directories(SpectralRelief PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(SpectralRelief PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SpectralRelief PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

juce_add_console_app(SpectralReliefTests PRODUCT_NAME "Spectral Relief Tests")
target_sources(SpectralReliefTests PRIVATE
    ${SPECTRAL_RELIEF_SOURCES}
    Tests/TestMain.cpp
    Tests/PluginProcessorTests.cpp
    Tests/AudioSampleQueueTests.cpp
    Tests/SpectrumAnalysisWorkerTests.cpp
    Tests/SpectrumFrameQueueTests.cpp
    Tests/SpectrumAnalyzerTests.cpp
    Tests/PerceptualPowerAveragerTests.cpp
    Tests/ParametersTests.cpp
    Tests/DisplayRowProcessorTests.cpp
    Tests/FrequencyGuidesTests.cpp
    Tests/CameraProjectionTests.cpp
    Tests/RenderRepaintDriverTests.cpp
    Tests/RuntimeDiagnosticsTests.cpp
    Tests/SurfaceMeshTests.cpp
    Tests/SpectralShadersTests.cpp
    Tests/EditorTests.cpp
    Tests/LifecycleTests.cpp)
target_include_directories(SpectralReliefTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(SpectralReliefTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(SpectralReliefTests PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME SpectralReliefTests COMMAND SpectralReliefTests)
add_test(
    NAME PublicReleaseAudit
    COMMAND ${CMAKE_COMMAND}
        -DREPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/PublicReleaseAudit.cmake)
