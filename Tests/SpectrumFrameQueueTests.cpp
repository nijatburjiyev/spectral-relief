#include "Source/Analysis/SpectrumFrameQueue.h"

#include <juce_core/juce_core.h>

namespace
{
class SpectrumFrameQueueTests final : public juce::UnitTest
{
public:
    SpectrumFrameQueueTests() : juce::UnitTest ("Spectrum frame queue", "queue") {}

    void runTest() override
    {
        beginTest ("eight rows preserve FIFO ordering");
        SpectrumFrameQueue queue;
        expectEquals (static_cast<int> (maximumSpectrumBandCount), 1024);
        expect (SpectrumFrameQueue::payloadBytes <= 40 * 1024);

        for (int sequence = 0; sequence < 8; ++sequence)
        {
            SpectrumRow row;
            row.fill (static_cast<float> (sequence));
            expect (queue.tryPush (row));
        }

        SpectrumRow overflow;
        overflow.fill (99.0f);
        expect (! queue.tryPush (overflow));

        for (int sequence = 0; sequence < 8; ++sequence)
        {
            SpectrumRow row;
            expect (queue.tryPop (row));
            expectEquals (row[0], static_cast<float> (sequence));
            expectEquals (row[137], static_cast<float> (sequence));
            expectEquals (row[255], static_cast<float> (sequence));
        }

        SpectrumRow empty;
        expect (! queue.tryPop (empty));

        beginTest ("a pop makes capacity available again");
        SpectrumRow row;
        row.fill (7.0f);
        for (int i = 0; i < 8; ++i)
            expect (queue.tryPush (row));
        expect (queue.tryPop (empty));
        expect (queue.tryPush (row));

        beginTest ("discard is serviced by the consumer");
        SpectrumFrameQueue discardQueue;
        row.fill (1.0f);
        expect (discardQueue.tryPush (row));
        row.fill (2.0f);
        expect (discardQueue.tryPush (row));
        discardQueue.requestDiscard();
        discardQueue.serviceDiscardRequest();
        expect (! discardQueue.tryPop (empty));
        row.fill (3.0f);
        expect (discardQueue.tryPush (row));
        expect (discardQueue.tryPop (empty));
        expectEquals (empty[137], 3.0f);
    }
};

SpectrumFrameQueueTests spectrumFrameQueueTests;
} // namespace
