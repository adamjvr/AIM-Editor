#include "IonProgramDecoder.h"

#include <cstdint>
#include <iterator>

namespace aim
{
namespace
{
int signed8 (std::uint8_t value)
{
    return static_cast<int> (static_cast<std::int8_t> (value));
}

int signed16BE (const std::uint8_t* p)
{
    const auto raw = static_cast<std::uint16_t> ((static_cast<std::uint16_t> (p[0]) << 8u) | p[1]);
    return static_cast<int> (static_cast<std::int16_t> (raw));
}
}

juce::Result IonProgramDecoder::decode (const IonPatchDump& patch,
                                        const ParameterRegistry& registry,
                                        IonProgram& program)
{
    if (patch.decodedBytes.size() != IonSysExCodec::decodedSinglePatchSize)
        return juce::Result::fail ("Ion patch image must contain 378 decoded bytes");

    program.clear();
    program.setName (patch.name);

    static constexpr const char* categories[] = {
        "Recent", "Faves", "Bass", "Lead", "Pad", "String", "Brass", "Key", "Comp", "Drum", "SFX"
    };

    const auto categoryRaw = signed8 (patch.decodedBytes[86]);
    if (categoryRaw >= 0 && categoryRaw < static_cast<int> (std::size (categories)))
        program.setCategory (categories[categoryRaw]);

    for (const auto& definition : registry.all())
    {
        if (definition.mappingStatus == MappingStatus::unmapped || ! definition.sysex.offset.has_value())
            continue;

        juce::var value;
        if (const auto result = readRawValue (definition, patch.decodedBytes, value); result.failed())
            return juce::Result::fail (juce::String (definition.id) + ": " + result.getErrorMessage());

        program.setParameter (definition.id, std::move (value));
    }

    return juce::Result::ok();
}

juce::Result IonProgramDecoder::readRawValue (const ParameterDefinition& definition,
                                               const std::vector<std::uint8_t>& bytes,
                                               juce::var& value)
{
    const auto offset = static_cast<std::size_t> (*definition.sysex.offset);
    if (offset >= bytes.size())
        return juce::Result::fail ("SysEx offset is outside patch image");

    const auto& encoding = definition.sysex.encoding;

    if (encoding == "bitfield")
    {
        if (! definition.sysex.mask.has_value() || ! definition.sysex.shift.has_value())
            return juce::Result::fail ("Bitfield mapping lacks mask/shift");

        value = static_cast<int> ((bytes[offset] & static_cast<std::uint8_t> (*definition.sysex.mask))
                                  >> static_cast<unsigned int> (*definition.sysex.shift));
        return juce::Result::ok();
    }

    if (encoding == "u8")
    {
        value = static_cast<int> (bytes[offset]);
        return juce::Result::ok();
    }

    if (encoding == "s8")
    {
        value = signed8 (bytes[offset]);
        return juce::Result::ok();
    }

    if (encoding == "s16be")
    {
        if (offset + 1 >= bytes.size())
            return juce::Result::fail ("16-bit mapping extends past patch image");
        value = signed16BE (bytes.data() + offset);
        return juce::Result::ok();
    }

    return juce::Result::fail ("Unsupported parameter SysEx encoding: " + encoding);
}
}
