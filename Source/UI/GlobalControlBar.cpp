#include "GlobalControlBar.h"
#include <initializer_list>

namespace aim
{
GlobalControlBar::GlobalControlBar (IonMidiService& midiService)
    : midi (midiService)
{
    for (auto* component : std::initializer_list<juce::Component*> { static_cast<juce::Component*> (&deviceSelector), &midiInput, &midiBank, &programSelector, &midiOutput,
                             &midiChannel, &pageSelector, &requestPatch, &sysexTools, &updateEditBuffer,
                             &allNotesOff, &librarian, &undo, &redo, &liveEdit, &settings, &status })
        addAndMakeVisible (component);

    deviceSelector.addItem ("Ion", 1);
    deviceSelector.addItem ("Micron", 2);
    deviceSelector.setSelectedId (1, juce::dontSendNotification);
    deviceSelector.setTooltip ("Select the physical Ion-family hardware shell. Programs share the same 378-byte format; request/storage capabilities remain device-specific.");
    deviceSelector.onChange = [this]
    {
        applyDeviceProfile (selectedDevice(), true);
    };

    refreshBankSelector();
    midiBank.onChange = [this]
    {
        refreshProgramSelector();
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    };
    refreshProgramSelector();
    programSelector.onChange = [this]
    {
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    };

    for (int channel = 1; channel <= 16; ++channel)
        midiChannel.addItem (juce::String (channel), channel);
    midiChannel.setSelectedId (1, juce::dontSendNotification);
    midiChannel.onChange = [this]
    {
        if (onMidiChannelChanged)
            onMidiChannelChanged (juce::jmax (1, midiChannel.getSelectedId()));
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    };

    pageSelector.addItemList ({ "Front", "Dual 1", "Dual 2", "Randomizer", "Rear" }, 1);
    pageSelector.setSelectedId (1, juce::dontSendNotification);
    pageSelector.onChange = [this]
    {
        const auto index = pageSelector.getSelectedItemIndex();
        for (std::size_t i = 0; i < pageButtons.size(); ++i)
            if (pageButtons[i] != nullptr)
                pageButtons[i]->setToggleState (static_cast<int> (i) == index, juce::dontSendNotification);
        if (onPageChanged)
            onPageChanged (index);
    };

    const juce::StringArray pageNames { "Front", "Dual 1", "Dual 2", "Random", "Rear" };
    const juce::StringArray pageHints { "Front editor (Cmd+1)", "Dual 1 editor (Cmd+2)", "Dual 2 editor (Cmd+3)",
                                        "Randomizer (Cmd+4)", "Rear / modulation editor (Cmd+5)" };
    for (std::size_t i = 0; i < pageButtons.size(); ++i)
    {
        pageButtons[i] = std::make_unique<juce::TextButton> (pageNames[static_cast<int> (i)]);
        pageButtons[i]->setClickingTogglesState (true);
        pageButtons[i]->setRadioGroupId (0x41494d);
        pageButtons[i]->setToggleState (i == 0u, juce::dontSendNotification);
        pageButtons[i]->setTooltip (pageHints[static_cast<int> (i)]);
        pageButtons[i]->setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (43, 43, 43));
        pageButtons[i]->setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (164, 38, 38));
        pageButtons[i]->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.92f));
        pageButtons[i]->setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        pageButtons[i]->onClick = [this, i]
        {
            pageSelector.setSelectedItemIndex (static_cast<int> (i), juce::dontSendNotification);
            if (onPageChanged)
                onPageChanged (static_cast<int> (i));
        };
        addAndMakeVisible (*pageButtons[i]);
    }

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
    librarian.onClick = [this]
    {
        if (onLibrarianRequested)
            onLibrarianRequested();
    };
    undo.onClick = [this]
    {
        if (onUndoRequested)
            onUndoRequested();
    };
    redo.onClick = [this]
    {
        if (onRedoRequested)
            onRedoRequested();
    };
    updateEditBuffer.onClick = [this]
    {
        if (onHardwareToolsRequested)
            onHardwareToolsRequested();
    };
    settings.onClick = [this]
    {
        refreshMidiDevices();
        status.setText ("MIDI devices refreshed", juce::dontSendNotification);
    };

    requestPatch.setEnabled (true);
    requestPatch.setTooltip ("Request the selected Program using the active Ion-family device profile (Ion request ID 0x22; Micron request ID 0x26). Returned Program dumps use the shared 0x22/378-byte format.");
    updateEditBuffer.setEnabled (true);
    updateEditBuffer.setTooltip ("Open guarded candidate hardware-transfer tools. Full patch writes require a captured source template and explicit arming.");
    sysexTools.setTooltip ("Open the live MIDI/SysEx capture inspector");
    librarian.setTooltip ("Open native JSON program/bank librarian and template-preserving .syx file tools");
    liveEdit.setToggleState (false, juce::dontSendNotification);
    liveEdit.setTooltip ("Opt-in candidate live editing: interactive controls send mapped Ion-family NRPN messages to the selected MIDI output. Patch loads/imports never echo.");
    settings.setTooltip ("Re-scan JUCE MIDI input/output devices without dropping still-available selections");
    undo.setTooltip ("Undo the last semantic editor change. Undo never echoes MIDI back to hardware.");
    redo.setTooltip ("Redo the last semantic editor change. Redo never echoes MIDI back to hardware.");
    setHistoryAvailability (false, false);

    status.setText ("protocol: candidate", juce::dontSendNotification);
    status.setJustificationType (juce::Justification::centredRight);
    status.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 160, 70));
    status.setFont (juce::FontOptions (10.0f));

    refreshMidiDevices();
}

