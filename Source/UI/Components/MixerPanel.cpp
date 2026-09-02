#include "MixerPanel.h"

namespace aim
{
namespace
{
void drawChrome (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
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
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText (title, header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

MixerPanel::MixerPanel (Mode modeToUse, const ParameterRegistry& registry, ProgramState& state)
    : mode (modeToUse)
{
    if (mode == Mode::preFilter)
    {
        addChannel ("OSC 1", { "pre_filter_mix.osc1.level", "pre_filter_mix.osc1.balance" }, registry, state);
        addChannel ("OSC 2", { "pre_filter_mix.osc2.level", "pre_filter_mix.osc2.balance" }, registry, state);
        addChannel ("OSC 3", { "pre_filter_mix.osc3.level", "pre_filter_mix.osc3.balance" }, registry, state);
        addChannel ("RING", { "pre_filter_mix.ring.level", "pre_filter_mix.ring.balance" }, registry, state);
        addChannel ("EXT", { "pre_filter_mix.external.level", "pre_filter_mix.external.balance" }, registry, state);
        addChannel ("NOISE", { "pre_filter_mix.noise.level", "pre_filter_mix.noise.balance" }, registry, state);
    }
    else
    {
        addChannel ("FILTER 1", { "post_filter_mix.filter1.level", "post_filter_mix.filter1.pan" }, registry, state);
        addChannel ("FILTER 2", { "post_filter_mix.filter2.level", "post_filter_mix.filter2.pan" }, registry, state);
        addChannel ("PRE", { "post_filter_mix.pre_filter.level", "post_filter_mix.pre_filter.pan" }, registry, state);
    }
}

void MixerPanel::addChannel (juce::String title,
                             std::initializer_list<const char*> ids,
                             const ParameterRegistry& registry,
                             ProgramState& state)
{
    Channel channel;
    channel.title = std::move (title);
    for (const auto* id : ids)
    {
        if (const auto* definition = registry.find (id))
        {
            auto control = std::make_unique<ParameterControl> (*definition, state);
            addAndMakeVisible (*control);
            channel.controls.push_back (std::move (control));
        }
    }
    channels.push_back (std::move (channel));
}

void MixerPanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f),
                mode == Mode::preFilter ? "PRE FILTER MIX" : "POST FILTER MIX");

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto columns = columnCount (area.getWidth());
    const auto rows = (static_cast<int> (channels.size()) + columns - 1) / columns;
    const auto cellWidth = area.getWidth() / juce::jmax (1, columns);
    const auto cellHeight = area.getHeight() / juce::jmax (1, rows);

    g.setFont (juce::FontOptions (9.2f).withStyle ("Bold"));
    for (int i = 0; i < static_cast<int> (channels.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        auto cell = juce::Rectangle<int> (area.getX() + column * cellWidth,
                                          area.getY() + row * cellHeight,
                                          cellWidth, cellHeight).reduced (2);
        g.setColour ((i & 1) == 0 ? juce::Colours::white.withAlpha (0.10f)
                                  : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (cell.toFloat(), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.62f));
        g.drawText (channels[static_cast<std::size_t> (i)].title,
                    cell.removeFromTop (15), juce::Justification::centred, true);
    }
}

void MixerPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto columns = columnCount (area.getWidth());
    const auto rows = (static_cast<int> (channels.size()) + columns - 1) / columns;
    const auto cellWidth = area.getWidth() / juce::jmax (1, columns);
    const auto cellHeight = area.getHeight() / juce::jmax (1, rows);

    for (int i = 0; i < static_cast<int> (channels.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        auto cell = juce::Rectangle<int> (area.getX() + column * cellWidth,
                                          area.getY() + row * cellHeight,
                                          cellWidth, cellHeight).reduced (3);
        cell.removeFromTop (16);
        layoutChannel (channels[static_cast<std::size_t> (i)], cell);
    }
}

int MixerPanel::columnCount (int width) const
{
    if (mode == Mode::postFilter)
        return width >= 360 ? 3 : (width >= 220 ? 2 : 1);
    return width >= 500 ? 3 : (width >= 235 ? 2 : 1);
}

void MixerPanel::layoutChannel (Channel& channel, juce::Rectangle<int> bounds)
{
    if (channel.controls.empty())
        return;
    const auto cellWidth = juce::jmax (1, bounds.getWidth() / static_cast<int> (channel.controls.size()));
    for (auto& control : channel.controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int MixerPanel::preferredHeightForWidth (int width) const
{
    const auto columns = columnCount (juce::jmax (1, width - 16));
    const auto rows = (static_cast<int> (channels.size()) + columns - 1) / columns;
    return 39 + rows * 106 + 8;
}
}
