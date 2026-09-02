#pragma once

#include "Core/ParameterDefinition.h"

#include <juce_core/juce_core.h>

namespace aim
{
/** Conservative formatter for display transforms explicitly recorded in JSON.

    Unknown transform strings never invent engineering units: they fall back to
    a raw-value representation so reverse-engineering uncertainty remains
    visible to the user.
*/
class ParameterFormatter
{
public:
    [[nodiscard]] static juce::String format (const ParameterDefinition& definition,
                                               const juce::var& rawValue);
};
}
