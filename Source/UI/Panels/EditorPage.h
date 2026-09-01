#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
class EditorPage final : public juce::Component
{
public:
    EditorPage (juce::String pageKey,
                juce::String pageTitle,
                const ParameterRegistry& registry,
                ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;

    [[nodiscard]] int preferredHeightForWidth (int width) const;
    [[nodiscard]] const juce::String& getPageKey() const noexcept { return key; }

private:
    static juce::String groupKeyForSection (const juce::String& section);
    static juce::String titleForGroup (const juce::String& group);
    [[nodiscard]] int columnCountForWidth (int width) const;

    juce::String key;
    juce::String title;
    std::vector<std::unique_ptr<SectionPanel>> sections;
};
}
