#include "SectionPanel.h"
#include "Core/ParameterFormatter.h"

#include <algorithm>
#include <cmath>

namespace aim
{
namespace
{
juce::String humanizeId (juce::String text)
{
    text = text.replaceCharacter ('_', ' ');
    if (text.isNotEmpty())
        text = text.substring (0, 1).toUpperCase() + text.substring (1);
    return text;
}

juce::String compactParameterName (const ParameterDefinition& definition)
{
    const auto id = juce::String::fromUTF8 (definition.id.c_str());
    auto name = definition.name;

    const auto stripPrefix = [&name] (const juce::String& prefix)
    {
        if (name.startsWithIgnoreCase (prefix))
            name = name.substring (prefix.length()).trimStart();
    };

    if (id == "osc.sync") name = "Sync";
    else if (id.startsWith ("osc1.")) stripPrefix ("Oscillator 1 ");
    else if (id.startsWith ("osc2.")) stripPrefix ("Oscillator 2 ");
    else if (id.startsWith ("osc3.")) stripPrefix ("Oscillator 3 ");
    else if (id.startsWith ("filter1.")) stripPrefix ("Filter 1 ");
    else if (id.startsWith ("filter2.")) stripPrefix ("Filter 2 ");
    else if (id.startsWith ("env.pitch.")) stripPrefix ("Pitch Envelope ");
    else if (id.startsWith ("env.filter.")) stripPrefix ("Filter Envelope ");
    else if (id.startsWith ("env.amp.")) stripPrefix ("Amp Envelope ");
    else if (id.startsWith ("lfo1.")) stripPrefix ("LFO 1 ");
    else if (id.startsWith ("lfo2.")) stripPrefix ("LFO 2 ");
    else if (id.startsWith ("sample_hold.")) stripPrefix ("Sample & Hold ");
    else if (id.startsWith ("tempo_arp.")) stripPrefix ("Arpeggiator ");
    else if (id.startsWith ("voice.portamento_")) stripPrefix ("Portamento ");
    else if (id == "voice.unison") name = "Voices";
    else if (id == "voice.detune") name = "Detune";
    else if (id == "voice.drift") name = "Drift";
    else if (id == "voice.poly_mode") name = "Poly / Mono";
    else if (id == "voice.pitch_wheel_mode") name = "Pitch Wheel";
    else if (id.startsWith ("output.")) stripPrefix ("Output ");
    else if (id.startsWith ("effects.")) stripPrefix ("Effects ");

    if (id.startsWith ("pre_filter_mix."))
    {
        if (id.endsWith (".level")) return "Level";
        if (id.endsWith (".balance")) return "Filter 1/2 Balance";
    }

    if (id.startsWith ("post_filter_mix."))
    {
        if (id.endsWith (".level")) return "Level";
        if (id.endsWith (".pan")) return "Pan";
    }

    if (id == "filters.offset") return "Offset";
    if (id == "filters.f1_to_f2") return "F1 to F2 Routing";
    if (id == "output.program_level") return "Program Level";
    if (id == "output.prefilter_signal") return "Prefilter Signal";
    if (id == "output.filter1_polarity") return "F1 Polarity";

    return name;
}
}

ParameterControl::ParameterControl (const ParameterDefinition& definitionToUse,
                                    ProgramState& stateToUse)
    : definition (definitionToUse), state (stateToUse), widgetKind (chooseWidgetKind())
{
    label.setText (compactParameterName (definition), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredTop);
    label.setFont (juce::FontOptions (9.25f));
    label.setMinimumHorizontalScale (0.72f);
    label.setTooltip (definition.name);
    addAndMakeVisible (label);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (juce::FontOptions (9.0f));
    valueLabel.setColour (juce::Label::textColourId, juce::Colours::black.withAlpha (0.72f));
    addAndMakeVisible (valueLabel);

    if (widgetKind == WidgetKind::slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.setScrollWheelEnabled (true);

        auto minimum = definition.rawMin.value_or (definition.nrpnMin.value_or (0.0));
        auto maximum = definition.rawMax.value_or (definition.nrpnMax.value_or (1.0));
        if (maximum <= minimum)
            maximum = minimum + 1.0;

        const auto interval = definition.kind == ParameterKind::continuous ? 0.0 : 1.0;
        slider.setRange (minimum, maximum, interval);
        slider.setDoubleClickReturnValue (true, definition.defaultRaw.value_or (minimum));
        slider.onValueChange = [this]
        {
            (void) state.setValue (definition.id, slider.getValue());
        };
        addAndMakeVisible (slider);
    }
    else if (widgetKind == WidgetKind::selector)
    {
        if (! definition.enumValues.empty())
        {
            int itemId = 1;
            for (const auto& item : definition.enumValues)
            {
                selector.addItem (item.name.isNotEmpty() ? item.name : humanizeId (item.id), itemId++);
                selectorRawValues.push_back (item.raw);
            }
        }
        else if (definition.rawMin && definition.rawMax
                 && (*definition.rawMax - *definition.rawMin) <= 32.0)
        {
            int itemId = 1;
            for (int raw = static_cast<int> (std::ceil (*definition.rawMin));
                 raw <= static_cast<int> (std::floor (*definition.rawMax)); ++raw)
            {
                selector.addItem (juce::String (raw), itemId++);
                selectorRawValues.push_back (raw);
            }
        }
        else
        {
            selector.addItem ("Unmapped", 1);
            selectorRawValues.push_back (0);
            selector.setEnabled (false);
        }

        selector.onChange = [this]
        {
            const auto index = selector.getSelectedItemIndex();
            if (juce::isPositiveAndBelow (index, static_cast<int> (selectorRawValues.size())))
                (void) state.setValue (definition.id, selectorRawValues[static_cast<std::size_t> (index)]);
        };
        addAndMakeVisible (selector);
    }
    else
    {
        toggle.onClick = [this]
        {
            if (definition.enumValues.size() == 2)
            {
                const auto raw = definition.enumValues[toggle.getToggleState() ? 1u : 0u].raw;
                (void) state.setValue (definition.id, raw);
            }
            else
            {
                (void) state.setValue (definition.id, toggle.getToggleState());
            }
        };
        addAndMakeVisible (toggle);
    }

    const auto mappingText = definition.mappingStatus == MappingStatus::verified
                               ? "verified"
                               : (definition.mappingStatus == MappingStatus::candidate ? "candidate mapping" : "unmapped");
    const auto tooltip = definition.name + " — " + mappingText;
    slider.setTooltip (tooltip);
    selector.setTooltip (tooltip);
    toggle.setTooltip (tooltip);

    state.addListener (this);
    refreshFromState();
}

ParameterControl::~ParameterControl()
{
    state.removeListener (this);
}

void ParameterControl::resized()
{
    auto area = getLocalBounds().reduced (3);

    // Compact rows (notably OSC COMMON and Micron X/Y/Z) used to spend almost
    // their entire height on the two text labels, leaving ComboBoxes crushed.
    // Preserve a real 24px interaction target first, then fit text around it.
    if (getHeight() < 78)
    {
        valueLabel.setBounds (area.removeFromTop (14));
        label.setBounds (area.removeFromBottom (20));
    }
    else
    {
        label.setBounds (area.removeFromBottom (28));
        valueLabel.setBounds (area.removeFromBottom (16));
    }

    if (widgetKind == WidgetKind::slider)
        slider.setBounds (area.reduced (5, 0));
    else if (widgetKind == WidgetKind::selector)
        selector.setBounds (area.withSizeKeepingCentre (juce::jmax (24, area.getWidth() - 6), 26));
    else
        toggle.setBounds (area.withSizeKeepingCentre (26, 26));
}

void ParameterControl::parameterValueChanged (std::string_view id,
                                              const juce::var& value,
                                              ProgramChangeOrigin)
{
    if (id != definition.id)
        return;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        if (widgetKind == WidgetKind::slider)
            slider.setValue (static_cast<double> (value), juce::dontSendNotification);
        else if (widgetKind == WidgetKind::selector)
        {
            const auto raw = static_cast<int> (value);
            const auto found = std::find (selectorRawValues.begin(), selectorRawValues.end(), raw);
            if (found != selectorRawValues.end())
                selector.setSelectedItemIndex (static_cast<int> (std::distance (selectorRawValues.begin(), found)),
                                               juce::dontSendNotification);
            else
            {
                selector.setSelectedId (0, juce::dontSendNotification);
                selector.setText ("Unknown " + juce::String (raw), juce::dontSendNotification);
            }
        }
        else if (definition.enumValues.size() == 2)
            toggle.setToggleState (static_cast<int> (value) == definition.enumValues[1].raw,
                                   juce::dontSendNotification);
        else
            toggle.setToggleState (static_cast<bool> (value), juce::dontSendNotification);

        refreshValueText (value);
        return;
    }

    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<ParameterControl> (this)]
    {
        if (safe != nullptr)
            safe->refreshFromState();
    });
}

