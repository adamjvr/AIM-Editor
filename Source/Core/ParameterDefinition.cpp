#include "ParameterDefinition.h"

namespace aim
{
ParameterKind parameterKindFromString (const juce::String& text)
{
    if (text == "discrete") return ParameterKind::discrete;
    if (text == "enum") return ParameterKind::enumeration;
    if (text == "boolean") return ParameterKind::boolean;
    return ParameterKind::continuous;
}

juce::String parameterKindToString (ParameterKind kind)
{
    switch (kind)
    {
        case ParameterKind::continuous: return "continuous";
        case ParameterKind::discrete: return "discrete";
        case ParameterKind::enumeration: return "enum";
        case ParameterKind::boolean: return "boolean";
    }

    return "continuous";
}

MappingStatus mappingStatusFromString (const juce::String& text)
{
    if (text == "verified") return MappingStatus::verified;
    if (text == "candidate") return MappingStatus::candidate;
    return MappingStatus::unmapped;
}

juce::String mappingStatusToString (MappingStatus status)
{
    switch (status)
    {
        case MappingStatus::unmapped: return "unmapped";
        case MappingStatus::candidate: return "candidate";
        case MappingStatus::verified: return "verified";
    }

    return "unmapped";
}
}
