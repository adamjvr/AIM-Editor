#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** Purpose-built 12-slot Ion modulation matrix.

    The generic JSON control factory remains useful everywhere else, but the
    matrix is fundamentally tabular: source -> level/offset -> destination.
    This component preserves that mental model and remains bound only to
    ProgramState, never directly to MIDI.
*/
class ModMatrixPanel final : public EditorSection
{
public:
    ModMatrixPanel (const ParameterRegistry& registry, ProgramState& state);
    ~ModMatrixPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    class Row;
    std::vector<std::unique_ptr<Row>> rows;
};
}
