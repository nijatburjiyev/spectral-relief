#pragma once

#include <juce_events/juce_events.h>

#include <functional>
#include <utility>

class RenderRepaintDriver final : private juce::Timer
{
public:
    explicit RenderRepaintDriver (std::function<void()> requestRepaint)
        : repaintRequest (std::move (requestRepaint))
    {
    }

    ~RenderRepaintDriver() override { stop(); }

    void start (int framesPerSecond)
    {
        startTimerHz (juce::jlimit (1, 120, framesPerSecond));
    }

    void stop() { stopTimer(); }

private:
    void timerCallback() override
    {
        if (repaintRequest)
            repaintRequest();
    }

    std::function<void()> repaintRequest;
};