void GlobalControlBar::setHistoryAvailability (bool canUndo, bool canRedo)
{
    undo.setEnabled (canUndo);
    redo.setEnabled (canRedo);
}

IonFamilyDevice GlobalControlBar::selectedDevice() const noexcept
{
    return deviceSelector.getSelectedId() == 2 ? IonFamilyDevice::micron : IonFamilyDevice::ion;
}

void GlobalControlBar::applyDeviceProfile (IonFamilyDevice device, bool notify)
{
    const auto previousBank = juce::jmax (0, midiBank.getSelectedItemIndex());
    refreshBankSelector();
    midiBank.setSelectedItemIndex (juce::jmin (previousBank, juce::jmax (0, midiBank.getNumItems() - 1)),
                                   juce::dontSendNotification);
    refreshProgramSelector();

    const auto& profile = profileFor (device);
    status.setText (juce::String (profile.displayName.data()) + " profile", juce::dontSendNotification);
    updateEditBuffer.setTooltip (profile.supportsIonEditBuffers
                                     ? "Open guarded Ion hardware-transfer tools. Full patch writes require a captured source template and explicit arming."
                                     : "Open Micron hardware-transfer tools. Full Program writes remain disabled until Micron destination/storage semantics are verified on hardware.");

    if (notify)
    {
        if (onDeviceChanged)
            onDeviceChanged (device);
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    }
}

void GlobalControlBar::setSelectedPage (int pageIndex)
{
    if (! juce::isPositiveAndBelow (pageIndex, static_cast<int> (pageButtons.size())))
        return;

    pageSelector.setSelectedItemIndex (pageIndex, juce::dontSendNotification);
    for (std::size_t i = 0; i < pageButtons.size(); ++i)
        if (pageButtons[i] != nullptr)
            pageButtons[i]->setToggleState (static_cast<int> (i) == pageIndex, juce::dontSendNotification);
}

