#include "MicronExtensionsPanel.h"

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
    g.drawText ("MICRON CONTROLS", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::white.withAlpha (0.58f));
    g.setFont (juce::FontOptions (8.8f));
    g.drawText ("Micron only / candidate until hardware verified",
                header.reduced (8.0f, 0.0f), juce::Justification::centredRight, true);
}
}

MicronExtensionsPanel::MicronExtensionsPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addControl (xyzControls, "micron.xyz.x_assignment", registry, state);
    addControl (xyzControls, "micron.xyz.y_assignment", registry, state);
    addControl (xyzControls, "micron.xyz.z_assignment", registry, state);

    addControl (fx2Controls, "micron.fx2.balance", registry, state);
    addControl (fx2Controls, "micron.fx2.type", registry, state);
}

void MicronExtensionsPanel::addControl (std::vector<std::unique_ptr<ParameterControl>>& destination,
                                        const char* id,
                                        const ParameterRegistry& registry,
                                        ProgramState& state)
{
    if (const auto* definition = registry.find (id))
    {
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        destination.push_back (std::move (control));
    }
}

void MicronExtensionsPanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f));

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    const auto stacked = getWidth() < 700;
    auto xyzArea = stacked ? area.removeFromTop (area.getHeight() / 2) : area.removeFromLeft (area.getWidth() * 3 / 5);
    auto fxArea = area;

    g.setColour (juce::Colours::black.withAlpha (0.045f));
    g.fillRoundedRectangle (xyzArea.toFloat().reduced (1.0f), 6.0f);
    g.fillRoundedRectangle (fxArea.toFloat().reduced (1.0f), 6.0f);

    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.setFont (juce::FontOptions (9.4f).withStyle ("Bold"));
    g.drawText ("X / Y / Z ASSIGNMENTS", xyzArea.removeFromTop (18).reduced (7, 0), juce::Justification::centredLeft);
    g.drawText ("FX2 / DELAY + REVERB", fxArea.removeFromTop (18).reduced (7, 0), juce::Justification::centredLeft);
}

void MicronExtensionsPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    const auto stacked = getWidth() < 700;
    auto xyzArea = stacked ? area.removeFromTop (area.getHeight() / 2) : area.removeFromLeft (area.getWidth() * 3 / 5);
    auto fxArea = area;

    xyzArea.removeFromTop (18);
    fxArea.removeFromTop (18);

    layoutRow (xyzControls, xyzArea.reduced (3, 1));
    layoutRow (fx2Controls, fxArea.reduced (3, 1));
}

void MicronExtensionsPanel::layoutRow (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                       juce::Rectangle<int> bounds)
{
    if (controls.empty())
        return;

    constexpr int gap = 6;
    const auto count = static_cast<int> (controls.size());
    const auto usable = juce::jmax (1, bounds.getWidth() - gap * (count - 1));
    const auto cell = juce::jmax (1, usable / count);

    for (auto& control : controls)
    {
        control->setBounds (bounds.removeFromLeft (cell));
        bounds.removeFromLeft (gap);
    }
}

int MicronExtensionsPanel::preferredHeightForWidth (int width) const
{
    return width < 700 ? 238 : 145;
}
}
