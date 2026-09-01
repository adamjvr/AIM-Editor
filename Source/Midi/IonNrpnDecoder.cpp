#include "IonNrpnDecoder.h"

#include <algorithm>

namespace aim
{
std::optional<DecodedNrpn> IonNrpnDecoder::push (const juce::MidiMessage& message)
{
    if (! message.isController())
        return std::nullopt;

    const auto channel = std::clamp (message.getChannel(), 1, 16);
    auto& state = channels[static_cast<std::size_t> (channel - 1)];
    const auto controller = message.getControllerNumber();
    const auto value = std::clamp (message.getControllerValue(), 0, 127);

    switch (controller)
    {
        case 99: // NRPN address MSB
            state.addressMsb = value;
            state.addressLsb = -1;
            state.dataMsb = -1;
            break;

        case 98: // NRPN address LSB
            state.addressLsb = value;
            state.dataMsb = -1;
            if (state.addressMsb == 127 && state.addressLsb == 127)
            {
                state.addressMsb = -1;
                state.addressLsb = -1;
            }
            break;

        case 6: // Data Entry MSB
            if (state.addressMsb >= 0 && state.addressLsb >= 0)
                state.dataMsb = value;
            break;

        case 38: // Data Entry LSB; Ion candidate evidence says this byte matters
            if (state.addressMsb >= 0 && state.addressLsb >= 0 && state.dataMsb >= 0)
            {
                const auto parameter = (state.addressMsb << 7) | state.addressLsb;
                const auto value14 = (state.dataMsb << 7) | value;
                return DecodedNrpn { channel, parameter, value14 };
            }
            break;

        default:
            break;
    }

    return std::nullopt;
}

void IonNrpnDecoder::reset() noexcept
{
    channels = {};
}
}