void ParameterControl::programReplaced (ProgramChangeOrigin)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        refreshFromState();
    else
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<ParameterControl> (this)]
        {
            if (safe != nullptr)
                safe->refreshFromState();
        });
}

void ParameterControl::refreshFromState()
{
    const auto* value = state.valueFor (definition.id);
    if (value == nullptr)
        return;

    if (widgetKind == WidgetKind::slider)
        slider.setValue (static_cast<double> (*value), juce::dontSendNotification);
    else if (widgetKind == WidgetKind::selector)
    {
        const auto raw = static_cast<int> (*value);
        const auto found = std::find (selectorRawValues.begin(), selectorRawValues.end(), raw);
        if (found != selectorRawValues.end())
            selector.setSelectedItemIndex (static_cast<int> (std::distance (selectorRawValues.begin(), found)),
                                           juce::dontSendNotification);
        else
        {
            selector.setSelectedId (0, juce::dontSendNotification);
            selector.setText ("Unknown " + juce::String (raw), juce::dontSendNotification);
        }
    }
    else if (definition.enumValues.size() == 2)
        toggle.setToggleState (static_cast<int> (*value) == definition.enumValues[1].raw,
                               juce::dontSendNotification);
    else
        toggle.setToggleState (static_cast<bool> (*value), juce::dontSendNotification);

    refreshValueText (*value);
}

