#include "EffectsPanel.h"

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
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText (title, header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
}
}

EffectsPanel::EffectsPanel (const ParameterRegistry& registry, ProgramState& state)
{
    addControls (toneControls,
                 { "effects.feedback", "effects.notch" }, registry, state);
    addControls (modulationControls,
                 { "effects.lfo_rate", "effects.lfo_depth" }, registry, state);
    addControls (modeControls,
                 { "effects.lfo_shape", "effects.tempo_sync", "effects.bypass" }, registry, state);
}

void EffectsPanel::addControls (std::vector<std::unique_ptr<ParameterControl>>& destination,
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

void EffectsPanel::paint (juce::Graphics& g)
{
    drawChrome (g, getLocalBounds().toFloat().reduced (0.5f),
                deviceProfile == IonFamilyDevice::micron ? "FX1 / MOD FX" : "EFFECTS");

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    auto note = area.removeFromBottom (24);

    g.setColour (juce::Colours::black.withAlpha (0.05f));
    g.fillRoundedRectangle (area.toFloat(), 5.0f);

    const auto half = area.getHeight() / 2;
    auto upper = area.removeFromTop (half);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("TONE", upper.removeFromLeft (38), juce::Justification::centred);
    g.drawText ("MOD", area.removeFromLeft (38), juce::Justification::centred);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.setFont (juce::FontOptions (8.6f));
    g.drawText (deviceProfile == IonFamilyDevice::micron
                    ? "Micron FX2 delay/reverb controls are exposed in MICRON CONTROLS"
                    : "FX labels depend on the selected hardware algorithm",
                note, juce::Justification::centred, true);
}


void EffectsPanel::setDeviceProfile (IonFamilyDevice device)
{
    if (deviceProfile == device)
        return;

    deviceProfile = device;
    repaint();
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    area.removeFromBottom (24);

    auto upper = area.removeFromTop (area.getHeight() / 2).withTrimmedLeft (40);
    auto lower = area.withTrimmedLeft (40);

    if (getWidth() >= 320)
    {
        auto upperLeft = upper.removeFromLeft (upper.getWidth() / 2);
        layoutControls (toneControls, upperLeft);
        layoutControls (modulationControls, upper);
        layoutControls (modeControls, lower);
    }
    else
    {
        layoutControls (toneControls, upper);
        auto lowerTop = lower.removeFromTop (lower.getHeight() / 2);
        layoutControls (modulationControls, lowerTop);
        layoutControls (modeControls, lower);
    }
}

void EffectsPanel::layoutControls (std::vector<std::unique_ptr<ParameterControl>>& controls,
                                   juce::Rectangle<int> bounds)
{
    if (controls.empty())
        return;
    const auto cellWidth = juce::jmax (1, bounds.getWidth() / static_cast<int> (controls.size()));
    for (auto& control : controls)
        control->setBounds (bounds.removeFromLeft (cellWidth));
}

int EffectsPanel::preferredHeightForWidth (int width) const
{
    return width < 280 ? 360 : (width < 420 ? 300 : 250);
}
}
