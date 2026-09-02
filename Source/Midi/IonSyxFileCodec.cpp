#include "IonSyxFileCodec.h"

namespace aim
{
namespace
{
juce::Result validatePayload (const std::uint8_t* payload, std::size_t payloadSize)
{
    if (payload == nullptr || payloadSize == 0)
        return juce::Result::fail ("SysEx payload is empty");

    for (std::size_t i = 0; i < payloadSize; ++i)
        if ((payload[i] & 0x80u) != 0u)
            return juce::Result::fail ("SysEx payload contains a MIDI status byte");

    return juce::Result::ok();
}
}

juce::MemoryBlock IonSyxFileCodec::encodeFileBytes (const juce::MidiMessage& message)
{
    return encodeFileBytes (std::vector<juce::MidiMessage> { message });
}

juce::MemoryBlock IonSyxFileCodec::encodeFileBytes (const std::vector<juce::MidiMessage>& messages)
{
    juce::MemoryBlock result;
    const std::uint8_t start = 0xf0;
    const std::uint8_t end = 0xf7;

    for (const auto& message : messages)
    {
        if (! message.isSysEx())
            return {};

        result.append (&start, 1);
        result.append (message.getSysExData(), static_cast<std::size_t> (message.getSysExDataSize()));
        result.append (&end, 1);
    }

    return result;
}

juce::Result IonSyxFileCodec::decodeFileBytes (const void* data,
                                                std::size_t size,
                                                juce::MidiMessage& message)
{
    message = {};
    std::vector<juce::MidiMessage> messages;
    if (const auto result = decodeFileMessages (data, size, messages); result.failed())
        return result;

    if (messages.size() != 1)
        return juce::Result::fail ("Expected exactly one SysEx message, found " + juce::String (static_cast<juce::int64> (messages.size())));

    message = messages.front();
    return juce::Result::ok();
}

juce::Result IonSyxFileCodec::decodeFileMessages (const void* data,
                                                   std::size_t size,
                                                   std::vector<juce::MidiMessage>& messages)
{
    messages.clear();
    if (data == nullptr || size == 0)
        return juce::Result::fail ("SysEx file is empty");

    const auto* bytes = static_cast<const std::uint8_t*> (data);

    // Raw status-free payloads are useful during protocol research and are
    // accepted as a single message. A standard .syx begins with F0.
    if (bytes[0] != 0xf0)
    {
        if (const auto result = validatePayload (bytes, size); result.failed())
            return result;
        messages.push_back (juce::MidiMessage::createSysExMessage (bytes, static_cast<int> (size)));
        return juce::Result::ok();
    }

    std::size_t cursor = 0;
    while (cursor < size)
    {
        if (bytes[cursor] != 0xf0)
            return juce::Result::fail ("Unexpected byte between SysEx messages at file offset " + juce::String (static_cast<juce::int64> (cursor)));

        const auto payloadBegin = cursor + 1u;
        auto end = payloadBegin;
        while (end < size && bytes[end] != 0xf7)
            ++end;

        if (end >= size)
            return juce::Result::fail ("SysEx message is missing its terminating F7 byte");

        const auto payloadSize = end - payloadBegin;
        if (const auto result = validatePayload (bytes + payloadBegin, payloadSize); result.failed())
            return result;

        messages.push_back (juce::MidiMessage::createSysExMessage (bytes + payloadBegin,
                                                                    static_cast<int> (payloadSize)));
        cursor = end + 1u;
    }

    if (messages.empty())
        return juce::Result::fail ("SysEx file contains no messages");

    return juce::Result::ok();
}

juce::Result IonSyxFileCodec::loadFromFile (const juce::File& file, juce::MidiMessage& message)
{
    juce::MemoryBlock bytes;
    if (! file.loadFileAsData (bytes))
        return juce::Result::fail ("Could not read SysEx file: " + file.getFullPathName());

    return decodeFileBytes (bytes.getData(), bytes.getSize(), message);
}

juce::Result IonSyxFileCodec::loadMessagesFromFile (const juce::File& file,
                                                     std::vector<juce::MidiMessage>& messages)
{
    juce::MemoryBlock bytes;
    if (! file.loadFileAsData (bytes))
        return juce::Result::fail ("Could not read SysEx file: " + file.getFullPathName());

    return decodeFileMessages (bytes.getData(), bytes.getSize(), messages);
}

juce::Result IonSyxFileCodec::saveToFile (const juce::File& file, const juce::MidiMessage& message)
{
    return saveToFile (file, std::vector<juce::MidiMessage> { message });
}

juce::Result IonSyxFileCodec::saveToFile (const juce::File& file,
                                           const std::vector<juce::MidiMessage>& messages)
{
    if (messages.empty())
        return juce::Result::fail ("There are no SysEx messages to write");

    const auto bytes = encodeFileBytes (messages);
    if (bytes.getSize() == 0)
        return juce::Result::fail ("Only SysEx MIDI messages can be written as .syx");

    if (! file.replaceWithData (bytes.getData(), bytes.getSize()))
        return juce::Result::fail ("Could not write SysEx file: " + file.getFullPathName());

    return juce::Result::ok();
}
}
