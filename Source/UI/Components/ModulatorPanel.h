#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** Purpose-built LFO / sample-and-hold / arpeggiator timing block.

    The original editor groups these modulation generators by signal role.
    Keeping that grouping explicit makes the desktop and iPad surfaces much
    faster to scan than a generic parameter matrix while all values still bind
    to the shared JSON-backed ProgramState.
*/
class ModulatorPanel final : public EditorSection
{
public:
    ModulatorPanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    struct Lane
    {
        juce::String title;
        juce::String kind;
        std::vector<std::unique_ptr<ParameterControl>> controls;
    };

    void addLane (juce::String title,
                  juce::String kind,
                  const std::vector<const ParameterDefinition*>& definitions,
                  ProgramState& state);
    static void drawWaveGlyph (juce::Graphics& g,
                               juce::Rectangle<float> bounds,
                               const juce::String& kind);
    static void layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                juce::Rectangle<int> bounds);

    std::vector<Lane> lanes;
    std::vector<std::unique_ptr<ParameterControl>> arpControls;
};
}
