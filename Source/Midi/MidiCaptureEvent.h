#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <utility>

namespace aim
{
enum class MidiDirection
{
    input,
    output
};

struct MidiCaptureEvent
{
    MidiCaptureEvent (MidiDirection directionIn,
                      std::int64_t utcMillisecondsIn,
                      juce::String deviceIdentifierIn,
                      const juce::MidiMessage& messageIn)
        : direction (directionIn),
          utcMilliseconds (utcMillisecondsIn),
          deviceIdentifier (std::move (deviceIdentifierIn)),
          message (messageIn)
    {}

    MidiDirection direction;
    std::int64_t utcMilliseconds;
    juce::String deviceIdentifier;
    juce::MidiMessage message;

    [[nodiscard]] bool isSysEx() const noexcept { return message.isSysEx(); }
    [[nodiscard]] juce::String directionName() const;
    [[nodiscard]] juce::String messageKind() const;
    [[nodiscard]] juce::String hexBytes() const;
    [[nodiscard]] juce::var toJson() const;
};
}
