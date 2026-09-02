#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <vector>

namespace aim
{
/** Standard .syx file framing helper.

    Standard files contain one or more F0 ... F7 messages concatenated. For
    reverse-engineering convenience, a raw status-free payload with no F0/F7
    wrapper is also accepted as one message.
*/
class IonSyxFileCodec
{
public:
    [[nodiscard]] static juce::MemoryBlock encodeFileBytes (const juce::MidiMessage& message);
    [[nodiscard]] static juce::MemoryBlock encodeFileBytes (const std::vector<juce::MidiMessage>& messages);

    static juce::Result decodeFileBytes (const void* data, std::size_t size, juce::MidiMessage& message);
    static juce::Result decodeFileMessages (const void* data,
                                            std::size_t size,
                                            std::vector<juce::MidiMessage>& messages);

    static juce::Result loadFromFile (const juce::File& file, juce::MidiMessage& message);
    static juce::Result loadMessagesFromFile (const juce::File& file, std::vector<juce::MidiMessage>& messages);
    static juce::Result saveToFile (const juce::File& file, const juce::MidiMessage& message);
    static juce::Result saveToFile (const juce::File& file, const std::vector<juce::MidiMessage>& messages);
};
}
