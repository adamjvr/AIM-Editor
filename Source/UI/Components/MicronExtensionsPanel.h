#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** Micron-only surface layered over the shared Ion-family Program model.

    The controls here are hidden in Ion mode. They expose only fields for which
    we have independent Micron evidence: X/Y/Z assignment, FX1/FX2 balance and
    FX2 algorithm. They remain candidate mappings until verified on physical
    Micron hardware.
*/
class MicronExtensionsPanel final : public EditorSection
{
public:
    MicronExtensionsPanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    void addControl (std::vector<std::unique_ptr<ParameterControl>>& destination,
                     const char* id,
                     const ParameterRegistry& registry,
                     ProgramState& state);
    static void layoutRow (std::vector<std::unique_ptr<ParameterControl>>& controls,
                           juce::Rectangle<int> bounds);

    std::vector<std::unique_ptr<ParameterControl>> xyzControls;
    std::vector<std::unique_ptr<ParameterControl>> fx2Controls;
};
}
