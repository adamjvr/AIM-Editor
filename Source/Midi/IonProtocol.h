#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstdint>
#include <vector>

namespace aim
{
class IonProtocol
{
public:
    // Generic MIDI NRPN framing. Ion-specific parameter numbers are supplied
    // by the verified JSON database rather than hard-coded here.
    static std::array<juce::MidiMessage, 4> makeNrpnSequence (int midiChannel,
                                                              int nrpnNumber,
                                                              int value14Bit);

    // Creates a SysEx message from payload bytes without inventing any Ion
    // manufacturer/model framing. That framing will be added once verified.
    static juce::MidiMessage makeSysExFromPayload (const std::vector<std::uint8_t>& payload);

    static bool isSysEx (const juce::MidiMessage& message) noexcept { return message.isSysEx(); }
};
}
