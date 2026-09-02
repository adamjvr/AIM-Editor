#include "ModMatrixPanel.h"
#include <initializer_list>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace aim
{
namespace
{
std::string slotPrefix (int zeroBasedSlot)
{
    std::ostringstream stream;
    stream << "mod_matrix.slot" << std::setw (2) << std::setfill ('0') << (zeroBasedSlot + 1);
    return stream.str();
}

void fillSelector (juce::ComboBox& selector,
                   std::vector<int>& rawValues,
                   const ParameterDefinition& definition)
{
    selector.clear (juce::dontSendNotification);
    rawValues.clear();

    int itemId = 1;
    for (const auto& item : definition.enumValues)
    {
        selector.addItem (item.name.isNotEmpty() ? item.name : item.id, itemId++);
        rawValues.push_back (item.raw);
    }

    if (rawValues.empty())
    {
        selector.addItem ("Unmapped", 1);
        rawValues.push_back (0);
        selector.setEnabled (false);
    }
}

void configurePercentSlider (juce::Slider& slider, const ParameterDefinition& definition)
{
    const auto minimum = definition.rawMin.value_or (-1000.0);
    const auto maximum = definition.rawMax.value_or (1000.0);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 68, 22);
    slider.setRange (minimum, maximum, 1.0);
    slider.setDoubleClickReturnValue (true, definition.defaultRaw.value_or (0.0));
    slider.textFromValueFunction = [] (double raw)
    {
        return juce::String (raw * 0.1, 1) + "%";
    };
    slider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.retainCharacters ("-+.0123456789").getDoubleValue() * 10.0;
    };
}
}

class ModMatrixPanel::Row final : public juce::Component,
                                  private ProgramState::Listener
{
public:
    Row (int slot, const ParameterRegistry& registry, ProgramState& stateToUse)
        : slotIndex (slot), state (stateToUse)
    {
        const auto prefix = slotPrefix (slot);
        sourceDefinition = registry.find (prefix + ".source");
        levelDefinition = registry.find (prefix + ".level");
        offsetDefinition = registry.find (prefix + ".offset");
        destinationDefinition = registry.find (prefix + ".destination");

        slotLabel.setText (juce::String (slot + 1), juce::dontSendNotification);
        slotLabel.setJustificationType (juce::Justification::centred);
        slotLabel.setFont (juce::FontOptions (11.0f));

        for (auto* component : std::initializer_list<juce::Component*> { static_cast<juce::Component*> (&slotLabel), &source, &level, &offset, &destination })
            addAndMakeVisible (component);

        if (sourceDefinition != nullptr)
            fillSelector (source, sourceRawValues, *sourceDefinition);
        else
            source.setEnabled (false);

        if (destinationDefinition != nullptr)
            fillSelector (destination, destinationRawValues, *destinationDefinition);
        else
            destination.setEnabled (false);

        if (levelDefinition != nullptr)
            configurePercentSlider (level, *levelDefinition);
        else
            level.setEnabled (false);

        if (offsetDefinition != nullptr)
            configurePercentSlider (offset, *offsetDefinition);
        else
            offset.setEnabled (false);

        source.onChange = [this]
        {
            if (sourceDefinition == nullptr)
                return;
            const auto index = source.getSelectedItemIndex();
            if (juce::isPositiveAndBelow (index, static_cast<int> (sourceRawValues.size())))
                (void) state.setValue (sourceDefinition->id, sourceRawValues[static_cast<std::size_t> (index)]);
        };
        destination.onChange = [this]
        {
            if (destinationDefinition == nullptr)
                return;
            const auto index = destination.getSelectedItemIndex();
            if (juce::isPositiveAndBelow (index, static_cast<int> (destinationRawValues.size())))
                (void) state.setValue (destinationDefinition->id, destinationRawValues[static_cast<std::size_t> (index)]);
        };
        level.onValueChange = [this]
        {
            if (levelDefinition != nullptr)
                (void) state.setValue (levelDefinition->id, level.getValue());
        };
        offset.onValueChange = [this]
        {
            if (offsetDefinition != nullptr)
                (void) state.setValue (offsetDefinition->id, offset.getValue());
        };

        state.addListener (this);
        refresh();
    }

    ~Row() override
    {
        state.removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        if ((slotIndex & 1) != 0)
        {
            g.setColour (juce::Colours::black.withAlpha (0.035f));
            g.fillAll();
        }
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4, 3);
        const auto gap = 5;
        slotLabel.setBounds (area.removeFromLeft (26));
        area.removeFromLeft (gap);

        const auto sourceWidth = juce::jmax (88, static_cast<int> (area.getWidth() * 0.28f));
        const auto destinationWidth = juce::jmax (88, static_cast<int> (area.getWidth() * 0.28f));
        source.setBounds (area.removeFromLeft (sourceWidth));
        area.removeFromLeft (gap);
        destination.setBounds (area.removeFromRight (destinationWidth));
        area.removeFromRight (gap);

        const auto half = juce::jmax (60, (area.getWidth() - gap) / 2);
        level.setBounds (area.removeFromLeft (half));
        area.removeFromLeft (gap);
        offset.setBounds (area);
    }

