#include "OscillatorPanel.h"

#include <algorithm>

namespace aim
{
namespace
{
void drawPanelChrome (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
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
    g.drawText (title, header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

OscillatorPanel::OscillatorPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addLane ("OSC 1", registry.parametersForSection ("osc1"), state);
    addLane ("OSC 2", registry.parametersForSection ("osc2"), state);
    addLane ("OSC 3", registry.parametersForSection ("osc3"), state);

    for (const auto* definition : registry.parametersForSection ("oscillators"))
    {
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        globalControls.push_back (std::move (control));
    }
}

void OscillatorPanel::addLane (juce::String title,
                               const std::vector<const ParameterDefinition*>& definitions,
                               ProgramState& state)
{
    Lane lane;
    lane.title = std::move (title);
    for (const auto* definition : definitions)
    {
        if (definition == nullptr)
            continue;
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        lane.controls.push_back (std::move (control));
    }
    lanes.push_back (std::move (lane));
}

void OscillatorPanel::paint (juce::Graphics& g)
{
    drawPanelChrome (g, getLocalBounds().toFloat().reduced (0.5f), "OSCILLATORS");

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto laneHeight = juce::jmax (86, (area.getHeight() - 66) / 3);

    g.setFont (juce::FontOptions (10.5f).withStyle ("Bold"));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto lane = juce::Rectangle<int> (area.getX(), area.getY() + static_cast<int> (i) * laneHeight,
                                                area.getWidth(), laneHeight);
        g.setColour ((i & 1u) == 0u ? juce::Colours::white.withAlpha (0.11f)
                                    : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (lane.toFloat().reduced (0.0f, 2.0f), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawText (lanes[i].title, lane.removeFromLeft (42).reduced (3, 0), juce::Justification::centred);
    }

    if (! globalControls.empty())
    {
        auto strip = getLocalBounds().reduced (8).removeFromBottom (62);
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.fillRoundedRectangle (strip.toFloat(), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
        g.drawText ("COMMON", strip.removeFromLeft (48), juce::Justification::centred);
    }
}

void OscillatorPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    const auto commonHeight = globalControls.empty() ? 0 : 62;
    auto laneArea = area;
    if (commonHeight > 0)
        laneArea.removeFromBottom (commonHeight + 4);

    const auto laneHeight = juce::jmax (86, laneArea.getHeight() / juce::jmax (1, static_cast<int> (lanes.size())));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto row = juce::Rectangle<int> (laneArea.getX(), laneArea.getY() + static_cast<int> (i) * laneHeight,
                                         laneArea.getWidth(), laneHeight);
        layoutLane (lanes[i], row.withTrimmedLeft (44));
    }

    if (! globalControls.empty())
    {
        auto strip = area.removeFromBottom (commonHeight).withTrimmedLeft (48);
        const auto cell = juce::jmax (64, strip.getWidth() / static_cast<int> (globalControls.size()));
        for (auto& control : globalControls)
            control->setBounds (strip.removeFromLeft (cell));
    }
}

void OscillatorPanel::layoutLane (Lane& lane, juce::Rectangle<int> bounds)
{
    if (lane.controls.empty())
        return;

    const auto cellWidth = juce::jmax (56, bounds.getWidth() / static_cast<int> (lane.controls.size()));
    for (auto& control : lane.controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int OscillatorPanel::preferredHeightForWidth (int width) const
{
    return width < 330 ? 470 : (width < 520 ? 410 : 380);
}
}
