#include "OutputPanel.h"

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
    g.drawText ("OUTPUT", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

OutputPanel::OutputPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addControls (driveControls, { "output.type", "output.drive", "output.bypass" }, registry, state);
    addControls (levelControls, { "output.fx_mix", "output.program_level" }, registry, state);
    addControls (routingControls, { "output.prefilter_signal", "output.filter1_polarity" }, registry, state);
}

void OutputPanel::addControls (std::vector<std::unique_ptr<ParameterControl>>& destination,
                               std::initializer_list<const char*> ids,
                               const ParameterRegistry& registry,
                               ProgramState& state)
{
    for (const auto* id : ids)
    {
        if (const auto* definition = registry.find (id))
        {
            auto control = std::make_unique<ParameterControl> (*definition, state);
            addAndMakeVisible (*control);
            destination.push_back (std::move (control));
        }
    }
}

void OutputPanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f));
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto thirds = juce::jmax (1, area.getHeight() / 3);
    const juce::StringArray labels { "DRIVE", "LEVEL", "ROUTING" };

    for (int i = 0; i < 3; ++i)
    {
        auto row = juce::Rectangle<int> (area.getX(), area.getY() + i * thirds,
                                         area.getWidth(), i == 2 ? area.getBottom() - (area.getY() + i * thirds) : thirds);
        g.setColour ((i & 1) == 0 ? juce::Colours::white.withAlpha (0.11f)
                                  : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (row.toFloat().reduced (0.0f, 2.0f), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.setFont (juce::FontOptions (8.8f).withStyle ("Bold"));
        g.drawText (labels[i], row.removeFromLeft (42), juce::Justification::centred);
    }
}

void OutputPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto thirds = juce::jmax (1, area.getHeight() / 3);
    auto drive = area.removeFromTop (thirds).withTrimmedLeft (44);
    auto level = area.removeFromTop (thirds).withTrimmedLeft (44);
    auto routing = area.withTrimmedLeft (44);

    layoutControls (driveControls, drive);
    layoutControls (levelControls, level);
    layoutControls (routingControls, routing);
}

void OutputPanel::layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                  juce::Rectangle<int> bounds)
{
    if (controls.empty())
        return;
    const auto cellWidth = juce::jmax (1, bounds.getWidth() / static_cast<int> (controls.size()));
    for (auto& control : controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int OutputPanel::preferredHeightForWidth (int width) const
{
    return width < 210 ? 330 : 290;
}
}
