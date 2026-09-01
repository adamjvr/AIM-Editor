#include "GlobalControlBar.h"

namespace aim
{
GlobalControlBar::GlobalControlBar (IonMidiService& midiService)
    : midi (midiService)
{
    for (auto* component : { static_cast<juce::Component*> (&midiInput), &programSelector, &midiOutput,
                             &pageSelector, &requestPatch, &sysexTools, &updateEditBuffer,
                             &allNotesOff, &settings, &status })
        addAndMakeVisible (component);

    programSelector.addItem ("1/128     Program 1", 1);
    programSelector.setSelectedId (1, juce::dontSendNotification);

    pageSelector.addItemList ({ "Front", "Dual 1", "Dual 2", "Randomizer", "Rear" }, 1);
    pageSelector.setSelectedId (1, juce::dontSendNotification);
    pageSelector.onChange = [this]
    {
        if (onPageChanged)
            onPageChanged (pageSelector.getSelectedItemIndex());
    };

    midiInput.onChange = [this] { selectMidiInput(); };
    midiOutput.onChange = [this] { selectMidiOutput(); };
    allNotesOff.onClick = [this] { sendAllNotesOff(); };

    requestPatch.setEnabled (false);
    requestPatch.setTooltip ("Enabled after the Ion patch-request SysEx protocol is verified");
    updateEditBuffer.setEnabled (false);
    updateEditBuffer.setTooltip ("Enabled after the Ion edit-buffer protocol is verified");
    sysexTools.setEnabled (false);
    sysexTools.setTooltip ("SysEx inspector is planned for the next implementation pass");
    settings.setEnabled (false);

    status.setText ("protocol: unmapped", juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centredRight);
    status.setColour (juce::Label::textColourId, juce::Colour::fromRGB (235, 80, 80));
    status.setFont (juce::FontOptions (10.0f));

    refreshMidiDevices();
}

void GlobalControlBar::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour::fromRGB (30, 30, 30));
    g.fillRect (getLocalBounds());
    g.setColour (juce::Colours::black);
    g.drawLine (0.0f, 0.5f, static_cast<float> (getWidth()), 0.5f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("MIDI IN", midiInput.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("PROGRAM", programSelector.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("MIDI OUT", midiOutput.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("PANEL", pageSelector.getBounds().translated (0, -15), juce::Justification::centredLeft);
}

void GlobalControlBar::resized()
{
    auto area = getLocalBounds().reduced (12, 8);

    if (getWidth() < 1000)
    {
        area.removeFromTop (14);
        auto firstRow = area.removeFromTop (30);
        area.removeFromTop (12);
        auto secondRow = area.removeFromTop (28);

        constexpr int gap = 6;
        const int firstWidth = (firstRow.getWidth() - gap * 3) / 4;
        midiInput.setBounds (firstRow.removeFromLeft (firstWidth));
        firstRow.removeFromLeft (gap);
        programSelector.setBounds (firstRow.removeFromLeft (firstWidth));
        firstRow.removeFromLeft (gap);
        midiOutput.setBounds (firstRow.removeFromLeft (firstWidth));
        firstRow.removeFromLeft (gap);
        pageSelector.setBounds (firstRow);

        const int statusWidth = 120;
        auto statusArea = secondRow.removeFromRight (statusWidth);
        status.setBounds (statusArea);
        secondRow.removeFromRight (gap);

        const int buttonWidth = juce::jmax (72, (secondRow.getWidth() - gap * 4) / 5);
        requestPatch.setBounds (secondRow.removeFromLeft (buttonWidth));
        secondRow.removeFromLeft (gap);
        sysexTools.setBounds (secondRow.removeFromLeft (buttonWidth));
        secondRow.removeFromLeft (gap);
        updateEditBuffer.setBounds (secondRow.removeFromLeft (buttonWidth));
        secondRow.removeFromLeft (gap);
        allNotesOff.setBounds (secondRow.removeFromLeft (buttonWidth));
        secondRow.removeFromLeft (gap);
        settings.setBounds (secondRow);
        return;
    }

    area.removeFromTop (14);
    const int gap = 7;
    const int h = 26;
    const int pageWidth = 130;
    const int statusWidth = 115;

    auto right = area.removeFromRight (pageWidth + statusWidth + gap);
    pageSelector.setBounds (right.removeFromLeft (pageWidth).withHeight (h));
    right.removeFromLeft (gap);
    status.setBounds (right.withHeight (h));

    const int available = area.getWidth();
    const int comboWidth = juce::jmax (130, (available - gap * 7) / 4);

    midiInput.setBounds (area.removeFromLeft (comboWidth).withHeight (h));
    area.removeFromLeft (gap);
    programSelector.setBounds (area.removeFromLeft (comboWidth).withHeight (h));
    area.removeFromLeft (gap);
    midiOutput.setBounds (area.removeFromLeft (comboWidth).withHeight (h));
    area.removeFromLeft (gap);

    auto buttons = area;
    const int buttonWidth = juce::jmax (74, buttons.getWidth() / 5);
    requestPatch.setBounds (buttons.removeFromLeft (buttonWidth).withHeight (h));
    buttons.removeFromLeft (3);
    sysexTools.setBounds (buttons.removeFromLeft (buttonWidth).withHeight (h));
    buttons.removeFromLeft (3);
    updateEditBuffer.setBounds (buttons.removeFromLeft (buttonWidth).withHeight (h));
    buttons.removeFromLeft (3);
    allNotesOff.setBounds (buttons.removeFromLeft (buttonWidth).withHeight (h));
    buttons.removeFromLeft (3);
    settings.setBounds (buttons.withHeight (h));
}

void GlobalControlBar::refreshMidiDevices()
{
    midiInput.clear (juce::dontSendNotification);
    midiOutput.clear (juce::dontSendNotification);
    inputIdentifiers.clear();
    outputIdentifiers.clear();

    midiInput.addItem ("None", 1);
    midiOutput.addItem ("None", 1);

    int id = 2;
    for (const auto& device : midi.availableInputs())
    {
        midiInput.addItem (device.name, id++);
        inputIdentifiers.add (device.identifier);
    }

    id = 2;
    for (const auto& device : midi.availableOutputs())
    {
        midiOutput.addItem (device.name, id++);
        outputIdentifiers.add (device.identifier);
    }

    midiInput.setSelectedId (1, juce::dontSendNotification);
    midiOutput.setSelectedId (1, juce::dontSendNotification);
}

void GlobalControlBar::selectMidiInput()
{
    const auto index = midiInput.getSelectedItemIndex() - 1;
    if (index < 0)
    {
        midi.closeInput();
        return;
    }

    if (index < inputIdentifiers.size())
        if (const auto result = midi.openInput (inputIdentifiers[index]); result.failed())
            status.setText (result.getErrorMessage(), juce::dontSendNotification);
}

void GlobalControlBar::selectMidiOutput()
{
    const auto index = midiOutput.getSelectedItemIndex() - 1;
    if (index < 0)
    {
        midi.closeOutput();
        return;
    }

    if (index < outputIdentifiers.size())
        if (const auto result = midi.openOutput (outputIdentifiers[index]); result.failed())
            status.setText (result.getErrorMessage(), juce::dontSendNotification);
}

void GlobalControlBar::sendAllNotesOff()
{
    for (int channel = 1; channel <= 16; ++channel)
        midi.sendNow (juce::MidiMessage::allNotesOff (channel));
}
}
