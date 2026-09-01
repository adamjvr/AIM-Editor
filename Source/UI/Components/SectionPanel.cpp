#include "SectionPanel.h"

#include <algorithm>

namespace aim
{
ParameterKnob::ParameterKnob (const ParameterDefinition& definition)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    if (definition.rawMin && definition.rawMax && *definition.rawMax > *definition.rawMin)
        slider.setRange (*definition.rawMin, *definition.rawMax, 0.0);
    else
        slider.setRange (0.0, 1.0, 0.001);

    slider.setValue (definition.defaultRaw.value_or (slider.getMinimum()), juce::dontSendNotification);
    slider.setTooltip (definition.mappingStatus == MappingStatus::verified
                           ? definition.name
                           : definition.name + " — hardware mapping not verified yet");
    addAndMakeVisible (slider);

    label.setText (definition.name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredTop);
    label.setFont (juce::FontOptions (10.0f));
    label.setMinimumHorizontalScale (0.65f);
    addAndMakeVisible (label);
}

void ParameterKnob::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromBottom (26));
    slider.setBounds (area.reduced (4));
}

SectionPanel::SectionPanel (juce::String newTitle,
                            const std::vector<const ParameterDefinition*>& definitions)
    : title (std::move (newTitle)), totalParameterCount (static_cast<int> (definitions.size()))
{
    constexpr int maximumPreviewControls = 8;
    const auto count = std::min (maximumPreviewControls, totalParameterCount);

    for (int i = 0; i < count; ++i)
    {
        auto knob = std::make_unique<ParameterKnob> (*definitions[static_cast<std::size_t> (i)]);
        addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }
}

void SectionPanel::paint (juce::Graphics& g)
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
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (title, header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (juce::String (totalParameterCount) + " params", header.reduced (8.0f, 0.0f),
                juce::Justification::centredRight, true);
}

void SectionPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    if (knobs.empty())
        return;

    const int columns = juce::jmax (1, juce::jmin (4, area.getWidth() / 82));
    const int rows = (static_cast<int> (knobs.size()) + columns - 1) / columns;
    const int cellWidth = area.getWidth() / columns;
    const int cellHeight = juce::jmax (58, area.getHeight() / juce::jmax (1, rows));

    for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        knobs[static_cast<std::size_t> (i)]->setBounds (area.getX() + column * cellWidth,
                                                        area.getY() + row * cellHeight,
                                                        cellWidth,
                                                        cellHeight);
    }
}
}
