#pragma once

#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"
#include "Midi/IonSysExCodec.h"

namespace aim
{
/** Bridges the candidate decoded Ion patch image into AIM Editor's portable
    parameter model. Values are kept in raw synth units; presentation/scaling
    remains a separate concern.
*/
class IonProgramDecoder
{
public:
    [[nodiscard]] static juce::Result decode (const IonPatchDump& patch,
                                               const ParameterRegistry& registry,
                                               IonProgram& program);

private:
    [[nodiscard]] static juce::Result readRawValue (const ParameterDefinition& definition,
                                                     const std::vector<std::uint8_t>& bytes,
                                                     juce::var& value);
};
}
