#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramJson.h"

#include <BinaryData.h>
#include <juce_core/juce_core.h>

#include <iostream>

namespace
{
int fail (const juce::String& message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    aim::ParameterRegistry registry;
    const auto parameterJson = juce::String::fromUTF8 (AIMBinaryData::parameters_json,
                                                        AIMBinaryData::parameters_jsonSize);
    if (const auto result = registry.loadFromJson (parameterJson); result.failed())
        return fail (result.getErrorMessage());

    if (registry.size() != 178)
        return fail ("Expected 178 initial parameter definitions, got " + juce::String (registry.size()));

    if (registry.find ("filter1.frequency") == nullptr)
        return fail ("filter1.frequency missing from registry");

    if (registry.find ("mod_matrix.slot12.destination") == nullptr)
        return fail ("mod_matrix.slot12.destination missing from registry");

    aim::IonProgram source;
    source.setName ("Round Trip");
    source.setCategory ("Test");
    source.setParameter ("filter1.frequency", 101);
    source.setParameter ("voice.unison", 2);
    source.setParameter ("output.bypass", false);
    source.preserveUnknownByte (206, 37);

    const auto encoded = aim::ProgramJson::encode (source);

    aim::IonProgram decoded;
    if (const auto result = aim::ProgramJson::decode (encoded, decoded); result.failed())
        return fail (result.getErrorMessage());

    if (decoded.getName() != source.getName() || decoded.getCategory() != source.getCategory())
        return fail ("Program metadata did not round trip");

    const auto* frequency = decoded.getParameter ("filter1.frequency");
    if (frequency == nullptr || static_cast<int> (*frequency) != 101)
        return fail ("filter1.frequency did not round trip");

    const auto unknown = decoded.getUnknownBytes().find (206);
    if (unknown == decoded.getUnknownBytes().end() || unknown->second != 37)
        return fail ("Unknown byte preservation failed");

    std::cout << "PASS: AIM Editor core tests\n";
    std::cout << "Loaded " << registry.size() << " JSON parameter definitions\n";
    return 0;
}
