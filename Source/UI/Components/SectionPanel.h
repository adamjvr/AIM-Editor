#pragma once

#include "Core/ParameterDefinition.h"
#include "Core/ProgramState.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace aim
{
/** One JSON-defined parameter control bound to ProgramState.

    The concrete widget is selected from the parameter metadata: rotary/value
    controls use Slider, enumerations use ComboBox, and binary controls use a
    ToggleButton. The protocol layer is intentionally not involved here.
*/
class ParameterControl final : public juce::Component,
                               private ProgramState::Listener
{
public:
    ParameterControl (const ParameterDefinition& definition, ProgramState& state);
    ~ParameterControl() override;

    void resized() override;

private:
    enum class WidgetKind { slider, selector, toggle };

    void parameterValueChanged (std::string_view id,
                                const juce::var& value,
                                ProgramChangeOrigin origin) override;
    void programReplaced (ProgramChangeOrigin origin) override;
    void refreshFromState();
    void refreshValueText (const juce::var& value);
    [[nodiscard]] juce::String displayTextFor (const juce::var& value) const;
    [[nodiscard]] WidgetKind chooseWidgetKind() const;

    const ParameterDefinition& definition;
    ProgramState& state;
    WidgetKind widgetKind;

    juce::Slider slider;
    juce::ComboBox selector;
    juce::ToggleButton toggle;
    juce::Label label;
    juce::Label valueLabel;
    std::vector<int> selectorRawValues;
};

class EditorSection : public juce::Component
{
public:
    ~EditorSection() override = default;
    [[nodiscard]] virtual int preferredHeightForWidth (int width) const = 0;
};

class SectionPanel final : public EditorSection
{
public:
    SectionPanel (juce::String title,
                  const std::vector<const ParameterDefinition*>& definitions,
                  ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;

    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    [[nodiscard]] int columnsForWidth (int width) const;

    juce::String title;
    std::vector<std::unique_ptr<ParameterControl>> controls;
};
}
