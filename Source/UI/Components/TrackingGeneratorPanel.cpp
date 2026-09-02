#include "TrackingGeneratorPanel.h"
#include <initializer_list>

#include <algorithm>
#include <cmath>

namespace aim
{
TrackingGeneratorPanel::TrackingGeneratorPanel (const ParameterRegistry& registryToUse, ProgramState& stateToUse)
    : registry (registryToUse), state (stateToUse)
{
    for (auto* component : std::initializer_list<juce::Component*> { static_cast<juce::Component*> (&inputRaw), &inputLabel, &gridSelector, &gridLabel,
                             &presetSelector, &presetLabel, &linear, &invert, &zero, &status })
        addAndMakeVisible (component);

    inputLabel.setText ("Input raw", juce::dontSendNotification);
    inputLabel.setFont (juce::FontOptions (11.0f));
    inputRaw.setSliderStyle (juce::Slider::LinearHorizontal);
    inputRaw.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 22);
    inputRaw.setRange (0.0, 79.0, 1.0);
    inputRaw.onValueChange = [this]
    {
        (void) state.setValue ("tracking_generator.input", static_cast<int> (inputRaw.getValue()));
    };
    inputRaw.setTooltip ("Candidate SysEx domain 0..79. Named input-source translation is not verified yet.");

    gridLabel.setText ("Grid", juce::dontSendNotification);
    gridLabel.setFont (juce::FontOptions (11.0f));
    gridSelector.addItem ("12", 1);
    gridSelector.addItem ("16", 2);
    gridSelector.onChange = [this]
    {
        const auto raw = gridSelector.getSelectedItemIndex();
        if (raw >= 0)
            (void) state.setValue ("tracking_generator.point_count", raw);
    };

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setFont (juce::FontOptions (11.0f));
    if (const auto* preset = registry.find ("tracking_generator.preset"))
        for (const auto& value : preset->enumValues)
            presetSelector.addItem (value.name, value.raw + 1);
    presetSelector.onChange = [this]
    {
        const auto raw = presetSelector.getSelectedId() - 1;
        if (raw >= 0)
            (void) state.setValue ("tracking_generator.preset", raw);
    };
    presetSelector.setTooltip ("Candidate hardware preset field. Selecting it does not synthesize undocumented curve values locally.");

    linear.onClick = [this] { applyLinear(); };
    invert.onClick = [this] { invertCurve(); };
    zero.onClick = [this] { zeroCurve(); };

    status.setText ("Drag curve points directly. Point mappings are candidate until verified on hardware.",
                    juce::dontSendNotification);
    status.setFont (juce::FontOptions (10.5f));
    status.setColour (juce::Label::textColourId, juce::Colours::black.withAlpha (0.62f));

    state.addListener (this);
    refreshControls();
}

TrackingGeneratorPanel::~TrackingGeneratorPanel()
{
    state.removeListener (this);
}

void TrackingGeneratorPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour::fromRGB (184, 184, 181));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    auto header = getLocalBounds().removeFromTop (32);
    g.setColour (juce::Colour::fromRGB (43, 43, 43));
    g.fillRect (header);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.5f).withStyle ("Bold"));
    g.drawText ("TRACKING GENERATOR", header.reduced (9, 0), juce::Justification::centredLeft);

    const auto graph = graphBounds();
    g.setColour (juce::Colour::fromRGB (27, 29, 30));
    g.fillRoundedRectangle (graph, 3.0f);

    // Grid: centre axes are intentionally stronger than helper divisions.
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    for (int i = 0; i <= 8; ++i)
    {
        const auto x = graph.getX() + graph.getWidth() * static_cast<float> (i) / 8.0f;
        g.drawVerticalLine (static_cast<int> (std::round (x)), graph.getY(), graph.getBottom());
    }
    for (int i = 0; i <= 4; ++i)
    {
        const auto y = graph.getY() + graph.getHeight() * static_cast<float> (i) / 4.0f;
        g.drawHorizontalLine (static_cast<int> (std::round (y)), graph.getX(), graph.getRight());
    }

    g.setColour (juce::Colours::white.withAlpha (0.34f));
    g.drawVerticalLine (static_cast<int> (std::round (graph.getCentreX())), graph.getY(), graph.getBottom());
    g.drawHorizontalLine (static_cast<int> (std::round (graph.getCentreY())), graph.getX(), graph.getRight());

    juce::Path curve;
    std::array<juce::Point<float>, 33> positions;
    for (int point = -16; point <= 16; ++point)
    {
        const auto index = point + 16;
        const auto x = graph.getX() + graph.getWidth() * static_cast<float> (index) / 32.0f;
        const auto normalized = static_cast<float> (std::clamp (pointValue (point), -100.0, 100.0) / 100.0);
        const auto y = graph.getCentreY() - normalized * graph.getHeight() * 0.5f;
        positions[static_cast<std::size_t> (index)] = { x, y };
        if (index == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    g.setColour (juce::Colour::fromRGB (220, 65, 48));
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    for (int point = -16; point <= 16; ++point)
    {
        const auto position = positions[static_cast<std::size_t> (point + 16)];
        const auto selected = point == selectedPoint;
        g.setColour (selected ? juce::Colours::white : juce::Colour::fromRGB (220, 65, 48));
        const auto radius = selected ? 4.5f : 2.6f;
        g.fillEllipse (position.x - radius, position.y - radius, radius * 2.0f, radius * 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.62f));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("-16", static_cast<int> (graph.getX()), static_cast<int> (graph.getBottom()) - 16, 30, 14,
                juce::Justification::centredLeft);
    g.drawText ("0", static_cast<int> (graph.getCentreX()) - 12, static_cast<int> (graph.getBottom()) - 16, 24, 14,
                juce::Justification::centred);
    g.drawText ("+16", static_cast<int> (graph.getRight()) - 30, static_cast<int> (graph.getBottom()) - 16, 30, 14,
                juce::Justification::centredRight);

    const auto selectedValue = pointValue (selectedPoint);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("Point " + juce::String (selectedPoint >= 0 ? "+" : "") + juce::String (selectedPoint)
                    + "   " + juce::String (selectedValue, 0) + "%",
                graph.toNearestInt().withHeight (20).translated (0, -21), juce::Justification::centredRight);
}

void TrackingGeneratorPanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (34);

    auto controls = area.removeFromTop (62);
    auto first = controls.removeFromTop (28);
    inputLabel.setBounds (first.removeFromLeft (58));
    inputRaw.setBounds (first.removeFromLeft (juce::jmax (130, getWidth() / 3)));
    first.removeFromLeft (8);
    gridLabel.setBounds (first.removeFromLeft (34));
    gridSelector.setBounds (first.removeFromLeft (64));
    first.removeFromLeft (8);
    presetLabel.setBounds (first.removeFromLeft (46));
    presetSelector.setBounds (first);

    auto second = controls.removeFromTop (28);
    const auto buttonWidth = juce::jmin (92, second.getWidth() / 4);
    linear.setBounds (second.removeFromLeft (buttonWidth));
    second.removeFromLeft (5);
    invert.setBounds (second.removeFromLeft (buttonWidth));
    second.removeFromLeft (5);
    zero.setBounds (second.removeFromLeft (buttonWidth));
    second.removeFromLeft (8);
    status.setBounds (second);
}

void TrackingGeneratorPanel::mouseDown (const juce::MouseEvent& event)
{
    editPointFromMouse (event);
}

void TrackingGeneratorPanel::mouseDrag (const juce::MouseEvent& event)
{
    editPointFromMouse (event);
}

int TrackingGeneratorPanel::preferredHeightForWidth (int width) const
{
    return width < 520 ? 390 : 355;
}

void TrackingGeneratorPanel::parameterValueChanged (std::string_view id,
                                                     const juce::var&,
                                                     ProgramChangeOrigin)
{
    if (! id.starts_with ("tracking_generator."))
        return;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        refreshControls();
        repaint();
    }
    else
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TrackingGeneratorPanel> (this)]
        {
            if (safe != nullptr)
            {
                safe->refreshControls();
                safe->repaint();
            }
        });
    }
}

