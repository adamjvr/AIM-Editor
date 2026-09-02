#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** Hardware-oriented oscillator block.

    The legacy editor presents the three oscillators as parallel rows rather
    than one anonymous bag of controls. This panel preserves that structure
    while still binding every widget to the shared JSON-backed ProgramState.
*/
class OscillatorPanel final : public EditorSection
{
public:
    OscillatorPanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    struct Lane
    {
        juce::String title;
        std::vector<std::unique_ptr<ParameterControl>> controls;
    };

    void addLane (juce::String title,
                  const std::vector<const ParameterDefinition*>& definitions,
                  ProgramState& state);
    void layoutLane (Lane& lane, juce::Rectangle<int> bounds);

    std::vector<Lane> lanes;
    std::vector<std::unique_ptr<ParameterControl>> globalControls;
};
}
