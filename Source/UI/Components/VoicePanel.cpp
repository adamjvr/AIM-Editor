#include "VoicePanel.h"

namespace aim
{
namespace
{
void drawChrome (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (juce::Colour::fromRGB (205, 205, 203));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds, 10.0f, 1.25f);

    auto header = bounds.withHeight (27.0f);
    g.setColour (juce::Colour::fromRGB (50, 50, 50));
    g.fillRoundedRectangle (header, 9.0f);
    g.fillRect (header.withTop (header.getCentreY()));
    g.setColour (juce::Colour::fromRGB (225, 30, 30));
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("VOICE", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

VoicePanel::VoicePanel (const ParameterRegistry& registry, ProgramState& state)
{
    addGroup ("UNISON", { "voice.unison", "voice.detune", "voice.drift" }, registry, state);
    addGroup ("PORTAMENTO", { "voice.portamento_enable", "voice.portamento_time",
                               "voice.portamento_type", "voice.portamento_trigger" }, registry, state);
    addGroup ("PLAY MODE", { "voice.poly_mode", "voice.pitch_wheel_mode" }, registry, state);
}

void VoicePanel::addGroup (juce::String title,
                           std::initializer_list<const char*> ids,
                           const ParameterRegistry& registry,
                           ProgramState& state)
{
    Group group;
    group.title = std::move (title);
    for (const auto* id : ids)
    {
        if (const auto* definition = registry.find (id))
        {
            auto control = std::make_unique<ParameterControl> (*definition, state);
            addAndMakeVisible (*control);
            group.controls.push_back (std::move (control));
        }
    }
    groups.push_back (std::move (group));
}

void VoicePanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f));
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    const auto rowHeight = area.getHeight() / juce::jmax (1, static_cast<int> (groups.size()));
    for (std::size_t i = 0; i < groups.size(); ++i)
    {
        auto row = juce::Rectangle<int> (area.getX(), area.getY() + static_cast<int> (i) * rowHeight,
                                         area.getWidth(), rowHeight);
        g.setColour ((i & 1u) == 0u ? juce::Colours::white.withAlpha (0.11f)
                                    : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (row.toFloat().reduced (0.0f, 2.0f), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.62f));
        g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
        g.drawText (groups[i].title, row.removeFromLeft (54).reduced (2, 0), juce::Justification::centred);
    }
}

void VoicePanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto rowHeight = area.getHeight() / juce::jmax (1, static_cast<int> (groups.size()));

    for (std::size_t i = 0; i < groups.size(); ++i)
    {
        auto row = juce::Rectangle<int> (area.getX(), area.getY() + static_cast<int> (i) * rowHeight,
                                         area.getWidth(), rowHeight).withTrimmedLeft (56);
        layoutGroup (groups[i], row);
    }
}

void VoicePanel::layoutGroup (Group& group, juce::Rectangle<int> bounds)
{
    if (group.controls.empty())
        return;
    const auto cellWidth = juce::jmax (58, bounds.getWidth() / static_cast<int> (group.controls.size()));
    for (auto& control : group.controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int VoicePanel::preferredHeightForWidth (int width) const
{
    if (width < 230)
        return 390;
    if (width < 340)
        return 330;
    return 292;
}
}