void GlobalControlBar::restoreSession (const SessionSnapshot& snapshot)
{
    // Restoring editor context is deliberately transport-safe. It may reopen
    // previously selected MIDI endpoints, but it never enables live NRPN and
    // never sends a patch request/write.
    liveEdit.setToggleState (false, juce::dontSendNotification);

    const auto restoredDevice = deviceFromId (snapshot.deviceProfileId.toStdString());
    deviceSelector.setSelectedId (restoredDevice == IonFamilyDevice::micron ? 2 : 1, juce::dontSendNotification);
    applyDeviceProfile (restoredDevice, false);

    const auto maxBankIndex = juce::jmax (0, midiBank.getNumItems() - 1);
    midiBank.setSelectedItemIndex (juce::jlimit (0, maxBankIndex, snapshot.bankIndex), juce::dontSendNotification);
    refreshProgramSelector();
    const auto maxProgramIndex = juce::jmax (0, programSelector.getNumItems() - 1);
    programSelector.setSelectedItemIndex (juce::jlimit (0, maxProgramIndex, snapshot.programIndex), juce::dontSendNotification);

    midiChannel.setSelectedId (juce::jlimit (1, 16, snapshot.midiChannel), juce::dontSendNotification);
    if (onMidiChannelChanged)
        onMidiChannelChanged (juce::jlimit (1, 16, snapshot.midiChannel));

    const auto restoreDevice = [] (juce::ComboBox& selector,
                                   const juce::StringArray& identifiers,
                                   const juce::String& wanted)
    {
        if (wanted.isEmpty())
        {
            selector.setSelectedId (1, juce::dontSendNotification);
            return false;
        }

        const auto index = identifiers.indexOf (wanted);
        if (index < 0)
        {
            selector.setSelectedId (1, juce::dontSendNotification);
            return false;
        }

        selector.setSelectedId (index + 2, juce::dontSendNotification);
        return true;
    };

    if (restoreDevice (midiInput, inputIdentifiers, snapshot.midiInputIdentifier))
        selectMidiInput();
    if (restoreDevice (midiOutput, outputIdentifiers, snapshot.midiOutputIdentifier))
        selectMidiOutput();
}

void GlobalControlBar::captureSession (SessionSnapshot& snapshot) const
{
    snapshot.midiChannel = juce::jlimit (1, 16, midiChannel.getSelectedId());
    snapshot.bankIndex = juce::jmax (0, midiBank.getSelectedItemIndex());
    snapshot.programIndex = juce::jmax (0, programSelector.getSelectedItemIndex());
    snapshot.deviceProfileId = juce::String (profileFor (selectedDevice()).id.data());
    snapshot.midiInputIdentifier = midi.currentInputIdentifier();
    snapshot.midiOutputIdentifier = midi.currentOutputIdentifier();
}

void GlobalControlBar::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour::fromRGB (30, 30, 30));
    g.fillRect (getLocalBounds());
    g.setColour (juce::Colours::black);
    g.drawLine (0.0f, 0.5f, static_cast<float> (getWidth()), 0.5f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.setFont (juce::FontOptions (9.5f));

    const auto drawSelectorHeader = [&g] (const juce::String& text, juce::Rectangle<int> controlBounds)
    {
        const auto header = juce::Rectangle<int> (controlBounds.getX(), controlBounds.getY() - 14,
                                                   controlBounds.getWidth(), 12);
        g.drawText (text, header, juce::Justification::centredLeft, false);
    };

    drawSelectorHeader ("DEVICE", deviceSelector.getBounds());
    drawSelectorHeader ("MIDI IN", midiInput.getBounds());
    drawSelectorHeader ("BANK", midiBank.getBounds());
    drawSelectorHeader ("PROGRAM", programSelector.getBounds());
    drawSelectorHeader ("MIDI OUT", midiOutput.getBounds());
    drawSelectorHeader ("CH", midiChannel.getBounds());
    drawSelectorHeader ("VIEW", pageTabsBounds.isEmpty() ? pageSelector.getBounds() : pageTabsBounds);
}

