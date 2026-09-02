#include "EnvelopePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace aim
{
class EnvelopePanel::EnvelopeGraph final : public juce::Component,
                                           private ProgramState::Listener
{
public:
    EnvelopeGraph (juce::String prefixToUse,
                   const ParameterRegistry& registry,
                   ProgramState& stateToUse)
        : prefix (std::move (prefixToUse)), state (stateToUse)
    {
        attack = registry.find ((prefix + ".attack").toStdString());
        decay = registry.find ((prefix + ".decay").toStdString());
        sustain = registry.find ((prefix + ".sustain").toStdString());
        release = registry.find ((prefix + ".release").toStdString());
        state.addListener (this);
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
    }

    ~EnvelopeGraph() override
    {
        state.removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        const auto graph = graphBounds();
        g.setColour (juce::Colour::fromRGB (29, 30, 31));
        g.fillRoundedRectangle (graph, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        for (int i = 1; i < 4; ++i)
        {
            const auto x = graph.getX() + graph.getWidth() * static_cast<float> (i) / 4.0f;
            g.drawVerticalLine (static_cast<int> (std::round (x)), graph.getY(), graph.getBottom());
        }
        for (int i = 1; i < 4; ++i)
        {
            const auto y = graph.getY() + graph.getHeight() * static_cast<float> (i) / 4.0f;
            g.drawHorizontalLine (static_cast<int> (std::round (y)), graph.getX(), graph.getRight());
        }

        const auto handles = handlePositions();
        juce::Path envelope;
        envelope.startNewSubPath (graph.getX(), graph.getBottom());
        envelope.lineTo (handles[0]);
        envelope.lineTo (handles[1]);
        envelope.lineTo (handles[3]);
        envelope.lineTo (graph.getRight(), graph.getBottom());

        g.setColour (juce::Colour::fromRGB (226, 43, 40));
        g.strokePath (envelope, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        for (std::size_t i = 0; i < handles.size(); ++i)
        {
            const auto selected = static_cast<int> (i) == selectedHandle;
            const auto radius = selected ? 5.0f : 3.4f;
            g.setColour (selected ? juce::Colours::white : juce::Colour::fromRGB (226, 43, 40));
            g.fillEllipse (handles[i].x - radius, handles[i].y - radius, radius * 2.0f, radius * 2.0f);
        }

        g.setFont (juce::FontOptions (8.5f).withStyle ("Bold"));
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawText ("A", graph.withWidth (24.0f).toNearestInt(), juce::Justification::centredTop);
        g.drawText ("D", juce::Rectangle<float> (handles[1].x - 12.0f, graph.getY(), 24.0f, 14.0f).toNearestInt(),
                    juce::Justification::centredTop);
        g.drawText ("S", juce::Rectangle<float> (handles[2].x - 12.0f, graph.getY(), 24.0f, 14.0f).toNearestInt(),
                    juce::Justification::centredTop);
        g.drawText ("R", graph.withLeft (graph.getRight() - 24.0f).toNearestInt(), juce::Justification::centredTop);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        const auto handles = handlePositions();
        float best = std::numeric_limits<float>::max();
        selectedHandle = 0;
        for (int i = 0; i < static_cast<int> (handles.size()); ++i)
        {
            const auto distance = handles[static_cast<std::size_t> (i)].getDistanceFrom (event.position);
            if (distance < best)
            {
                best = distance;
                selectedHandle = i;
            }
        }

        dragStart = event.position;
        dragStartRaw = rawForHandle (selectedHandle);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto* definition = definitionForHandle (selectedHandle);
        if (definition == nullptr)
            return;

        const auto minimum = definition->rawMin.value_or (0.0);
        const auto maximum = definition->rawMax.value_or (255.0);
        const auto range = juce::jmax (1.0, maximum - minimum);
        const auto graph = graphBounds();
        double candidate = dragStartRaw;

        if (selectedHandle == 2)
            candidate -= static_cast<double> (event.position.y - dragStart.y) / juce::jmax (1.0f, graph.getHeight()) * range;
        else
            candidate += static_cast<double> (event.position.x - dragStart.x) / juce::jmax (1.0f, graph.getWidth()) * range * 2.5;

        candidate = std::clamp (candidate, minimum, maximum);
        if (definition->kind != ParameterKind::continuous)
            candidate = std::round (candidate);

        (void) state.setValue (definition->id, candidate, ProgramChangeOrigin::interactive);
    }

private:
    void parameterValueChanged (std::string_view id, const juce::var&, ProgramChangeOrigin) override
    {
        if ((attack != nullptr && id == attack->id)
            || (decay != nullptr && id == decay->id)
            || (sustain != nullptr && id == sustain->id)
            || (release != nullptr && id == release->id))
            repaintOnMessageThread();
    }

    void programReplaced (ProgramChangeOrigin) override
    {
        repaintOnMessageThread();
    }

    void repaintOnMessageThread()
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            repaint();
        else
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<EnvelopeGraph> (this)]
            {
                if (safe != nullptr)
                    safe->repaint();
            });
    }

    [[nodiscard]] juce::Rectangle<float> graphBounds() const
    {
        return getLocalBounds().reduced (4).toFloat();
    }

    [[nodiscard]] double normalized (const ParameterDefinition* definition) const
    {
        if (definition == nullptr)
            return 0.0;
        const auto* value = state.valueFor (definition->id);
        if (value == nullptr)
            return 0.0;
        const auto minimum = definition->rawMin.value_or (0.0);
        const auto maximum = definition->rawMax.value_or (255.0);
        if (maximum <= minimum)
            return 0.0;
        return std::clamp ((static_cast<double> (*value) - minimum) / (maximum - minimum), 0.0, 1.0);
    }

    [[nodiscard]] std::array<juce::Point<float>, 4> handlePositions() const
    {
        const auto graph = graphBounds();
        const auto attackN = static_cast<float> (normalized (attack));
        const auto decayN = static_cast<float> (normalized (decay));
        const auto sustainN = static_cast<float> (normalized (sustain));
        const auto releaseN = static_cast<float> (normalized (release));

        const auto attackWidth = graph.getWidth() * (0.08f + attackN * 0.22f);
        const auto decayWidth = graph.getWidth() * (0.08f + decayN * 0.18f);
        const auto releaseWidth = graph.getWidth() * (0.08f + releaseN * 0.22f);
        const auto attackPoint = juce::Point<float> (graph.getX() + attackWidth, graph.getY() + 5.0f);
        const auto sustainY = graph.getBottom() - 5.0f - sustainN * (graph.getHeight() - 10.0f);
        const auto decayPoint = juce::Point<float> (juce::jmin (attackPoint.x + decayWidth, graph.getRight() - releaseWidth - 12.0f), sustainY);
        const auto releasePoint = juce::Point<float> (juce::jmax (decayPoint.x + 12.0f, graph.getRight() - releaseWidth), sustainY);
        const auto sustainPoint = juce::Point<float> ((decayPoint.x + releasePoint.x) * 0.5f, sustainY);
        return { attackPoint, decayPoint, sustainPoint, releasePoint };
    }

    [[nodiscard]] const ParameterDefinition* definitionForHandle (int handle) const
    {
        if (handle == 0) return attack;
        if (handle == 1) return decay;
        if (handle == 2) return sustain;
        if (handle == 3) return release;
        return attack;
    }

    [[nodiscard]] double rawForHandle (int handle) const
    {
        const auto* definition = definitionForHandle (handle);
        if (definition == nullptr)
            return 0.0;
        if (const auto* value = state.valueFor (definition->id))
            return static_cast<double> (*value);
        return definition->rawMin.value_or (0.0);
    }

    juce::String prefix;
    ProgramState& state;
    const ParameterDefinition* attack = nullptr;
    const ParameterDefinition* decay = nullptr;
    const ParameterDefinition* sustain = nullptr;
    const ParameterDefinition* release = nullptr;
    int selectedHandle = 0;
    juce::Point<float> dragStart;
    double dragStartRaw = 0.0;
};