void TrackingGeneratorPanel::programReplaced (ProgramChangeOrigin)
{
    refreshControls();
    repaint();
}

juce::Rectangle<float> TrackingGeneratorPanel::graphBounds() const
{
    return getLocalBounds().reduced (12).withTrimmedTop (112).withTrimmedBottom (8).toFloat();
}

std::string TrackingGeneratorPanel::pointId (int point)
{
    if (point < 0)
        return "tracking_generator.point_minus_" + std::to_string (-point);
    return "tracking_generator.point_plus_" + std::to_string (point);
}

double TrackingGeneratorPanel::pointValue (int point) const
{
    if (const auto* value = state.valueFor (pointId (point)); value != nullptr)
        return static_cast<double> (*value);
    return 0.0;
}

int TrackingGeneratorPanel::nearestPointForX (float x) const
{
    const auto graph = graphBounds();
    if (graph.getWidth() <= 0.0f)
        return 0;

    const auto normalized = std::clamp ((x - graph.getX()) / graph.getWidth(), 0.0f, 1.0f);
    return std::clamp (static_cast<int> (std::round (normalized * 32.0f)) - 16, -16, 16);
}

void TrackingGeneratorPanel::editPointFromMouse (const juce::MouseEvent& event)
{
    const auto graph = graphBounds();
    if (! graph.expanded (8.0f).contains (event.position))
        return;

    selectedPoint = nearestPointForX (event.position.x);
    const auto normalized = std::clamp ((graph.getCentreY() - event.position.y) / (graph.getHeight() * 0.5f), -1.0f, 1.0f);
    const auto raw = static_cast<int> (std::round (normalized * 100.0f));
    (void) state.setValue (pointId (selectedPoint), raw, ProgramChangeOrigin::interactive);
    (void) state.setValue ("tracking_generator.selection", selectedPoint, ProgramChangeOrigin::internal);
    repaint();
}

void TrackingGeneratorPanel::refreshControls()
{
    if (const auto* input = state.valueFor ("tracking_generator.input"))
        inputRaw.setValue (static_cast<double> (*input), juce::dontSendNotification);

    if (const auto* grid = state.valueFor ("tracking_generator.point_count"))
        gridSelector.setSelectedItemIndex (std::clamp (static_cast<int> (*grid), 0, 1), juce::dontSendNotification);

    if (const auto* preset = state.valueFor ("tracking_generator.preset"))
    {
        const auto raw = static_cast<int> (*preset);
        if (raw >= 0 && raw <= 9)
            presetSelector.setSelectedId (raw + 1, juce::dontSendNotification);
    }

    if (const auto* selection = state.valueFor ("tracking_generator.selection"))
        selectedPoint = std::clamp (static_cast<int> (*selection), -16, 16);
}

void TrackingGeneratorPanel::applyLinear()
{
    static constexpr int values[] = {
        -100, -93, -87, -81, -75, -68, -62, -56, -50, -43, -37, -31, -25, -18, -12, -6, 0,
        6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100
    };
    for (int point = -16; point <= 16; ++point)
        (void) state.setValue (pointId (point), values[point + 16], ProgramChangeOrigin::internal);
    (void) state.setValue ("tracking_generator.preset", 0, ProgramChangeOrigin::internal);
}

void TrackingGeneratorPanel::invertCurve()
{
    for (int point = -16; point <= 16; ++point)
        (void) state.setValue (pointId (point), static_cast<int> (std::round (-pointValue (point))),
                              ProgramChangeOrigin::internal);
    (void) state.setValue ("tracking_generator.preset", 0, ProgramChangeOrigin::internal);
}

void TrackingGeneratorPanel::zeroCurve()
{
    for (int point = -16; point <= 16; ++point)
        (void) state.setValue (pointId (point), 0, ProgramChangeOrigin::internal);
    (void) state.setValue ("tracking_generator.preset", 0, ProgramChangeOrigin::internal);
}
}
