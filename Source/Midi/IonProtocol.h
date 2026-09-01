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
    // Generic MIDI NRPN framing. This follows the normal 14-bit MIDI layout
    // and does not apply any Ion-specific signed-value semantics.
    static std::array<juce::MidiMessage, 4> makeNrpnSequence (int midiChannel,
                                                              int nrpnNumber,
                                                              int value14Bit);

    // Candidate Alesis/Ion value representation from community reverse
    // engineering. Negative values wrap through the unsigned 14-bit space:
    // -1 -> 16383, -100 -> 16284. This deliberately remains separate from
    // generic NRPN framing until verified against hardware captures.
    static int encodeIonSigned14 (int signedValue) noexcept;
    static int decodeIonSigned14 (int encodedValue) noexcept;

    static std::array<juce::MidiMessage, 4> makeIonNrpnSequence (int midiChannel,
                                                                 int nrpnNumber,
                                                                 int signedValue);

    // Creates a SysEx message from payload bytes without inventing any Ion
    // manufacturer/model framing. Use IonSysExCodec for candidate Ion messages.
    static juce::MidiMessage makeSysExFromPayload (const std::vector<std::uint8_t>& payload);

    static bool isSysEx (const juce::MidiMessage& message) noexcept { return message.isSysEx(); }
};
}
