#include "ProgramState.h"

#include <algorithm>
#include <cmath>

namespace aim
{
ProgramState::ProgramState (const ParameterRegistry& registryToUse)
    : registry (registryToUse)
{
    resetToRegistryDefaults();
}

const juce::var* ProgramState::valueFor (std::string_view id) const
{
    return currentProgram.getParameter (id);
}

void ProgramState::setName (juce::String name, ProgramChangeOrigin origin)
{
    if (currentProgram.getName() == name)
        return;

    currentProgram.setName (std::move (name));
    for (auto* listener : listeners)
        if (listener != nullptr)
            listener->programMetadataChanged (origin);
}

void ProgramState::setCategory (juce::String category, ProgramChangeOrigin origin)
{
    if (currentProgram.getCategory() == category)
        return;

    currentProgram.setCategory (std::move (category));
    for (auto* listener : listeners)
        if (listener != nullptr)
            listener->programMetadataChanged (origin);
}

juce::Result ProgramState::setValue (std::string_view id,
                                    juce::var value,
                                    ProgramChangeOrigin origin)
{
    const auto* definition = registry.find (id);
    if (definition == nullptr)
        return juce::Result::fail ("Unknown parameter ID: " + juce::String::fromUTF8 (id.data(), static_cast<int> (id.size())));

    auto normalized = normalizedValue (*definition, value);

    // Avoid redundant notifications and live-MIDI traffic when a control is
    // assigned the value it already holds. This is especially important for
    // synchronized duplicate views and incoming controller feedback.
    if (const auto* previous = currentProgram.getParameter (id); previous != nullptr && *previous == normalized)
        return juce::Result::ok();

    currentProgram.setParameter (std::string (id), normalized);

    for (auto* listener : listeners)
        if (listener != nullptr)
            listener->parameterValueChanged (id, normalized, origin);

    return juce::Result::ok();
}

void ProgramState::replaceProgram (const IonProgram& replacement, ProgramChangeOrigin origin)
{
    IonProgram normalized;
    normalized.setName (replacement.getName());
    normalized.setCategory (replacement.getCategory());

    for (const auto& [id, value] : replacement.getParameters())
    {
        // Imported/captured values are evidence. Preserve them exactly even if
        // they fall outside today's candidate enum/range metadata. Validation
        // is applied to interactive setValue() calls, not destructive imports.
        normalized.setParameter (id, value);
    }

    for (const auto& [offset, value] : replacement.getUnknownBytes())
        normalized.preserveUnknownByte (offset, value);

    normalized.setSourcePatchBytes (replacement.getSourcePatchBytes());

    currentProgram = std::move (normalized);

    for (auto* listener : listeners)
        if (listener != nullptr)
            listener->programReplaced (origin);
}

void ProgramState::resetToRegistryDefaults (ProgramChangeOrigin origin)
{
    IonProgram initial;
    initial.setName ("Init");

    for (const auto& definition : registry.all())
        initial.setParameter (definition.id, fallbackValue (definition));

    replaceProgram (initial, origin);
}

void ProgramState::addListener (Listener* listener)
{
    if (listener == nullptr)
        return;

    if (std::find (listeners.begin(), listeners.end(), listener) == listeners.end())
        listeners.push_back (listener);
}

void ProgramState::removeListener (Listener* listener)
{
    listeners.erase (std::remove (listeners.begin(), listeners.end(), listener), listeners.end());
}

juce::var ProgramState::normalizedValue (const ParameterDefinition& definition,
                                         const juce::var& value) const
{
    if (definition.kind == ParameterKind::boolean && definition.enumValues.size() <= 2)
        return juce::var (static_cast<bool> (value));

    if (value.isInt() || value.isInt64() || value.isDouble() || value.isBool())
    {
        double numeric = static_cast<double> (value);

        if (definition.kind == ParameterKind::discrete
            || definition.kind == ParameterKind::enumeration
            || definition.kind == ParameterKind::boolean)
            numeric = std::round (numeric);

        // A complete enum is an exact semantic domain, not merely a numeric
        // range. Validate the submitted raw value before applying min/max
        // clamping so that an invalid value cannot be transformed into a
        // different, valid enum member (for example 999 -> raw_max 20).
        // Incomplete enum tables intentionally skip this membership gate: an
        // unknown but in-range raw value may be real hardware evidence and
        // must be preserved.
        if (! definition.enumValues.empty() && definition.enumValuesComplete)
        {
            const auto exact = std::find_if (definition.enumValues.begin(), definition.enumValues.end(),
                                             [numeric] (const ParameterEnumValue& item)
                                             {
                                                 return item.raw == static_cast<int> (numeric);
                                             });
            if (exact == definition.enumValues.end())
                return fallbackValue (definition);
        }

        if (definition.rawMin)
            numeric = std::max (numeric, *definition.rawMin);
        if (definition.rawMax)
            numeric = std::min (numeric, *definition.rawMax);

        if (definition.kind == ParameterKind::continuous)
            return juce::var (numeric);

        return juce::var (static_cast<int> (numeric));
    }

    return fallbackValue (definition);
}

juce::var ProgramState::fallbackValue (const ParameterDefinition& definition) const
{
    // Complete enum tables have a finite, known domain. Prefer a declared
    // default only when it is itself a member of that domain; otherwise use
    // the first declared member as the stable fallback. This avoids recursive
    // normalization if future metadata accidentally contains an invalid
    // default_raw.
    if (! definition.enumValues.empty() && definition.enumValuesComplete)
    {
        if (definition.defaultRaw)
        {
            const auto roundedDefault = static_cast<int> (std::round (*definition.defaultRaw));
            const auto exact = std::find_if (definition.enumValues.begin(), definition.enumValues.end(),
                                             [roundedDefault] (const ParameterEnumValue& item)
                                             {
                                                 return item.raw == roundedDefault;
                                             });
            if (exact != definition.enumValues.end())
                return juce::var (exact->raw);
        }

        return juce::var (definition.enumValues.front().raw);
    }

    if (definition.defaultRaw)
    {
        double numeric = *definition.defaultRaw;

        if (definition.kind == ParameterKind::discrete
            || definition.kind == ParameterKind::enumeration
            || definition.kind == ParameterKind::boolean)
            numeric = std::round (numeric);

        if (definition.rawMin)
            numeric = std::max (numeric, *definition.rawMin);
        if (definition.rawMax)
            numeric = std::min (numeric, *definition.rawMax);

        if (definition.kind == ParameterKind::continuous)
            return juce::var (numeric);

        return juce::var (static_cast<int> (numeric));
    }

    if (! definition.enumValues.empty())
        return juce::var (definition.enumValues.front().raw);

    if (definition.kind == ParameterKind::boolean)
        return juce::var (false);

    if (definition.rawMin && definition.rawMax)
    {
        if (*definition.rawMin <= 0.0 && *definition.rawMax >= 0.0)
            return definition.kind == ParameterKind::continuous ? juce::var (0.0) : juce::var (0);

        return definition.kind == ParameterKind::continuous
                 ? juce::var (*definition.rawMin)
                 : juce::var (static_cast<int> (std::round (*definition.rawMin)));
    }

    return definition.kind == ParameterKind::continuous ? juce::var (0.0) : juce::var (0);
}
}
