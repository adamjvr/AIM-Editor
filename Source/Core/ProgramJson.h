#pragma once

#include "IonProgram.h"

namespace aim
{
class ProgramJson
{
public:
    static juce::String encode (const IonProgram& program);
    static juce::Result decode (const juce::String& jsonText, IonProgram& destination);

private:
    static void insertNestedParameter (juce::DynamicObject& root, const std::string& dottedId, const juce::var& value);
    static void flattenParameterObject (const juce::var& value, const juce::String& prefix, IonProgram& destination);
};
}
