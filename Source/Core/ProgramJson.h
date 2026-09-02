#pragma once

#include "Core/IonProgram.h"

#include <juce_core/juce_core.h>

namespace aim
{
class ProgramJson
{
public:
    [[nodiscard]] static juce::String encode (const IonProgram& program);
    [[nodiscard]] static juce::var toVar (const IonProgram& program);

    static juce::Result decode (const juce::String& jsonText, IonProgram& destination);
    static juce::Result fromVar (const juce::var& value, IonProgram& destination);

private:
    static void insertNestedParameter (juce::DynamicObject& root,
                                       const std::string& dottedId,
                                       const juce::var& value);
    static void flattenParameterObject (const juce::var& value,
                                        const juce::String& prefix,
                                        IonProgram& destination);
};
}
