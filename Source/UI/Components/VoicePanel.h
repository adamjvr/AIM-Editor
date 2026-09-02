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
class VoicePanel final : public EditorSection
{
public:
    VoicePanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    struct Group
    {
        juce::String title;
        std::vector<std::unique_ptr<ParameterControl>> controls;
    };

    void addGroup (juce::String title,
                   std::initializer_list<const char*> ids,
                   const ParameterRegistry& registry,
                   ProgramState& state);
    static void layoutGroup (Group& group, juce::Rectangle<int> bounds);

    std::vector<Group> groups;
};
}
