#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace aim
{
class IonProgram
{
public:
    using ParameterMap = std::map<std::string, juce::var, std::less<>>;
    using UnknownByteMap = std::map<int, std::uint8_t>;
    using RawPatchBytes = std::vector<std::uint8_t>;

    void setName (juce::String newName) { name = std::move (newName); }
    void setCategory (juce::String newCategory) { category = std::move (newCategory); }

    [[nodiscard]] const juce::String& getName() const noexcept { return name; }
    [[nodiscard]] const juce::String& getCategory() const noexcept { return category; }

    void setParameter (std::string id, juce::var value);
    [[nodiscard]] const juce::var* getParameter (std::string_view id) const;
    [[nodiscard]] const ParameterMap& getParameters() const noexcept { return parameters; }

    void preserveUnknownByte (int offset, std::uint8_t value);
    [[nodiscard]] const UnknownByteMap& getUnknownBytes() const noexcept { return unknownBytes; }

    void setSourcePatchBytes (RawPatchBytes bytes) { sourcePatchBytes = std::move (bytes); }
    [[nodiscard]] const RawPatchBytes& getSourcePatchBytes() const noexcept { return sourcePatchBytes; }
    [[nodiscard]] bool hasSourcePatchBytes() const noexcept { return ! sourcePatchBytes.empty(); }

    [[nodiscard]] bool operator== (const IonProgram& other) const
    {
        return name == other.name
            && category == other.category
            && parameters == other.parameters
            && unknownBytes == other.unknownBytes
            && sourcePatchBytes == other.sourcePatchBytes;
    }
    [[nodiscard]] bool operator!= (const IonProgram& other) const { return ! (*this == other); }

    void clear();

private:
    juce::String name;
    juce::String category;
    ParameterMap parameters;
    UnknownByteMap unknownBytes;
    RawPatchBytes sourcePatchBytes;
};
}
