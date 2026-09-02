#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <string>

namespace aim
{
/** Touch-friendly 33-point Tracking Generator editor.

    The 33 candidate curve bytes (-16..+16) are first-class semantic parameters
    in data/parameters.json. The graph therefore edits the same ProgramState as
    JSON import/export, SysEx decoding, NRPN input, and the rest of the GUI.
*/
class TrackingGeneratorPanel final : public EditorSection,
                                     private ProgramState::Listener
{
public:
    TrackingGeneratorPanel (const ParameterRegistry& registry, ProgramState& state);
    ~TrackingGeneratorPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    void parameterValueChanged (std::string_view id,
                                const juce::var& value,
                                ProgramChangeOrigin origin) override;
    void programReplaced (ProgramChangeOrigin origin) override;

    [[nodiscard]] juce::Rectangle<float> graphBounds() const;
    [[nodiscard]] static std::string pointId (int point);
    [[nodiscard]] double pointValue (int point) const;
    [[nodiscard]] int nearestPointForX (float x) const;
    void editPointFromMouse (const juce::MouseEvent& event);
    void refreshControls();
    void applyLinear();
    void invertCurve();
    void zeroCurve();

    const ParameterRegistry& registry;
    ProgramState& state;

    juce::Slider inputRaw;
    juce::Label inputLabel;
    juce::ComboBox gridSelector;
    juce::Label gridLabel;
    juce::ComboBox presetSelector;
    juce::Label presetLabel;
    juce::TextButton linear { "linear" };
    juce::TextButton invert { "invert" };
    juce::TextButton zero { "zero" };
    juce::Label status;

    int selectedPoint = 0;
};
}
