#pragma once

#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace aim
{
struct RandomizerSettings
{
    bool oscillators = true;
    bool filters = true;
    bool envelopes = true;
    bool modulation = true;
    bool effects = true;
    bool voiceAndOutput = false;

    bool continuousValues = true;
    bool enumerations = true;
    bool switches = false;

    /** 0.0 keeps the original value. 1.0 applies the full random target. */
    double strength = 0.35;

    /** A zero seed requests a fresh nondeterministic seed. The seed actually
        used is returned in RandomizerResult so a result can be reproduced. */
    std::uint64_t seed = 0;
};

struct RandomizerResult
{
    IonProgram program;
    std::vector<std::string> changedParameterIds;
    std::uint64_t seedUsed = 0;
};

/** Semantic, protocol-independent patch randomizer.

    The engine only uses domains declared in data/parameters.json. It never
    invents ranges and never touches source-patch bytes or unknown bytes.
    Randomization therefore remains useful during reverse engineering without
    destroying evidence needed for safe SysEx round trips.
*/
class RandomizerEngine
{
public:
    static RandomizerResult randomize (const IonProgram& source,
                                       const ParameterRegistry& registry,
                                       const RandomizerSettings& settings);
};
}
