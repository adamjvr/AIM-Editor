#include "IonProtocol.h"

#include <algorithm>

namespace aim
{
std::array<juce::MidiMessage, 4> IonProtocol::makeNrpnSequence (int midiChannel,
                                                                int nrpnNumber,
                                                                int value14Bit)
{
    midiChannel = std::clamp (midiChannel, 1, 16);
    nrpnNumber = std::clamp (nrpnNumber, 0, 16383);
    value14Bit = std::clamp (value14Bit, 0, 16383);

    const auto nrpnMsb = (nrpnNumber >> 7) & 0x7f;
    const auto nrpnLsb = nrpnNumber & 0x7f;
    const auto valueMsb = (value14Bit >> 7) & 0x7f;
    const auto valueLsb = value14Bit & 0x7f;

    return {
        juce::MidiMessage::controllerEvent (midiChannel, 99, nrpnMsb),
        juce::MidiMessage::controllerEvent (midiChannel, 98, nrpnLsb),
        juce::MidiMessage::controllerEvent (midiChannel, 6, valueMsb),
        juce::MidiMessage::controllerEvent (midiChannel, 38, valueLsb)
    };
}

int IonProtocol::encodeIonSigned14 (int signedValue) noexcept
{
    signedValue = std::clamp (signedValue, -8192, 8191);
    return signedValue < 0 ? 16384 + signedValue : signedValue;
}

int IonProtocol::decodeIonSigned14 (int encodedValue) noexcept
{
    encodedValue = std::clamp (encodedValue, 0, 16383);
    return encodedValue >= 8192 ? encodedValue - 16384 : encodedValue;
}

std::array<juce::MidiMessage, 4> IonProtocol::makeIonNrpnSequence (int midiChannel,
                                                                   int nrpnNumber,
                                                                   int signedValue)
{
    return makeNrpnSequence (midiChannel, nrpnNumber, encodeIonSigned14 (signedValue));
}

juce::MidiMessage IonProtocol::makeSysExFromPayload (const std::vector<std::uint8_t>& payload)
{
    if (payload.empty())
        return juce::MidiMessage::createSysExMessage (nullptr, 0);

    return juce::MidiMessage::createSysExMessage (payload.data(), static_cast<int> (payload.size()));
}
}
