#pragma once

#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"
#include "Midi/IonSysExCodec.h"

namespace aim
{
/** Candidate semantic-program -> Ion patch-image encoder.

    Encoding always starts from an existing decoded patch image. This is a key
    clean-room/research invariant: bytes and bits that AIM Editor does not yet
    understand are preserved rather than synthesized or zeroed.
*/
class IonProgramEncoder
{
public:
    [[nodiscard]] static juce::Result encodeOntoTemplate (const IonProgram& program,
                                                           const ParameterRegistry& registry,
                                                           const IonPatchDump& sourcePatch,
                                                           juce::MidiMessage& message);

private:
    [[nodiscard]] static juce::Result writeRawValue (const ParameterDefinition& definition,
                                                      const juce::var& value,
                                                      std::vector<std::uint8_t>& bytes);
};
}
