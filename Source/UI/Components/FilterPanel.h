#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
class FilterPanel final : public EditorSection
{
public:
    FilterPanel (const ParameterRegistry& registry, ProgramState& state);

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
    static void layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                juce::Rectangle<int> bounds);

    std::vector<Lane> lanes;
    std::vector<std::unique_ptr<ParameterControl>> routingControls;
};
}
