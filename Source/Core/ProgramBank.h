#pragma once

#include "Core/IonProgram.h"

#include <juce_core/juce_core.h>

#include <array>
#include <optional>
#include <vector>

namespace aim
{
/** Human-readable librarian bank independent of transport.

    Slots are zero-based internally and map directly to the Ion's 0..127
    program indices for normal banks. Empty slots are first-class so partial
    research/import banks do not have to invent placeholder programs.
*/
class ProgramBank
{
public:
    static constexpr int slotCount = 128;

    void setName (juce::String newName) { name = std::move (newName); }
    [[nodiscard]] const juce::String& getName() const noexcept { return name; }

    void setHardwareBank (juce::String newHardwareBank) { hardwareBank = std::move (newHardwareBank); }
    [[nodiscard]] const juce::String& getHardwareBank() const noexcept { return hardwareBank; }

    [[nodiscard]] static bool isValidSlot (int slot) noexcept { return slot >= 0 && slot < slotCount; }

    juce::Result setProgram (int slot, const IonProgram& program);
    juce::Result clearProgram (int slot);

    [[nodiscard]] const IonProgram* programAt (int slot) const noexcept;
    [[nodiscard]] IonProgram* programAt (int slot) noexcept;
    [[nodiscard]] bool isOccupied (int slot) const noexcept { return programAt (slot) != nullptr; }
    [[nodiscard]] int occupiedCount() const noexcept;
    [[nodiscard]] std::vector<int> occupiedSlots() const;

    [[nodiscard]] bool operator== (const ProgramBank& other) const
    {
        return name == other.name
            && hardwareBank == other.hardwareBank
            && programs == other.programs;
    }
    [[nodiscard]] bool operator!= (const ProgramBank& other) const { return ! (*this == other); }

    void clear();

private:
    juce::String name { "Untitled Bank" };
    juce::String hardwareBank;
    std::array<std::optional<IonProgram>, slotCount> programs;
};
}
