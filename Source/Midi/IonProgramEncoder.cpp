#include "IonProgramEncoder.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace aim
{
namespace
{
constexpr std::array<const char*, 11> categories {
    "Recent", "Faves", "Bass", "Lead", "Pad", "String", "Brass", "Key", "Comp", "Drum", "SFX"
};

void writeName (const juce::String& name, std::vector<std::uint8_t>& bytes)
{
    constexpr std::size_t offset = 63;
    constexpr std::size_t storage = 15;
    constexpr std::size_t maxCharacters = 14;

    std::fill_n (bytes.begin() + static_cast<std::ptrdiff_t> (offset), storage, std::uint8_t { 0 });
    const auto ascii = name.toRawUTF8();
    const auto length = std::min<std::size_t> (maxCharacters, static_cast<std::size_t> (name.getNumBytesAsUTF8()));
    for (std::size_t i = 0; i < length; ++i)
    {
        const auto byte = static_cast<unsigned char> (ascii[i]);
        bytes[offset + i] = byte >= 0x20 && byte <= 0x7e ? static_cast<std::uint8_t> (byte)
                                                          : static_cast<std::uint8_t> ('?');
    }
}
}

juce::Result IonProgramEncoder::encodeOntoTemplate (const IonProgram& program,
                                                     const ParameterRegistry& registry,
                                                     const IonPatchDump& sourcePatch,
                                                     juce::MidiMessage& message)
{
    if (sourcePatch.decodedBytes.size() != IonSysExCodec::decodedSinglePatchSize)
        return juce::Result::fail ("A 378-byte source patch template is required for safe encoding");

    auto bytes = sourcePatch.decodedBytes;
    writeName (program.getName(), bytes);

    for (std::size_t i = 0; i < categories.size(); ++i)
    {
        if (program.getCategory().equalsIgnoreCase (categories[i]))
        {
            bytes[86] = static_cast<std::uint8_t> (i);
            break;
        }
    }

    for (const auto& [id, value] : program.getParameters())
    {
        const auto* definition = registry.find (id);
        if (definition == nullptr || definition->mappingStatus == MappingStatus::unmapped
            || ! definition->sysex.offset.has_value())
            continue;

        if (const auto result = writeRawValue (*definition, value, bytes); result.failed())
            return juce::Result::fail (juce::String (id) + ": " + result.getErrorMessage());
    }

    return IonSysExCodec::encodeSinglePatchDump (bytes, message);
}

juce::Result IonProgramEncoder::writeRawValue (const ParameterDefinition& definition,
                                                const juce::var& value,
                                                std::vector<std::uint8_t>& bytes)
{
    if (! (value.isInt() || value.isInt64() || value.isDouble() || value.isBool()))
        return juce::Result::fail ("Mapped SysEx value is not numeric");

    const auto offset = static_cast<std::size_t> (*definition.sysex.offset);
    if (offset >= bytes.size())
        return juce::Result::fail ("SysEx offset is outside patch image");

    const auto raw = static_cast<long long> (static_cast<double> (value));
    const auto& encoding = definition.sysex.encoding;

    if (encoding == "bitfield")
    {
        if (! definition.sysex.mask || ! definition.sysex.shift)
            return juce::Result::fail ("Bitfield mapping lacks mask/shift");

        const auto mask = static_cast<std::uint8_t> (*definition.sysex.mask);
        const auto shift = static_cast<unsigned int> (*definition.sysex.shift);
        const auto shifted = static_cast<unsigned long long> (raw) << shift;
        if ((shifted & ~static_cast<unsigned long long> (mask)) != 0u)
            return juce::Result::fail ("Value does not fit candidate bitfield mask");

        bytes[offset] = static_cast<std::uint8_t> ((bytes[offset] & static_cast<std::uint8_t> (~mask))
                                                   | (static_cast<std::uint8_t> (shifted) & mask));
        return juce::Result::ok();
    }

    if (encoding == "u8")
    {
        if (raw < 0 || raw > 255)
            return juce::Result::fail ("u8 value is outside 0..255");
        bytes[offset] = static_cast<std::uint8_t> (raw);
        return juce::Result::ok();
    }

    if (encoding == "s8")
    {
        if (raw < -128 || raw > 127)
            return juce::Result::fail ("s8 value is outside -128..127");
        bytes[offset] = static_cast<std::uint8_t> (static_cast<std::int8_t> (raw));
        return juce::Result::ok();
    }

    if (encoding == "s16be")
    {
        if (offset + 1 >= bytes.size())
            return juce::Result::fail ("s16be mapping extends past patch image");
        if (raw < -32768 || raw > 32767)
            return juce::Result::fail ("s16be value is outside -32768..32767");

        const auto encoded = static_cast<std::uint16_t> (static_cast<std::int16_t> (raw));
        bytes[offset] = static_cast<std::uint8_t> ((encoded >> 8u) & 0xffu);
        bytes[offset + 1] = static_cast<std::uint8_t> (encoded & 0xffu);
        return juce::Result::ok();
    }

    return juce::Result::fail ("Unsupported candidate SysEx encoding: " + encoding);
}
}
