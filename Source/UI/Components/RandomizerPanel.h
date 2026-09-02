#pragma once

#include "Core/ParameterRegistry.h"
#include "Core/ProgramState.h"
#include "Core/RandomizerEngine.h"
#include "UI/Components/SectionPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

namespace aim
{
/** Editor-side patch randomizer inspired by the legacy Ion editor.

    Randomization operates on semantic ProgramState only. It deliberately does
    not spray a large sequence of NRPN writes at connected hardware; the user
    can audition/inspect the result and explicitly push an edit buffer once
    that bulk-write protocol is verified.
*/
class RandomizerPanel final : public EditorSection
{
public:
    RandomizerPanel (const ParameterRegistry& registry, ProgramState& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    [[nodiscard]] int preferredHeightForWidth (int width) const override;

private:
    RandomizerSettings settingsFromUi() const;
    void applyRandomization();
    void restorePrevious();
    void chooseNewSeed();
    void updateStrengthLabel();

    const ParameterRegistry& registry;
    ProgramState& state;
    std::optional<IonProgram> previousProgram;

    juce::ToggleButton oscillators { "Oscillators + pre-filter mix" };
    juce::ToggleButton filters { "Filters + post-filter mix" };
    juce::ToggleButton envelopes { "Envelopes" };
    juce::ToggleButton modulation { "LFO / S&H / mod matrix / tracking" };
    juce::ToggleButton effects { "Effects" };
    juce::ToggleButton voiceOutput { "Voice + output" };

    juce::ToggleButton continuousValues { "Values / amounts" };
    juce::ToggleButton enumerations { "Types / routing" };
    juce::ToggleButton switches { "Switches" };

    juce::Slider strength;
    juce::Label strengthLabel;
    juce::Label seedLabel;
    juce::TextEditor seedEditor;
    juce::TextButton newSeed { "new seed" };
    juce::TextButton randomize { "RANDOMIZE" };
    juce::TextButton undo { "undo randomize" };
    juce::Label status;
};
}
