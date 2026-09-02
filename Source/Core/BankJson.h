#pragma once

#include "Core/ProgramBank.h"

#include <juce_core/juce_core.h>

namespace aim
{
class BankJson
{
public:
    [[nodiscard]] static juce::String encode (const ProgramBank& bank);
    [[nodiscard]] static juce::var toVar (const ProgramBank& bank);

    static juce::Result decode (const juce::String& jsonText, ProgramBank& destination);
    static juce::Result fromVar (const juce::var& value, ProgramBank& destination);
};
}
