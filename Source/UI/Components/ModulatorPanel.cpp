#include "ModulatorPanel.h"

#include <cmath>

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
    g.drawText ("LFO / S&H / TEMPO", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

ModulatorPanel::ModulatorPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addLane ("LFO 1", "lfo", registry.parametersForSection ("lfo1"), state);
    addLane ("LFO 2", "lfo", registry.parametersForSection ("lfo2"), state);
    addLane ("S&H", "sample_hold", registry.parametersForSection ("sample_hold"), state);

    for (const auto* definition : registry.parametersForSection ("tempo_arp"))
    {
        if (definition == nullptr)
            continue;
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        arpControls.push_back (std::move (control));
    }
}

void ModulatorPanel::addLane (juce::String title,
                              juce::String kind,
                              const std::vector<const ParameterDefinition*>& definitions,
                              ProgramState& state)
{
    Lane lane;
    lane.title = std::move (title);
    lane.kind = std::move (kind);
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

void ModulatorPanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f));

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto arpHeight = arpControls.empty() ? 0 : 76;
    auto laneArea = area.withTrimmedBottom (arpHeight + (arpHeight > 0 ? 4 : 0));
    const auto laneHeight = laneArea.getHeight() / juce::jmax (1, static_cast<int> (lanes.size()));

    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto row = juce::Rectangle<int> (laneArea.getX(),
                                         laneArea.getY() + static_cast<int> (i) * laneHeight,
                                         laneArea.getWidth(), laneHeight);
        g.setColour ((i & 1u) == 0u ? juce::Colours::white.withAlpha (0.11f)
                                    : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (row.toFloat().reduced (0.0f, 2.0f), 5.0f);

        auto legend = row.removeFromLeft (58).reduced (3, 4);
        g.setColour (juce::Colours::black.withAlpha (0.68f));
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.drawText (lanes[i].title, legend.removeFromTop (18), juce::Justification::centred);
        drawWaveGlyph (g, legend.reduced (5.0f, 4.0f).toFloat(), lanes[i].kind);
    }

    if (! arpControls.empty())
    {
        auto strip = area.removeFromBottom (arpHeight);
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.fillRoundedRectangle (strip.toFloat(), 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.setFont (juce::FontOptions (9.5f).withStyle ("Bold"));
        g.drawText ("TEMPO / ARP", strip.removeFromLeft (66), juce::Justification::centred);
    }
}

void ModulatorPanel::drawWaveGlyph (juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    const juce::String& kind)
{
    if (bounds.isEmpty())
        return;

    juce::Path path;
    const auto centreY = bounds.getCentreY();
    g.setColour (juce::Colour::fromRGB (190, 28, 28).withAlpha (0.78f));

    if (kind == "sample_hold")
    {
        const float values[] { 0.68f, 0.25f, 0.55f, 0.18f, 0.78f };
        const auto step = bounds.getWidth() / 5.0f;
        auto x = bounds.getX();
        auto y = bounds.getBottom() - bounds.getHeight() * values[0];
        path.startNewSubPath (x, y);
        for (int i = 1; i < 5; ++i)
        {
            x = bounds.getX() + step * static_cast<float> (i);
            path.lineTo (x, y);
            y = bounds.getBottom() - bounds.getHeight() * values[i];
            path.lineTo (x, y);
        }
        path.lineTo (bounds.getRight(), y);
    }
    else
    {
        constexpr int points = 32;
        for (int i = 0; i < points; ++i)
        {
            const auto t = static_cast<float> (i) / static_cast<float> (points - 1);
            const auto x = bounds.getX() + bounds.getWidth() * t;
            const auto y = centreY - std::sin (t * juce::MathConstants<float>::twoPi) * bounds.getHeight() * 0.32f;
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
    }

    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void ModulatorPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto arpHeight = arpControls.empty() ? 0 : 76;
    auto laneArea = area;
    if (arpHeight > 0)
        laneArea.removeFromBottom (arpHeight + 4);

    const auto laneHeight = laneArea.getHeight() / juce::jmax (1, static_cast<int> (lanes.size()));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto row = juce::Rectangle<int> (laneArea.getX(),
                                         laneArea.getY() + static_cast<int> (i) * laneHeight,
                                         laneArea.getWidth(), laneHeight).withTrimmedLeft (60);
        layoutControls (lanes[i].controls, row);
    }

    if (! arpControls.empty())
        layoutControls (arpControls, area.removeFromBottom (arpHeight).withTrimmedLeft (68));
}

void ModulatorPanel::layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                     juce::Rectangle<int> bounds)
{
    if (controls.empty())
        return;
    const auto cellWidth = juce::jmax (58, bounds.getWidth() / static_cast<int> (controls.size()));
    for (auto& control : controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int ModulatorPanel::preferredHeightForWidth (int width) const
{
    if (width < 250)
        return 430;
    if (width < 360)
        return 360;
    return 330;
}
}
