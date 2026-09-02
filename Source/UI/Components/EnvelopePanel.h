#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** Three touch-editable Ion envelope lanes sharing the canonical ProgramState. */
class EnvelopePanel final : public EditorSection
{
public:
    EnvelopePanel (const ParameterRegistry& registry, ProgramState& state);
    ~EnvelopePanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    class EnvelopeGraph;

    struct Lane
    {
        juce::String title;
        std::unique_ptr<EnvelopeGraph> graph;
        std::vector<std::unique_ptr<ParameterControl>> controls;
    };

    void addLane (juce::String title,
                  juce::String prefix,
                  juce::String section,
                  const ParameterRegistry& registry,
                  ProgramState& state);
    void layoutLane (Lane& lane, juce::Rectangle<int> bounds);

    std::vector<Lane> lanes;
};
}
