#include "IonParameterTransmitter.h"

#include "Midi/IonProtocol.h"

#include <algorithm>
#include <cmath>

namespace aim
{
IonParameterTransmitter::IonParameterTransmitter (ProgramState& stateToUse,
                                                  const ParameterRegistry& registryToUse,
                                                  IonMidiService& midiToUse)
    : state (stateToUse), registry (registryToUse), midi (midiToUse)
{
    state.addListener (this);
}

IonParameterTransmitter::~IonParameterTransmitter()
{
    state.removeListener (this);
}

void IonParameterTransmitter::setMidiChannel (int channel) noexcept
{
    midiChannel = std::clamp (channel, 1, 16);
}

std::optional<std::array<juce::MidiMessage, 4>>
IonParameterTransmitter::messagesForParameter (const ParameterDefinition& definition,
                                               const juce::var& semanticValue,
                                               int channel)
{
    if (! definition.nrpn || definition.mappingStatus == MappingStatus::unmapped)
        return std::nullopt;

    if (! (semanticValue.isInt() || semanticValue.isInt64()
           || semanticValue.isDouble() || semanticValue.isBool()))
        return std::nullopt;

    auto numeric = static_cast<double> (semanticValue);
    if (definition.nrpnMin)
        numeric = std::max (numeric, *definition.nrpnMin);
    if (definition.nrpnMax)
        numeric = std::min (numeric, *definition.nrpnMax);

    const auto raw = static_cast<int> (std::llround (numeric));

    if (definition.nrpnValueEncoding == "signed_14_wrap")
        return IonProtocol::makeIonNrpnSequence (channel, *definition.nrpn, raw);

    if (definition.nrpnValueEncoding == "unsigned_14")
        return IonProtocol::makeNrpnSequence (channel, *definition.nrpn, std::clamp (raw, 0, 16383));

    return std::nullopt;
}

void IonParameterTransmitter::parameterValueChanged (std::string_view id,
                                                      const juce::var& value,
                                                      ProgramChangeOrigin origin)
{
    if (! enabled || origin != ProgramChangeOrigin::interactive)
        return;

    const auto* definition = registry.find (id);
    if (definition == nullptr)
        return;

    const auto messages = messagesForParameter (*definition, value, midiChannel);
    if (! messages)
        return;

    for (const auto& message : *messages)
        midi.sendNow (message);
}
}
