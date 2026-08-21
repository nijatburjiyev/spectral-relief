#include "Source/Analysis/AudioSampleQueue.h"

#include <juce_core/juce_core.h>

namespace
{
class AudioSampleQueueTests final : public juce::UnitTest
{
public:
    AudioSampleQueueTests() : juce::UnitTest ("Audio sample queue", "queue") {}

    void runTest() override
    {
        beginTest ("fixed queue preserves FIFO order and rejects overflow");
        AudioSampleQueue queue;
        for (std::size_t index = 0; index < AudioSampleQueue::capacity; ++index)
            expect (queue.tryPush (static_cast<float> (index)));
        expect (! queue.tryPush (-1.0f));

        float value = 0.0f;
        for (std::size_t index = 0; index < AudioSampleQueue::capacity; ++index)
        {
            expect (queue.tryPop (value));
            expectEquals (value, static_cast<float> (index));
        }
        expect (! queue.tryPop (value));

        beginTest ("consumer discard makes all capacity available");
        expect (queue.tryPush (1.0f));
        expect (queue.tryPush (2.0f));
        queue.discard();
        expect (! queue.tryPop (value));
        expect (queue.tryPush (3.0f));
        expect (queue.tryPop (value));
        expectEquals (value, 3.0f);

        beginTest ("boundary discard preserves samples published after Reset");
        expect (queue.tryPush (10.0f));
        expect (queue.tryPush (11.0f));
        const auto resetBoundary = queue.getWriteSequence();
        expect (queue.tryPush (20.0f));
        expect (queue.tryPush (21.0f));
        queue.discardBefore (resetBoundary);
        expect (queue.tryPop (value));
        expectEquals (value, 20.0f);
        expect (queue.tryPop (value));
        expectEquals (value, 21.0f);
        expect (! queue.tryPop (value));

        beginTest ("sequence metadata identifies a post-Reset suffix already in a local batch");
        expect (queue.tryPush (30.0f));
        expect (queue.tryPush (31.0f));
        const auto localResetBoundary = queue.getWriteSequence();
        expect (queue.tryPush (40.0f));
        expect (queue.tryPush (41.0f));
        std::array<float, 4> localBatch {};
        std::array<std::uint64_t, 4> localSequences {};
        for (std::size_t index = 0; index < localBatch.size(); ++index)
            expect (queue.tryPop (localBatch[index], localSequences[index]));
        const auto postReset = std::lower_bound (localSequences.begin(), localSequences.end(),
                                                 localResetBoundary);
        const auto postResetOffset = static_cast<std::size_t> (
            std::distance (localSequences.begin(), postReset));
        expectEquals (postResetOffset, static_cast<std::size_t> (2));
        expectEquals (localBatch[postResetOffset], 40.0f);
        expectEquals (localBatch[postResetOffset + 1], 41.0f);

        beginTest ("sample storage stays within the approved budget");
        expectEquals (AudioSampleQueue::payloadBytes,
                      AudioSampleQueue::capacity * sizeof (float));
        expect (AudioSampleQueue::payloadBytes <= 256 * 1024);
    }
};

AudioSampleQueueTests audioSampleQueueTests;
} // namespace
