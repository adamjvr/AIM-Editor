#pragma once

#include "Core/ParameterDefinition.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "Midi/IonMidiService.h"

#include <array>
#include <optional>

namespace aim
{
/** Opt-in live editor -> Ion NRPN bridge.

    Candidate mappings are intentionally disabled by default. The transmitter
    listens to ProgramState but only emits MIDI for ProgramChangeOrigin::interactive,
    preventing patch loads, captures, and protocol input from being echoed back
    to the synth.
*/
class IonParameterTransmitter final : private ProgramState::Listener
{
public:
    IonParameterTransmitter (ProgramState& stateToUse,
                             const ParameterRegistry& registryToUse,
                             IonMidiService& midiToUse);
    ~IonParameterTransmitter() override;

    void setEnabled (bool shouldSend) noexcept { enabled = shouldSend; }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled; }

    void setMidiChannel (int channel) noexcept;
    [[nodiscard]] int getMidiChannel() const noexcept { return midiChannel; }

    /** Builds the four-message candidate NRPN sequence for a definition.
        std::nullopt means the definition is not currently safe/complete enough
        for live candidate transmission.
    */
    static std::optional<std::array<juce::MidiMessage, 4>>
        messagesForParameter (const ParameterDefinition& definition,
                              const juce::var& semanticValue,
                              int midiChannel);

private:
    void parameterValueChanged (std::string_view id,
                                const juce::var& value,
                                ProgramChangeOrigin origin) override;
    void programReplaced (ProgramChangeOrigin) override {}

    ProgramState& state;
    const ParameterRegistry& registry;
    IonMidiService& midi;
    bool enabled = false;
    int midiChannel = 1;
};
}
