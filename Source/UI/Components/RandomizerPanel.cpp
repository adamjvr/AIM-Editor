#include "RandomizerPanel.h"

#include <cstdint>

namespace aim
{
RandomizerPanel::RandomizerPanel (const ParameterRegistry& registryToUse, ProgramState& stateToUse)
    : registry (registryToUse), state (stateToUse)
{
    for (auto* component : { static_cast<juce::Component*> (&oscillators), &filters, &envelopes, &modulation,
                             &effects, &voiceOutput, &continuousValues, &enumerations, &switches,
                             &strength, &strengthLabel, &seedLabel, &seedEditor, &newSeed,
                             &randomize, &undo, &status })
        addAndMakeVisible (component);

    oscillators.setToggleState (true, juce::dontSendNotification);
    filters.setToggleState (true, juce::dontSendNotification);
    envelopes.setToggleState (true, juce::dontSendNotification);
    modulation.setToggleState (true, juce::dontSendNotification);
    effects.setToggleState (true, juce::dontSendNotification);
    voiceOutput.setToggleState (false, juce::dontSendNotification);

    continuousValues.setToggleState (true, juce::dontSendNotification);
    enumerations.setToggleState (true, juce::dontSendNotification);
    switches.setToggleState (false, juce::dontSendNotification);

    strength.setSliderStyle (juce::Slider::LinearHorizontal);
    strength.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    strength.setRange (0.0, 100.0, 1.0);
    strength.setValue (35.0, juce::dontSendNotification);
    strength.onValueChange = [this] { updateStrengthLabel(); };

    strengthLabel.setJustificationType (juce::Justification::centredRight);
    strengthLabel.setFont (juce::FontOptions (12.0f));
    updateStrengthLabel();

    seedLabel.setText ("Seed", juce::dontSendNotification);
    seedLabel.setFont (juce::FontOptions (12.0f));
    seedEditor.setInputRestrictions (20, "0123456789");
    seedEditor.setText ("0", false);
    seedEditor.setTooltip ("0 = choose a fresh seed. Non-zero seeds reproduce a randomization exactly.");

    randomize.onClick = [this] { applyRandomization(); };
    undo.onClick = [this] { restorePrevious(); };
    undo.setEnabled (false);
    newSeed.onClick = [this] { chooseNewSeed(); };

    status.setText ("Randomization changes semantic patch state only; it does not bulk-send MIDI.",
                    juce::dontSendNotification);
    status.setFont (juce::FontOptions (11.0f));
    status.setColour (juce::Label::textColourId, juce::Colours::black.withAlpha (0.62f));
    status.setJustificationType (juce::Justification::centredLeft);
}

void RandomizerPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour::fromRGB (190, 190, 187));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    auto header = getLocalBounds().removeFromTop (34).reduced (10, 0);
    g.setColour (juce::Colour::fromRGB (42, 42, 42));
    g.fillRect (getLocalBounds().removeFromTop (34));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
    g.drawText ("PATCH RANDOMIZER", header, juce::Justification::centredLeft);

    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (40);
    const auto split = area.getX() + area.getWidth() / 2;
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.drawText ("Sections", area.getX(), area.getY(), area.getWidth() / 2 - 8, 20, juce::Justification::centredLeft);
    g.drawText ("What changes", split + 8, area.getY(), area.getWidth() / 2 - 8, 20, juce::Justification::centredLeft);
}

void RandomizerPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (62);

    const auto wide = getWidth() >= 700;
    if (wide)
    {
        auto toggles = area.removeFromTop (116);
        auto left = toggles.removeFromLeft (toggles.getWidth() / 2).reduced (0, 2);
        auto right = toggles.reduced (10, 2);

        constexpr int h = 26;
        oscillators.setBounds (left.removeFromTop (h));
        filters.setBounds (left.removeFromTop (h));
        envelopes.setBounds (left.removeFromTop (h));
        modulation.setBounds (left.removeFromTop (h));

        effects.setBounds (right.removeFromTop (h));
        voiceOutput.setBounds (right.removeFromTop (h));
        continuousValues.setBounds (right.removeFromTop (h));
        enumerations.setBounds (right.removeFromTop (h));
        switches.setBounds (right.removeFromTop (h));
    }
    else
    {
        auto toggles = area.removeFromTop (250);
        for (auto* button : { &oscillators, &filters, &envelopes, &modulation, &effects, &voiceOutput,
                              &continuousValues, &enumerations, &switches })
            button->setBounds (toggles.removeFromTop (26));
    }

    area.removeFromTop (8);
    auto strengthRow = area.removeFromTop (34);
    strengthLabel.setBounds (strengthRow.removeFromRight (84));
    strength.setBounds (strengthRow);

    area.removeFromTop (8);
    auto seedRow = area.removeFromTop (30);
    seedLabel.setBounds (seedRow.removeFromLeft (46));
    newSeed.setBounds (seedRow.removeFromRight (92));
    seedRow.removeFromRight (6);
    seedEditor.setBounds (seedRow);

    area.removeFromTop (12);
    auto actions = area.removeFromTop (34);
    randomize.setBounds (actions.removeFromLeft (juce::jmax (130, actions.getWidth() * 2 / 3)).reduced (0, 1));
    actions.removeFromLeft (8);
    undo.setBounds (actions.reduced (0, 1));

    area.removeFromTop (6);
    status.setBounds (area.removeFromTop (32));
}

int RandomizerPanel::preferredHeightForWidth (int width) const
{
    return width >= 700 ? 330 : 455;
}

RandomizerSettings RandomizerPanel::settingsFromUi() const
{
    RandomizerSettings settings;
    settings.oscillators = oscillators.getToggleState();
    settings.filters = filters.getToggleState();
    settings.envelopes = envelopes.getToggleState();
    settings.modulation = modulation.getToggleState();
    settings.effects = effects.getToggleState();
    settings.voiceAndOutput = voiceOutput.getToggleState();
    settings.continuousValues = continuousValues.getToggleState();
    settings.enumerations = enumerations.getToggleState();
    settings.switches = switches.getToggleState();
    settings.strength = strength.getValue() / 100.0;
    settings.seed = static_cast<std::uint64_t> (seedEditor.getText().getLargeIntValue());
    return settings;
}

void RandomizerPanel::applyRandomization()
{
    previousProgram = state.snapshot();
    const auto result = RandomizerEngine::randomize (*previousProgram, registry, settingsFromUi());
    state.replaceProgram (result.program, ProgramChangeOrigin::interactive);
    undo.setEnabled (true);
    seedEditor.setText (juce::String (static_cast<juce::int64> (result.seedUsed)), false);
    status.setText ("Changed " + juce::String (static_cast<int> (result.changedParameterIds.size()))
                        + " mapped parameters. Seed " + juce::String (static_cast<juce::int64> (result.seedUsed)) + ".",
                    juce::dontSendNotification);
}

void RandomizerPanel::restorePrevious()
{
    if (! previousProgram)
        return;

    state.replaceProgram (*previousProgram, ProgramChangeOrigin::interactive);
    previousProgram.reset();
    undo.setEnabled (false);
    status.setText ("Previous semantic patch restored.", juce::dontSendNotification);
}

void RandomizerPanel::chooseNewSeed()
{
    auto seed = static_cast<std::uint64_t> (juce::Random::getSystemRandom().nextInt64());
    if (seed == 0)
        seed = 1;
    seedEditor.setText (juce::String (static_cast<juce::int64> (seed)), false);
}

void RandomizerPanel::updateStrengthLabel()
{
    strengthLabel.setText ("Strength  " + juce::String (static_cast<int> (strength.getValue())) + "%",
                           juce::dontSendNotification);
}
}