void GlobalControlBar::resized()
{
    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (17);

    constexpr int gap = 6;
    const int rowHeight = 28;

    const auto compactActions = getWidth() < 1180;
    requestPatch.setButtonText (compactActions ? "Request" : "request patch");
    sysexTools.setButtonText (compactActions ? "SysEx" : "sysex tools");
    librarian.setButtonText (compactActions ? "Library" : "librarian");
    updateEditBuffer.setButtonText (compactActions ? "Hardware" : "update edit buffer");
    allNotesOff.setButtonText (compactActions ? "Panic" : "all notes off");
    liveEdit.setButtonText (compactActions ? "Live" : "live NRPN");
    settings.setButtonText (compactActions ? "Refresh" : "refresh MIDI");

    auto selectorRow = area.removeFromTop (rowHeight);
    // Full page names are intentionally never ellipsized. At narrower widths
    // collapse to the page selector rather than recreating the old cryptic tabs.
    const auto useSegmentedPages = getWidth() >= 1180;

    if (useSegmentedPages)
    {
        const auto available = selectorRow.getWidth();
        const int deviceW = juce::jlimit (105, 140, available / 10);
        const int midiW = juce::jlimit (135, 190, available / 8);
        const int bankW = juce::jlimit (110, 155, available / 9);
        const int programW = juce::jlimit (100, 135, available / 10);
        const int channelW = 64;

        deviceSelector.setBounds (selectorRow.removeFromLeft (deviceW)); selectorRow.removeFromLeft (gap);
        midiInput.setBounds (selectorRow.removeFromLeft (midiW)); selectorRow.removeFromLeft (gap);
        midiBank.setBounds (selectorRow.removeFromLeft (bankW)); selectorRow.removeFromLeft (gap);
        programSelector.setBounds (selectorRow.removeFromLeft (programW)); selectorRow.removeFromLeft (gap);
        midiOutput.setBounds (selectorRow.removeFromLeft (midiW)); selectorRow.removeFromLeft (gap);
        midiChannel.setBounds (selectorRow.removeFromLeft (channelW)); selectorRow.removeFromLeft (gap);

        pageTabsBounds = selectorRow;
        pageSelector.setVisible (false);
        auto tabs = pageTabsBounds;
        constexpr int tabGap = 3;
        const auto tabWidth = juce::jmax (48, (tabs.getWidth() - tabGap * 4) / 5);
        for (auto& button : pageButtons)
        {
            button->setVisible (true);
            button->setBounds (tabs.removeFromLeft (tabWidth));
            tabs.removeFromLeft (tabGap);
        }
    }
    else
    {
        const int selectorWidth = juce::jmax (68, (selectorRow.getWidth() - gap * 6) / 7);
        deviceSelector.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        midiInput.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        midiBank.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        programSelector.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        midiOutput.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        midiChannel.setBounds (selectorRow.removeFromLeft (selectorWidth)); selectorRow.removeFromLeft (gap);
        pageTabsBounds = selectorRow;
        pageSelector.setVisible (true);
        pageSelector.setBounds (pageTabsBounds);
        for (auto& button : pageButtons)
            button->setVisible (false);
    }

    area.removeFromTop (getWidth() < 1000 ? 10 : 7);
    auto buttonRow = area.removeFromTop (rowHeight);

    const int statusWidth = juce::jlimit (110, 175, buttonRow.getWidth() / 6);
    status.setBounds (buttonRow.removeFromRight (statusWidth));
    buttonRow.removeFromRight (gap);

    const int buttonWidth = juce::jmax (54, (buttonRow.getWidth() - gap * 8) / 9);
    requestPatch.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    sysexTools.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    librarian.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    undo.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    redo.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    updateEditBuffer.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    allNotesOff.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    liveEdit.setBounds (buttonRow.removeFromLeft (buttonWidth)); buttonRow.removeFromLeft (gap);
    settings.setBounds (buttonRow);
}

void GlobalControlBar::refreshMidiDevices()
{
    // Preserve the actually-open JUCE device identifiers. Earlier versions
    // repopulated the ComboBoxes with "None" while leaving the MIDI service
    // connected, which made the UI lie about its transport state.
    const auto openInput = midi.currentInputIdentifier();
    const auto openOutput = midi.currentOutputIdentifier();

    midiInput.clear (juce::dontSendNotification);
    midiOutput.clear (juce::dontSendNotification);
    inputIdentifiers.clear();
    outputIdentifiers.clear();

    midiInput.addItem ("None", 1);
    midiOutput.addItem ("None", 1);

    int selectedInputId = 1;
    int id = 2;
    for (const auto& device : midi.availableInputs())
    {
        midiInput.addItem (device.name, id);
        inputIdentifiers.add (device.identifier);
        if (device.identifier == openInput)
            selectedInputId = id;
        ++id;
    }

    int selectedOutputId = 1;
    id = 2;
    for (const auto& device : midi.availableOutputs())
    {
        midiOutput.addItem (device.name, id);
        outputIdentifiers.add (device.identifier);
        if (device.identifier == openOutput)
            selectedOutputId = id;
        ++id;
    }

    midiInput.setSelectedId (selectedInputId, juce::dontSendNotification);
    midiOutput.setSelectedId (selectedOutputId, juce::dontSendNotification);

    // If a device disappeared, close the stale endpoint rather than retaining
    // a connection that the selectors can no longer represent.
    if (openInput.isNotEmpty() && selectedInputId == 1)
        midi.closeInput();
    if (openOutput.isNotEmpty() && selectedOutputId == 1)
        midi.closeOutput();
}


