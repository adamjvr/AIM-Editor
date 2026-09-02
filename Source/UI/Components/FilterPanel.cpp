#include "FilterPanel.h"

namespace aim
{
FilterPanel::FilterPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addLane ("FILTER 1", registry.parametersForSection ("filter1"), state);
    addLane ("FILTER 2", registry.parametersForSection ("filter2"), state);

    for (const auto* definition : registry.parametersForSection ("filters"))
    {
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        routingControls.push_back (std::move (control));
    }
}

void FilterPanel::addLane (juce::String title,
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

void FilterPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
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
    g.drawText ("FILTERS", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto routingHeight = routingControls.empty() ? 0 : 66;
    auto lanesArea = area.withTrimmedBottom (routingHeight + 4);
    const auto laneHeight = lanesArea.getHeight() / juce::jmax (1, static_cast<int> (lanes.size()));

    g.setFont (juce::FontOptions (10.5f).withStyle ("Bold"));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto lane = juce::Rectangle<int> (lanesArea.getX(), lanesArea.getY() + static_cast<int> (i) * laneHeight,
                                          lanesArea.getWidth(), laneHeight);
        g.setColour ((i & 1u) == 0u ? juce::Colours::white.withAlpha (0.11f)
                                    : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (lane.toFloat().reduced (0.0f, 2.0f), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawText (lanes[i].title, lane.removeFromLeft (50).reduced (3, 0), juce::Justification::centred);
    }

    if (! routingControls.empty())
    {
        auto strip = area.removeFromBottom (routingHeight);
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.fillRoundedRectangle (strip.toFloat(), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
        g.drawText ("ROUTING", strip.removeFromLeft (52), juce::Justification::centred);
    }
}

void FilterPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto routingHeight = routingControls.empty() ? 0 : 66;
    auto lanesArea = area;
    if (routingHeight > 0)
        lanesArea.removeFromBottom (routingHeight + 4);

    const auto laneHeight = lanesArea.getHeight() / juce::jmax (1, static_cast<int> (lanes.size()));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto row = juce::Rectangle<int> (lanesArea.getX(), lanesArea.getY() + static_cast<int> (i) * laneHeight,
                                         lanesArea.getWidth(), laneHeight).withTrimmedLeft (52);
        layoutControls (lanes[i].controls, row);
    }

    if (! routingControls.empty())
        layoutControls (routingControls, area.removeFromBottom (routingHeight).withTrimmedLeft (54));
}

void FilterPanel::layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                  juce::Rectangle<int> bounds)
{
    if (controls.empty())
        return;
    const auto cellWidth = juce::jmax (58, bounds.getWidth() / static_cast<int> (controls.size()));
    for (auto& control : controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int FilterPanel::preferredHeightForWidth (int width) const
{
    return width < 330 ? 350 : 300;
}
}
