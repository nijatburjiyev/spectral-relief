#include "Source/Rendering/RenderRepaintDriver.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace
{
class RenderRepaintDriverTests final : public juce::UnitTest
{
public:
    RenderRepaintDriverTests() : juce::UnitTest ("Render repaint driver", "rendering") {}

    void runTest() override
    {
        beginTest ("timer requests multiple frames without a display-link callback");
        int repaintRequests = 0;
        RenderRepaintDriver driver ([&repaintRequests] { ++repaintRequests; });
        driver.start (60);
        for (int tick = 0; tick < 4; ++tick)
        {
            juce::Thread::sleep (25);
            juce::Timer::callPendingTimersSynchronously();
        }
        driver.stop();

        expect (repaintRequests >= 3,
                "expected repeated repaint requests, got " + juce::String (repaintRequests));
    }
};

RenderRepaintDriverTests renderRepaintDriverTests;
} // namespace