void GlobalControlBar::selectMidiInput()
{
    const auto index = midiInput.getSelectedItemIndex() - 1;
    if (index < 0)
    {
        midi.closeInput();
        if (onPersistentContextChanged)
            onPersistentContextChanged();
        return;
    }

    if (index < inputIdentifiers.size())
    {
        const auto result = midi.openInput (inputIdentifiers[index]);
        status.setText (result.wasOk() ? "MIDI input open" : result.getErrorMessage(), juce::dontSendNotification);
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    }
}

void GlobalControlBar::selectMidiOutput()
{
    const auto index = midiOutput.getSelectedItemIndex() - 1;
    if (index < 0)
    {
        midi.closeOutput();
        if (onPersistentContextChanged)
            onPersistentContextChanged();
        return;
    }

    if (index < outputIdentifiers.size())
    {
        const auto result = midi.openOutput (outputIdentifiers[index]);
        status.setText (result.wasOk() ? "MIDI output open" : result.getErrorMessage(), juce::dontSendNotification);
        if (onPersistentContextChanged)
            onPersistentContextChanged();
    }
}

void GlobalControlBar::sendAllNotesOff()
{
    for (int channel = 1; channel <= 16; ++channel)
        midi.sendNow (juce::MidiMessage::allNotesOff (channel));
}

void GlobalControlBar::refreshBankSelector()
{
    const auto previous = juce::jmax (0, midiBank.getSelectedItemIndex());
    midiBank.clear (juce::dontSendNotification);

    if (selectedDevice() == IonFamilyDevice::ion)
    {
        midiBank.addItemList ({ "Red", "Green", "Blue", "Yellow/User", "Edit" }, 1);
    }
    else
    {
        for (int bank = 1; bank <= micronDeviceProfile.requestBankCount; ++bank)
            midiBank.addItem ("Bank " + juce::String (bank), bank);
    }

    midiBank.setSelectedItemIndex (juce::jmin (previous, juce::jmax (0, midiBank.getNumItems() - 1)),
                                   juce::dontSendNotification);
}

void GlobalControlBar::refreshProgramSelector()
{
    const auto previous = juce::jmax (0, programSelector.getSelectedItemIndex());
    programSelector.clear (juce::dontSendNotification);

    const bool isIonEdit = selectedDevice() == IonFamilyDevice::ion
                        && midiBank.getSelectedItemIndex() == static_cast<int> (IonBank::edit);
    const int programCount = isIonEdit ? 4 : 128;

    for (int index = 0; index < programCount; ++index)
        programSelector.addItem (juce::String (index + 1) + "/" + juce::String (programCount), index + 1);

    programSelector.setSelectedItemIndex (juce::jmin (previous, programCount - 1), juce::dontSendNotification);
}

void GlobalControlBar::requestCurrentPatch()
{
    const auto bankIndex = midiBank.getSelectedItemIndex();
    const auto programIndex = programSelector.getSelectedItemIndex();
    if (bankIndex < 0 || programIndex < 0)
        return;

    const auto device = selectedDevice();
    const auto& profile = profileFor (device);
    if (bankIndex >= profile.requestBankCount)
        return;

    midi.sendNow (IonSysExCodec::makeSinglePatchRequest (device, bankIndex, programIndex));
    status.setText (juce::String (profile.displayName.data()) + " request sent", juce::dontSendNotification);
}
}
