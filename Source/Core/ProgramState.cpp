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

juce::Result ProgramState::setValue (std::string_view id,
                                    juce::var value,
                                    ProgramChangeOrigin origin)
{
    const auto* definition = registry.find (id);
    if (definition == nullptr)
        return juce::Result::fail ("Unknown parameter ID: " + juce::String::fromUTF8 (id.data(), static_cast<int> (id.size())));

    auto normalized = normalizedValue (*definition, value);
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

    currentProgram = std::move (normalized);

    for (auto* listener : listeners)
        if (listener != nullptr)
            listener->programReplaced (origin);
}

void ProgramState::resetToRegistryDefaults()
{
    IonProgram initial;
    initial.setName ("Init");

    for (const auto& definition : registry.all())
        initial.setParameter (definition.id, fallbackValue (definition));

    replaceProgram (initial, ProgramChangeOrigin::internal);
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

        if (definition.rawMin)
            numeric = std::max (numeric, *definition.rawMin);
        if (definition.rawMax)
            numeric = std::min (numeric, *definition.rawMax);

        if (! definition.enumValues.empty())
        {
            const auto exact = std::find_if (definition.enumValues.begin(), definition.enumValues.end(),
                                             [numeric] (const ParameterEnumValue& item)
                                             {
                                                 return item.raw == static_cast<int> (numeric);
                                             });
            if (exact == definition.enumValues.end())
                numeric = definition.enumValues.front().raw;
        }

        if (definition.kind == ParameterKind::continuous)
            return juce::var (numeric);

        return juce::var (static_cast<int> (numeric));
    }

    return fallbackValue (definition);
}

juce::var ProgramState::fallbackValue (const ParameterDefinition& definition) const
{
    if (definition.defaultRaw)
        return normalizedValue (definition, juce::var (*definition.defaultRaw));

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
