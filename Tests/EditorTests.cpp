#include "Source/PluginEditor.h"
#include "Source/PluginProcessor.h"

#include <juce_core/juce_core.h>

namespace
{
juce::Component* findDescendant (juce::Component& parent, const juce::String& id)
{
    if (parent.getComponentID() == id)
        return &parent;

    for (int index = 0; index < parent.getNumChildComponents(); ++index)
        if (auto* result = findDescendant (*parent.getChildComponent (index), id))
            return result;

    return nullptr;
}

class EditorTests final : public juce::UnitTest
{
public:
    EditorTests() : juce::UnitTest ("Plugin editor", "editor") {}

    void runTest() override
    {
        SpectralReliefAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

        beginTest ("editor opens at the approved size and limits");
        expectEquals (editor->getWidth(), 900);
        expectEquals (editor->getHeight(), 560);
        expect (editor->getConstrainer() != nullptr);
        expectEquals (editor->getConstrainer()->getMinimumWidth(), 640);
        expectEquals (editor->getConstrainer()->getMinimumHeight(), 400);

        beginTest ("all controls expose stable accessible identities");
        for (const auto* id : { "heightControl", "lensControl", "depthControl", "tiltControl",
                                "orbitControl", "zoomControl", "contrastControl", "smoothControl",
                                "averageControl", "historyControl", "viewControl", "resolutionControl", "rangeControl",
                                "holdControl", "resetControl",
                                "spectralSurface" })
            expect (findDescendant (*editor, id) != nullptr, juce::String ("missing ") + id);
        expect (findDescendant (*editor, "curveControl") == nullptr);

        beginTest ("control names are permanently visible");
        for (const auto* id : { "heightLabel", "lensLabel", "depthLabel", "tiltLabel",
                                "orbitLabel", "zoomLabel", "contrastLabel", "smoothLabel",
                                "averageLabel", "historyLabel", "viewLabel", "resolutionLabel", "rangeLabel" })
        {
            const auto* label = findDescendant (*editor, id);
            expect (label != nullptr && label->isVisible()
                                      && label->getWidth() > 0 && label->getHeight() > 0,
                    juce::String ("missing visible ") + id);
        }
        const auto* lensLabel = dynamic_cast<juce::Label*> (findDescendant (*editor, "lensLabel"));
        expect (lensLabel != nullptr);
        if (lensLabel != nullptr)
            expectEquals (lensLabel->getText(), juce::String ("LENS"));
        expect (ControlStrip::getPreferredHeight (640) > ControlStrip::getPreferredHeight (1400));

        auto* average = dynamic_cast<juce::Slider*> (
            findDescendant (*editor, "averageControl"));
        expect (average != nullptr);
        if (average != nullptr)
        {
            expectEquals (average->getTextFromValue (0.0), juce::String ("OFF"));
            expectEquals (average->getTextFromValue (80.0), juce::String ("80 ms"));
            expectEquals (average->getTextFromValue (1000.0), juce::String ("1000 ms"));
        }

        const auto* resolution = dynamic_cast<const juce::TextButton*> (
            findDescendant (*editor, "resolutionControl"));
        const auto* range = dynamic_cast<const juce::TextButton*> (
            findDescendant (*editor, "rangeControl"));
        auto* view = dynamic_cast<juce::TextButton*> (findDescendant (*editor, "viewControl"));
        expect (resolution != nullptr && resolution->getButtonText() == "NORMAL");
        expect (range != nullptr && range->getButtonText() == "FULL");
        expect (view != nullptr && view->getButtonText() == "3D");

        beginTest ("2D view visibly dims inactive camera controls");
        expect (view != nullptr);
        if (view != nullptr)
        {
            view->setToggleState (true, juce::sendNotification);
            const auto* height = findDescendant (*editor, "heightControl");
            const auto* orbit = findDescendant (*editor, "orbitControl");
            expect (view->getButtonText() == "2D");
            expect (height != nullptr && height->getAlpha() < 0.7f);
            expect (orbit != nullptr && orbit->getAlpha() < 0.7f);
            view->setToggleState (false, juce::sendNotification);
        }

        beginTest ("OpenGL component overlay remains transparent");
        const auto* spectralSurface = findDescendant (*editor, "spectralSurface");
        expect (spectralSurface != nullptr && ! spectralSurface->isOpaque());

        beginTest ("minimum and large layouts keep children visible");
        for (const auto size : { juce::Point<int> { 640, 400 }, juce::Point<int> { 900, 560 },
                                 juce::Point<int> { 1400, 850 } })
        {
            editor->setSize (size.x, size.y);
            for (const auto* id : { "heightControl", "lensControl", "depthControl", "tiltControl",
                                    "orbitControl", "zoomControl", "contrastControl", "smoothControl",
                                    "averageControl", "historyControl", "heightLabel", "lensLabel",
                                    "viewControl", "resolutionControl", "rangeControl",
                                    "depthLabel", "tiltLabel", "orbitLabel", "zoomLabel",
                                    "contrastLabel", "smoothLabel", "averageLabel", "historyLabel", "viewLabel",
                                    "resolutionLabel", "rangeLabel",
                                    "holdControl", "resetControl", "spectralSurface" })
            {
                const auto* child = findDescendant (*editor, id);
                expect (child != nullptr && child->isVisible()
                                         && child->getWidth() > 0 && child->getHeight() > 0,
                        juce::String ("invalid layout for ") + id + " at "
                            + juce::String (size.x) + "x" + juce::String (size.y));
            }

            const auto* averageControl = findDescendant (*editor, "averageControl");
            for (const auto* id : { "zoomControl", "contrastControl", "smoothControl",
                                    "historyControl", "viewControl", "resolutionControl", "rangeControl" })
            {
                const auto* other = findDescendant (*editor, id);
                expect (averageControl != nullptr && other != nullptr
                        && ! averageControl->getBounds().intersects (other->getBounds()),
                        juce::String ("average overlaps ") + id + " at "
                            + juce::String (size.x) + "x" + juce::String (size.y));
            }
        }

        beginTest ("editor dimensions persist in plugin state");
        editor->setSize (1110, 710);
        juce::MemoryBlock state;
        processor.getStateInformation (state);
        SpectralReliefAudioProcessor restored;
        restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        std::unique_ptr<juce::AudioProcessorEditor> restoredEditor (restored.createEditor());
        expectEquals (restoredEditor->getWidth(), 1110);
        expectEquals (restoredEditor->getHeight(), 710);
    }
};

EditorTests editorTests;
} // namespace
