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
class MixerPanel final : public EditorSection
{
public:
    enum class Mode { preFilter, postFilter };

    MixerPanel (Mode mode, const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    struct Channel
    {
        juce::String title;
        std::vector<std::unique_ptr<ParameterControl>> controls;
    };

    void addChannel (juce::String title,
                     std::initializer_list<const char*> ids,
                     const ParameterRegistry& registry,
                     ProgramState& state);
    [[nodiscard]] int columnCount (int width) const;
    static void layoutChannel (Channel& channel, juce::Rectangle<int> bounds);

    Mode mode;
    std::vector<Channel> channels;
};
}
