#include "GlobalControlBar.h"

namespace aim
{
GlobalControlBar::GlobalControlBar (IonMidiService& midiService)
    : midi (midiService)
{
    for (auto* component : { static_cast<juce::Component*> (&midiInput), &midiBank, &programSelector, &midiOutput,
                             &midiChannel, &pageSelector, &requestPatch, &sysexTools, &updateEditBuffer,
                             &allNotesOff, &liveEdit, &settings, &status })
        addAndMakeVisible (component);

    midiBank.addItemList ({ "Red", "Green", "Blue", "Yellow/User", "Edit" }, 1);
    midiBank.setSelectedId (1, juce::dontSendNotification);
    midiBank.onChange = [this] { refreshProgramSelector(); };
    refreshProgramSelector();


    for (int channel = 1; channel <= 16; ++channel)
        midiChannel.addItem (juce::String (channel), channel);
    midiChannel.setSelectedId (1, juce::dontSendNotification);
    midiChannel.onChange = [this]
    {
        if (onMidiChannelChanged)
            onMidiChannelChanged (juce::jmax (1, midiChannel.getSelectedId()));
    };

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
    liveEdit.onClick = [this]
    {
        const auto enabled = liveEdit.getToggleState();
        status.setText (enabled ? "LIVE NRPN (candidate)" : "protocol: candidate",
                        juce::dontSendNotification);
        status.setColour (juce::Label::textColourId,
                          enabled ? juce::Colour::fromRGB (235, 95, 75)
                                  : juce::Colour::fromRGB (220, 160, 70));
        if (onLiveEditingChanged)
            onLiveEditingChanged (enabled);
    };
    requestPatch.onClick = [this] { requestCurrentPatch(); };
    sysexTools.onClick = [this]
    {
        if (onSysExToolsRequested)
            onSysExToolsRequested();
    };

    requestPatch.setEnabled (true);
    requestPatch.setTooltip ("Send candidate Ion patch-request SysEx; capture and verify the response before treating the mapping as authoritative");
    updateEditBuffer.setEnabled (false);
    updateEditBuffer.setTooltip ("Enabled after the Ion edit-buffer protocol is verified");
    sysexTools.setTooltip ("Open the live MIDI/SysEx capture inspector");
    liveEdit.setToggleState (false, juce::dontSendNotification);
    liveEdit.setTooltip ("Opt-in candidate live editing: interactive controls send mapped Ion NRPN messages to the selected MIDI output. Patch loads/imports never echo.");
    settings.setEnabled (false);

    status.setText ("protocol: candidate", juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centredRight);
    status.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 160, 70));
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
    g.drawText ("BANK", midiBank.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("PROGRAM", programSelector.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("MIDI OUT", midiOutput.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("CH", midiChannel.getBounds().translated (0, -15), juce::Justification::centredLeft);
    g.drawText ("PANEL", pageSelector.getBounds().translated (0, -15), juce::Justification::centredLeft);
}

void GlobalControlBar::resized()
{
    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (14);

    constexpr int gap = 6;
    const int rowHeight = 28;

    // Hardware/program context stays visible across every page. MIDI channel
    // is explicit because live NRPN editing is channel-sensitive.
    auto selectorRow = area.removeFromTop (rowHeight);
    const int selectorWidth = juce::jmax (78, (selectorRow.getWidth() - gap * 5) / 6);

    midiInput.setBounds (selectorRow.removeFromLeft (selectorWidth));
    selectorRow.removeFromLeft (gap);
    midiBank.setBounds (selectorRow.removeFromLeft (selectorWidth));
    selectorRow.removeFromLeft (gap);
    programSelector.setBounds (selectorRow.removeFromLeft (selectorWidth));
    selectorRow.removeFromLeft (gap);
    midiOutput.setBounds (selectorRow.removeFromLeft (selectorWidth));
    selectorRow.removeFromLeft (gap);
    midiChannel.setBounds (selectorRow.removeFromLeft (selectorWidth));
    selectorRow.removeFromLeft (gap);
    pageSelector.setBounds (selectorRow);

    area.removeFromTop (getWidth() < 1000 ? 10 : 7);
    auto buttonRow = area.removeFromTop (rowHeight);

    const int statusWidth = juce::jlimit (100, 150, buttonRow.getWidth() / 6);
    status.setBounds (buttonRow.removeFromRight (statusWidth));
    buttonRow.removeFromRight (gap);

    const int buttonWidth = juce::jmax (68, (buttonRow.getWidth() - gap * 5) / 6);
    requestPatch.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (gap);
    sysexTools.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (gap);
    updateEditBuffer.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (gap);
    allNotesOff.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (gap);
    liveEdit.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (gap);
    settings.setBounds (buttonRow);
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
    {
        const auto result = midi.openInput (inputIdentifiers[index]);
        status.setText (result.wasOk() ? "MIDI input open" : result.getErrorMessage(), juce::dontSendNotification);
    }
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
    {
        const auto result = midi.openOutput (outputIdentifiers[index]);
        status.setText (result.wasOk() ? "MIDI output open" : result.getErrorMessage(), juce::dontSendNotification);
    }
}

void GlobalControlBar::sendAllNotesOff()
{
    for (int channel = 1; channel <= 16; ++channel)
        midi.sendNow (juce::MidiMessage::allNotesOff (channel));
}

void GlobalControlBar::refreshProgramSelector()
{
    const auto previous = juce::jmax (0, programSelector.getSelectedItemIndex());
    programSelector.clear (juce::dontSendNotification);

    const bool isEdit = midiBank.getSelectedItemIndex() == static_cast<int> (IonBank::edit);
    const int programCount = isEdit ? 4 : 128;

    for (int index = 0; index < programCount; ++index)
        programSelector.addItem (juce::String (index + 1) + "/" + juce::String (programCount), index + 1);

    programSelector.setSelectedItemIndex (juce::jmin (previous, programCount - 1), juce::dontSendNotification);
}

void GlobalControlBar::requestCurrentPatch()
{
    const auto bankIndex = midiBank.getSelectedItemIndex();
    const auto programIndex = programSelector.getSelectedItemIndex();

    if (! IonSysExCodec::isValidBank (bankIndex) || programIndex < 0)
        return;

    const auto bank = static_cast<IonBank> (bankIndex);
    midi.sendNow (IonSysExCodec::makeSinglePatchRequest (bank, programIndex));

    status.setText ("request sent (candidate)", juce::dontSendNotification);
}
}
