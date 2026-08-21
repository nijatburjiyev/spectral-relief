#include <juce_events/juce_events.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseGui;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult (i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