EnvelopePanel::EnvelopePanel (const ParameterRegistry& registry, ProgramState& state)
{
    addLane ("PITCH", "env.pitch", "env_pitch", registry, state);
    addLane ("FILTER", "env.filter", "env_filter", registry, state);
    addLane ("AMP", "env.amp", "env_amp", registry, state);
}

EnvelopePanel::~EnvelopePanel() = default;

void EnvelopePanel::addLane (juce::String title,
                             juce::String prefix,
                             juce::String section,
                             const ParameterRegistry& registry,
                             ProgramState& state)
{
    Lane lane;
    lane.title = std::move (title);
    lane.graph = std::make_unique<EnvelopeGraph> (std::move (prefix), registry, state);
    addAndMakeVisible (*lane.graph);

    for (const auto* definition : registry.parametersForSection (section))
    {
        if (definition == nullptr)
            continue;
        auto control = std::make_unique<ParameterControl> (*definition, state);
        addAndMakeVisible (*control);
        lane.controls.push_back (std::move (control));
    }
    lanes.push_back (std::move (lane));
}

void EnvelopePanel::paint (juce::Graphics& g)
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
    g.drawText ("ENVELOPES", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);

    const auto wide = getWidth() >= 780;
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        juce::Rectangle<int> lane;
        if (wide)
        {
            const auto w = area.getWidth() / static_cast<int> (lanes.size());
            lane = { area.getX() + static_cast<int> (i) * w, area.getY(), w, area.getHeight() };
        }
        else
        {
            const auto h = area.getHeight() / static_cast<int> (lanes.size());
            lane = { area.getX(), area.getY() + static_cast<int> (i) * h, area.getWidth(), h };
        }

        g.setColour ((i & 1u) == 0u ? juce::Colours::white.withAlpha (0.10f)
                                    : juce::Colours::black.withAlpha (0.025f));
        g.fillRoundedRectangle (lane.toFloat().reduced (3.0f), 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.68f));
        g.drawText (lanes[i].title + " ENV", lane.removeFromTop (18).reduced (8, 0), juce::Justification::centredLeft);
    }
}

void EnvelopePanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (31);
    const auto wide = getWidth() >= 780;

    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        juce::Rectangle<int> laneBounds;
        if (wide)
        {
            const auto width = area.getWidth() / static_cast<int> (lanes.size());
            laneBounds = { area.getX() + static_cast<int> (i) * width, area.getY(), width, area.getHeight() };
        }
        else
        {
            const auto height = area.getHeight() / static_cast<int> (lanes.size());
            laneBounds = { area.getX(), area.getY() + static_cast<int> (i) * height, area.getWidth(), height };
        }
        layoutLane (lanes[i], laneBounds.reduced (5).withTrimmedTop (18));
    }
}

void EnvelopePanel::layoutLane (Lane& lane, juce::Rectangle<int> bounds)
{
    const auto graphHeight = juce::jmin (122, juce::jmax (86, bounds.getHeight() / 3));
    lane.graph->setBounds (bounds.removeFromTop (graphHeight));
    bounds.removeFromTop (4);

    if (lane.controls.empty())
        return;

    const auto columns = juce::jmax (3, juce::jmin (7, bounds.getWidth() / 72));
    const auto rows = (static_cast<int> (lane.controls.size()) + columns - 1) / columns;
    const auto cellWidth = bounds.getWidth() / columns;
    const auto cellHeight = juce::jmax (74, bounds.getHeight() / juce::jmax (1, rows));

    for (int i = 0; i < static_cast<int> (lane.controls.size()); ++i)
    {
        const auto column = i % columns;
        const auto row = i / columns;
        lane.controls[static_cast<std::size_t> (i)]->setBounds (bounds.getX() + column * cellWidth,
                                                                bounds.getY() + row * cellHeight,
                                                                cellWidth, cellHeight);
    }
}

int EnvelopePanel::preferredHeightForWidth (int width) const
{
    if (width >= 780)
        return 405;
    if (width >= 520)
        return 850;
    return 1030;
}
}
