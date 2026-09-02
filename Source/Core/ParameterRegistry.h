#pragma once

#include "ParameterDefinition.h"

#include <map>
#include <string>
#include <vector>

namespace aim
{
class ParameterRegistry
{
public:
    juce::Result loadFromJson (const juce::String& jsonText);
    juce::Result loadEnumTableFromJson (const juce::String& jsonText);

    [[nodiscard]] const ParameterDefinition* find (std::string_view id) const;
    [[nodiscard]] const ParameterDefinition* findByNrpn (int nrpn) const;
    [[nodiscard]] std::vector<const ParameterDefinition*> parametersForPage (const juce::String& page) const;
    [[nodiscard]] std::vector<const ParameterDefinition*> parametersForSection (const juce::String& section) const;
    [[nodiscard]] const std::vector<ParameterDefinition>& all() const noexcept { return definitions; }
    [[nodiscard]] int size() const noexcept { return static_cast<int> (definitions.size()); }

private:
    std::vector<ParameterDefinition> definitions;
    std::map<std::string, std::size_t, std::less<>> indexById;
    std::map<juce::String, std::vector<ParameterEnumValue>> enumTables;
};
}