void ParameterControl::refreshValueText (const juce::var& value)
{
    valueLabel.setText (displayTextFor (value), juce::dontSendNotification);
}

juce::String ParameterControl::displayTextFor (const juce::var& value) const
{
    return ParameterFormatter::format (definition, value);
}

ParameterControl::WidgetKind ParameterControl::chooseWidgetKind() const
{
    if (definition.control == "selector" || definition.kind == ParameterKind::enumeration
        || definition.enumValues.size() > 2)
        return WidgetKind::selector;

    if (definition.control == "toggle" || definition.kind == ParameterKind::boolean)
        return WidgetKind::toggle;

    return WidgetKind::slider;
}

SectionPanel::SectionPanel (juce::String newTitle,
                            const std::vector<const ParameterDefinition*>& definitions,
                            ProgramState& state)
    : title (std::move (newTitle))
{
    for (const auto* definition : definitions)
    {
        if (definition == nullptr)
            continue;

        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        controls.push_back (std::move (control));
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
    g.drawText (juce::String (controls.size()) + " params", header.reduced (8.0f, 0.0f),
                juce::Justification::centredRight, true);
}

void SectionPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);

    if (controls.empty())
        return;

    const auto columns = columnsForWidth (area.getWidth());
    const auto rows = (static_cast<int> (controls.size()) + columns - 1) / columns;
    const auto cellWidth = area.getWidth() / columns;
    const auto cellHeight = juce::jmax (84, area.getHeight() / juce::jmax (1, rows));

    for (int i = 0; i < static_cast<int> (controls.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        controls[static_cast<std::size_t> (i)]->setBounds (area.getX() + column * cellWidth,
                                                           area.getY() + row * cellHeight,
                                                           cellWidth,
                                                           cellHeight);
    }
}

int SectionPanel::preferredHeightForWidth (int width) const
{
    const auto usableWidth = juce::jmax (1, width - 16);
    const auto columns = columnsForWidth (usableWidth);
    const auto rows = (static_cast<int> (controls.size()) + columns - 1) / columns;
    return 39 + juce::jmax (1, rows) * 94 + 8;
}

int SectionPanel::columnsForWidth (int width) const
{
    return juce::jmax (1, juce::jmin (5, width / 86));
}
}