private:
    void parameterValueChanged (std::string_view id,
                                const juce::var&,
                                ProgramChangeOrigin) override
    {
        if ((sourceDefinition != nullptr && id == sourceDefinition->id)
            || (levelDefinition != nullptr && id == levelDefinition->id)
            || (offsetDefinition != nullptr && id == offsetDefinition->id)
            || (destinationDefinition != nullptr && id == destinationDefinition->id))
            refreshOnMessageThread();
    }

    void programReplaced (ProgramChangeOrigin) override
    {
        refreshOnMessageThread();
    }

    void refreshOnMessageThread()
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            refresh();
            return;
        }

        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Row> (this)]
        {
            if (safe != nullptr)
                safe->refresh();
        });
    }

    void refreshSelector (juce::ComboBox& selector,
                          const std::vector<int>& rawValues,
                          const ParameterDefinition* definition)
    {
        if (definition == nullptr)
            return;
        const auto* value = state.valueFor (definition->id);
        if (value == nullptr)
            return;

        const auto raw = static_cast<int> (*value);
        const auto found = std::find (rawValues.begin(), rawValues.end(), raw);
        if (found != rawValues.end())
        {
            selector.setSelectedItemIndex (static_cast<int> (std::distance (rawValues.begin(), found)),
                                           juce::dontSendNotification);
        }
        else
        {
            selector.setSelectedId (0, juce::dontSendNotification);
            selector.setText ("Unknown " + juce::String (raw), juce::dontSendNotification);
        }
    }

    void refresh()
    {
        refreshSelector (source, sourceRawValues, sourceDefinition);
        refreshSelector (destination, destinationRawValues, destinationDefinition);

        if (levelDefinition != nullptr)
            if (const auto* value = state.valueFor (levelDefinition->id))
                level.setValue (static_cast<double> (*value), juce::dontSendNotification);
        if (offsetDefinition != nullptr)
            if (const auto* value = state.valueFor (offsetDefinition->id))
                offset.setValue (static_cast<double> (*value), juce::dontSendNotification);
    }

    int slotIndex = 0;
    ProgramState& state;
    const ParameterDefinition* sourceDefinition = nullptr;
    const ParameterDefinition* levelDefinition = nullptr;
    const ParameterDefinition* offsetDefinition = nullptr;
    const ParameterDefinition* destinationDefinition = nullptr;

    juce::Label slotLabel;
    juce::ComboBox source;
    juce::Slider level;
    juce::Slider offset;
    juce::ComboBox destination;
    std::vector<int> sourceRawValues;
    std::vector<int> destinationRawValues;
};

ModMatrixPanel::ModMatrixPanel (const ParameterRegistry& registry, ProgramState& state)
{
    rows.reserve (12);
    for (int slot = 0; slot < 12; ++slot)
    {
        auto row = std::make_unique<Row> (slot, registry, state);
        addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }
}

ModMatrixPanel::~ModMatrixPanel() = default;

void ModMatrixPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour::fromRGB (205, 205, 203));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds, 10.0f, 1.25f);

    const auto header = bounds.withHeight (27.0f);
    g.setColour (juce::Colour::fromRGB (50, 50, 50));
    g.fillRoundedRectangle (header, 9.0f);
    g.fillRect (header.withTop (header.getCentreY()));
    g.setColour (juce::Colour::fromRGB (225, 30, 30));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("MOD MATRIX", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);

    auto columns = getLocalBounds().reduced (8, 0).withTop (30).withHeight (20);
    const auto gap = 5;
    columns.removeFromLeft (26 + gap);
    const auto sourceWidth = juce::jmax (88, static_cast<int> (columns.getWidth() * 0.28f));
    const auto destinationWidth = juce::jmax (88, static_cast<int> (columns.getWidth() * 0.28f));

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("SOURCE", columns.removeFromLeft (sourceWidth), juce::Justification::centred);
    columns.removeFromLeft (gap);
    auto destinationArea = columns.removeFromRight (destinationWidth);
    columns.removeFromRight (gap);
    const auto half = juce::jmax (60, (columns.getWidth() - gap) / 2);
    g.drawText ("LEVEL", columns.removeFromLeft (half), juce::Justification::centred);
    columns.removeFromLeft (gap);
    g.drawText ("OFFSET", columns, juce::Justification::centred);
    g.drawText ("DESTINATION", destinationArea, juce::Justification::centred);
}

void ModMatrixPanel::resized()
{
    auto area = getLocalBounds().reduced (4);
    area.removeFromTop (49);
    const auto rowHeight = getWidth() < 650 ? 40 : 33;
    for (auto& row : rows)
        row->setBounds (area.removeFromTop (rowHeight));
}

int ModMatrixPanel::preferredHeightForWidth (int width) const
{
    const auto rowHeight = width < 650 ? 40 : 33;
    return 57 + static_cast<int> (rows.size()) * rowHeight;
}
}
