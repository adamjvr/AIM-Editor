#include "ParameterFormatter.h"

#include <algorithm>
#include <cmath>

namespace aim
{
namespace
{
juce::String formatNumber (double value, int decimals = 2)
{
    if (std::abs (value - std::round (value)) < 0.00001)
        return juce::String (static_cast<int> (std::round (value)));
    return juce::String (value, decimals).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
}

juce::String formatEngineering (double value, const juce::String& unit)
{
    const auto magnitude = std::abs (value);
    if (unit == "Hz" && magnitude >= 1000.0)
        return formatNumber (value / 1000.0, 2) + " kHz";
    if (unit == "ms" && magnitude >= 1000.0)
        return formatNumber (value / 1000.0, 2) + " s";
    return formatNumber (value, 2) + (unit.isNotEmpty() ? " " + unit : juce::String{});
}
}

juce::String ParameterFormatter::format (const ParameterDefinition& definition,
                                          const juce::var& rawValue)
{
    if (! definition.enumValues.empty())
    {
        const auto raw = static_cast<int> (rawValue);
        const auto found = std::find_if (definition.enumValues.begin(), definition.enumValues.end(),
                                         [raw] (const ParameterEnumValue& item) { return item.raw == raw; });
        if (found != definition.enumValues.end())
            return found->name.isNotEmpty() ? found->name : found->id.replaceCharacter ('_', ' ');
        return "Unknown " + juce::String (raw);
    }

    if (definition.kind == ParameterKind::boolean)
        return static_cast<bool> (rawValue) ? "On" : "Off";

    const auto raw = static_cast<double> (rawValue);
    const auto transform = definition.displayTransform.trim();

    if (transform.isEmpty() || transform == "unknown")
        return "raw " + formatNumber (raw, 3);

    if (transform == "raw" || transform == "enum")
        return formatEngineering (raw, definition.unit);

    if (transform == "raw * 0.1")
        return formatEngineering (raw * 0.1, definition.unit);

    if (transform == "raw * 0.01")
        return formatEngineering (raw * 0.01, definition.unit);

    if (transform == "raw-3")
        return formatEngineering (raw - 3.0, definition.unit);

    if (transform == "raw-7")
        return formatEngineering (raw - 7.0, definition.unit);

    if (transform == "exp(x/23.177415)/2 ms")
        return formatEngineering (std::exp (raw / 23.177415) / 2.0, "ms");

    if (transform == "x==1023 ? 1000Hz : exp(x/88.85677)/100Hz")
        return formatEngineering (static_cast<int> (std::round (raw)) == 1023
                                    ? 1000.0
                                    : std::exp (raw / 88.85677) / 100.0,
                                  "Hz");

    if (transform == "x==1023 ? 20000Hz : exp(x/147.933647)*20Hz")
        return formatEngineering (static_cast<int> (std::round (raw)) == 1023
                                    ? 20000.0
                                    : std::exp (raw / 147.933647) * 20.0,
                                  "Hz");

    if (transform == "0..254: exp(x/25.5188668) ms; 255:30000ms; 256:held")
    {
        const auto rounded = static_cast<int> (std::round (raw));
        if (rounded >= 256)
            return "Held";
        if (rounded == 255)
            return "30 s";
        return formatEngineering (std::exp (raw / 25.5188668), "ms");
    }

    if (transform == "x==127 ? 10000ms : exp(x/18.38514)*10ms")
        return formatEngineering (static_cast<int> (std::round (raw)) == 127
                                    ? 10000.0
                                    : std::exp (raw / 18.38514) * 10.0,
                                  "ms");

    // The transform is documented but not executable by this version. Keep the
    // raw value visible instead of attaching a potentially false unit.
    return "raw " + formatNumber (raw, 3);
}
}
