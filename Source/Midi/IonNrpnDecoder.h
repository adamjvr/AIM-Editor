#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <optional>

namespace aim
{
struct DecodedNrpn
{
    int midiChannel = 1;
    int parameter = 0;
    int value14Bit = 0;
};

/** Small state machine for the candidate Ion NRPN transport.

    The decoder waits for the full CC99/CC98 address and CC6/CC38 data-entry
    pair. It deliberately does not guess a semantic parameter; that mapping is
    owned by the JSON ParameterRegistry.
*/
class IonNrpnDecoder
{
public:
    std::optional<DecodedNrpn> push (const juce::MidiMessage& message);
    void reset() noexcept;

private:
    struct ChannelState
    {
        int addressMsb = -1;
        int addressLsb = -1;
        int dataMsb = -1;
    };

    std::array<ChannelState, 16> channels {};
};
}
