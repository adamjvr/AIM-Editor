#include "ProgramBank.h"

namespace aim
{
juce::Result ProgramBank::setProgram (int slot, const IonProgram& program)
{
    if (! isValidSlot (slot))
        return juce::Result::fail ("Program bank slot must be in the range 0..127");

    programs[static_cast<std::size_t> (slot)] = program;
    return juce::Result::ok();
}

juce::Result ProgramBank::clearProgram (int slot)
{
    if (! isValidSlot (slot))
        return juce::Result::fail ("Program bank slot must be in the range 0..127");

    programs[static_cast<std::size_t> (slot)].reset();
    return juce::Result::ok();
}

const IonProgram* ProgramBank::programAt (int slot) const noexcept
{
    if (! isValidSlot (slot))
        return nullptr;

    const auto& item = programs[static_cast<std::size_t> (slot)];
    return item ? &*item : nullptr;
}

IonProgram* ProgramBank::programAt (int slot) noexcept
{
    if (! isValidSlot (slot))
        return nullptr;

    auto& item = programs[static_cast<std::size_t> (slot)];
    return item ? &*item : nullptr;
}

int ProgramBank::occupiedCount() const noexcept
{
    int count = 0;
    for (const auto& program : programs)
        if (program)
            ++count;
    return count;
}

std::vector<int> ProgramBank::occupiedSlots() const
{
    std::vector<int> result;
    result.reserve (static_cast<std::size_t> (occupiedCount()));

    for (int slot = 0; slot < slotCount; ++slot)
        if (programs[static_cast<std::size_t> (slot)])
            result.push_back (slot);

    return result;
}

void ProgramBank::clear()
{
    for (auto& item : programs)
        item.reset();

    name = "Untitled Bank";
    hardwareBank.clear();
}
}
