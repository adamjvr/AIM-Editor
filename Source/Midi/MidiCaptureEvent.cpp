#include "MidiCaptureEvent.h"

namespace aim
{
juce::String MidiCaptureEvent::directionName() const
{
    return direction == MidiDirection::input ? "input" : "output";
}

juce::String MidiCaptureEvent::messageKind() const
{
    if (message.isSysEx())              return "sysex";
    if (message.isNoteOn())             return "note_on";
    if (message.isNoteOff())            return "note_off";
    if (message.isController())         return "control_change";
    if (message.isProgramChange())      return "program_change";
    if (message.isPitchWheel())         return "pitch_wheel";
    if (message.isAftertouch())         return "poly_aftertouch";
    if (message.isChannelPressure())    return "channel_pressure";
    if (message.isMidiClock())          return "clock";
    if (message.isMidiStart())          return "start";
    if (message.isMidiContinue())       return "continue";
    if (message.isMidiStop())           return "stop";
    if (message.isActiveSense())        return "active_sense";
    if (message.isMetaEvent())          return "meta";
    return "other";
}

juce::String MidiCaptureEvent::hexBytes() const
{
    juce::StringArray bytes;
    const auto* data = message.getRawData();

    for (int i = 0; i < message.getRawDataSize(); ++i)
        bytes.add (juce::String::formatted ("%02X", static_cast<unsigned int> (data[i])));

    return bytes.joinIntoString (" ");
}

juce::var MidiCaptureEvent::toJson() const
{
    auto* object = new juce::DynamicObject();
    object->setProperty ("direction", directionName());
    object->setProperty ("utc_ms", static_cast<juce::int64> (utcMilliseconds));
    object->setProperty ("device_identifier", deviceIdentifier);
    object->setProperty ("kind", messageKind());
    object->setProperty ("size", message.getRawDataSize());
    object->setProperty ("hex", hexBytes());

    juce::Array<juce::var> rawBytes;
    const auto* data = message.getRawData();
    rawBytes.ensureStorageAllocated (message.getRawDataSize());
    for (int i = 0; i < message.getRawDataSize(); ++i)
        rawBytes.add (static_cast<int> (data[i]));
    object->setProperty ("bytes", juce::var (rawBytes));

    if (message.isSysEx())
    {
        juce::StringArray payloadBytes;
        const auto* payload = message.getSysExData();
        const auto payloadSize = message.getSysExDataSize();
        for (int i = 0; i < payloadSize; ++i)
            payloadBytes.add (juce::String::formatted ("%02X", static_cast<unsigned int> (payload[i])));

        object->setProperty ("sysex_payload_size", payloadSize);
        object->setProperty ("sysex_payload_hex", payloadBytes.joinIntoString (" "));
    }

    return juce::var (object);
}
}
