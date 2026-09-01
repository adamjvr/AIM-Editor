#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <string>
#include <vector>

namespace aim
{
enum class ParameterKind
{
    continuous,
    discrete,
    enumeration,
    boolean
};

enum class MappingStatus
{
    unmapped,
    candidate,
    verified
};

struct ParameterEnumValue
{
    int raw = 0;
    juce::String id;
    juce::String name;
};

struct SysExMapping
{
    std::optional<int> offset;
    std::optional<int> bits;
    std::optional<int> widthBytes;
    std::optional<int> mask;
    std::optional<int> shift;
    juce::String encoding;
    juce::String fieldId;
};

struct ParameterDefinition
{
    std::string id;
    juce::String name;
    juce::String section;
    ParameterKind kind = ParameterKind::continuous;

    std::optional<double> rawMin;
    std::optional<double> rawMax;
    std::optional<double> defaultRaw;

    std::vector<ParameterEnumValue> enumValues;

    juce::String unit;
    juce::String displayTransform { "unknown" };

    MappingStatus mappingStatus = MappingStatus::unmapped;
    std::optional<int> nrpn;
    std::optional<double> nrpnMin;
    std::optional<double> nrpnMax;
    juce::String nrpnValueEncoding;
    juce::String nrpnSourceId;
    SysExMapping sysex;

    std::vector<juce::String> pages;
    juce::String control;
};

ParameterKind parameterKindFromString (const juce::String& text);
juce::String parameterKindToString (ParameterKind kind);
MappingStatus mappingStatusFromString (const juce::String& text);
juce::String mappingStatusToString (MappingStatus status);
}
