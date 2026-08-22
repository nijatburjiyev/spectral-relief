#include <juce_events/juce_events.h>

#include <iostream>

namespace
{
class ConsoleUnitTestRunner final : public juce::UnitTestRunner
{
protected:
    void logMessage (const juce::String& message) override
    {
        std::cout << message << std::endl;
    }
};
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseGui;
    ConsoleUnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult (i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
