#include "RandomizerEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace aim
{
namespace
{
bool isIntegral (double value)
{
    return std::abs (value - std::round (value)) < 1.0e-9;
}

bool sectionEnabled (const ParameterDefinition& definition, const RandomizerSettings& settings)
{
    const auto& section = definition.section;

    if (section.startsWith ("osc") || section == "pre_filter_mix")
        return settings.oscillators;

    if (section.startsWith ("filter") || section == "filters" || section == "post_filter_mix")
        return settings.filters;

    if (section.startsWith ("env_"))
        return settings.envelopes;

    if (section.startsWith ("lfo") || section == "sample_hold" || section == "tempo_arp"
        || section == "mod_matrix" || section == "tracking_generator")
        return settings.modulation;

    if (section == "effects")
        return settings.effects;

    if (section == "voice" || section == "output")
        return settings.voiceAndOutput;

    return false;
}

bool kindEnabled (const ParameterDefinition& definition, const RandomizerSettings& settings)
{
    switch (definition.kind)
    {
        case ParameterKind::continuous:
        case ParameterKind::discrete:    return settings.continuousValues;
        case ParameterKind::enumeration: return settings.enumerations;
        case ParameterKind::boolean:     return settings.switches;
    }

    return false;
}

std::uint64_t makeSeed (std::uint64_t requested)
{
    if (requested != 0)
        return requested;

    std::random_device device;
    const auto hi = static_cast<std::uint64_t> (device());
    const auto lo = static_cast<std::uint64_t> (device());
    auto seed = (hi << 32u) ^ lo;
    return seed == 0 ? 0x41494d454449544full : seed; // "AIMEDITO"-ish fallback
}

bool numericValue (const juce::var& value, double& result)
{
    if (! (value.isInt() || value.isInt64() || value.isDouble() || value.isBool()))
        return false;

    result = static_cast<double> (value);
    return true;
}

juce::var randomTarget (const ParameterDefinition& definition, std::mt19937_64& generator)
{
    if (! definition.enumValues.empty())
    {
        std::uniform_int_distribution<std::size_t> choice (0, definition.enumValues.size() - 1);
        return definition.enumValues[choice (generator)].raw;
    }

    if (definition.kind == ParameterKind::boolean)
    {
        std::bernoulli_distribution bit (0.5);
        return bit (generator);
    }

    if (! definition.rawMin || ! definition.rawMax)
        return {};

    const auto lo = *definition.rawMin;
    const auto hi = *definition.rawMax;

    if (isIntegral (lo) && isIntegral (hi))
    {
        std::uniform_int_distribution<long long> distribution (static_cast<long long> (std::llround (lo)),
                                                                static_cast<long long> (std::llround (hi)));
        return static_cast<int> (distribution (generator));
    }

    std::uniform_real_distribution<double> distribution (lo, hi);
    return distribution (generator);
}
}

RandomizerResult RandomizerEngine::randomize (const IonProgram& source,
                                               const ParameterRegistry& registry,
                                               const RandomizerSettings& requested)
{
    RandomizerResult result;
    result.program = source;
    result.seedUsed = makeSeed (requested.seed);

    auto settings = requested;
    settings.strength = std::clamp (settings.strength, 0.0, 1.0);

    if (settings.strength <= 0.0)
        return result;

    std::mt19937_64 generator (result.seedUsed);
    std::bernoulli_distribution replaceDiscrete (settings.strength);

    for (const auto& definition : registry.all())
    {
        if (! sectionEnabled (definition, settings) || ! kindEnabled (definition, settings))
            continue;

        // Editor-only/unmapped controls are intentionally excluded. The
        // randomizer is meant to generate portable Ion patch state, not mutate
        // UI bookkeeping such as a selected curve point.
        if (definition.mappingStatus == MappingStatus::unmapped)
            continue;

        const auto* current = source.getParameter (definition.id);
        if (current == nullptr)
            continue;

        auto target = randomTarget (definition, generator);
        if (target.isVoid())
            continue;

        juce::var next = target;
        const auto categorical = definition.kind == ParameterKind::enumeration
                              || definition.kind == ParameterKind::boolean
                              || ! definition.enumValues.empty();

        if (categorical)
        {
            if (! replaceDiscrete (generator))
                continue;
        }
        else
        {
            double from = 0.0;
            double to = 0.0;
            if (! numericValue (*current, from) || ! numericValue (target, to))
                continue;

            auto blended = from + (to - from) * settings.strength;
            if (definition.rawMin)
                blended = std::max (blended, *definition.rawMin);
            if (definition.rawMax)
                blended = std::min (blended, *definition.rawMax);

            if (definition.kind == ParameterKind::discrete
                || (definition.rawMin && definition.rawMax
                    && isIntegral (*definition.rawMin) && isIntegral (*definition.rawMax)))
                next = static_cast<int> (std::llround (blended));
            else
                next = blended;
        }

        if (next == *current)
            continue;

        result.program.setParameter (definition.id, next);
        result.changedParameterIds.push_back (definition.id);
    }

    return result;
}
}
