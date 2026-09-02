#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <initializer_list>
#include <memory>
#include <vector>

namespace aim
{
class EffectsPanel final : public EditorSection
{
public:
    EffectsPanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    void addControls (std::vector<std::unique_ptr<ParameterControl>>& destination,
                      std::initializer_list<const char*> ids,
                      const ParameterRegistry& registry,
                      ProgramState& state);
    static void layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                juce::Rectangle<int> bounds);

    std::vector<std::unique_ptr<ParameterControl>> toneControls;
    std::vector<std::unique_ptr<ParameterControl>> modulationControls;
    std::vector<std::unique_ptr<ParameterControl>> modeControls;
};
}
