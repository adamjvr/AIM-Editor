#include "HardwareTransferPanel.h"
#include <initializer_list>

namespace aim
{
HardwareTransferPanel::HardwareTransferPanel (IonMidiService& midiService,
                                              const ParameterRegistry& parameterRegistry,
                                              ProgramState& programState)
    : midi (midiService), registry (parameterRegistry), state (programState)
{
    title.setText ("Alesis Ion Hardware Transfer", juce::dontSendNotification);
    title.setFont (juce::FontOptions (18.0f));
    title.setColour (juce::Label::textColourId, juce::Colours::white);

    warning.setText ("Candidate protocol: patch requests and full-patch writes are based on the reverse-engineered Ion format. "
                     "Writes stay disabled until you explicitly arm them and the current program has a captured/imported 378-byte source template.",
                     juce::dontSendNotification);
    warning.setColour (juce::Label::textColourId, juce::Colour::fromRGB (235, 170, 85));
    warning.setJustificationType (juce::Justification::topLeft);

    bankLabel.setText ("Hardware Bank", juce::dontSendNotification);
    programLabel.setText ("Program", juce::dontSendNotification);
    editSlotLabel.setText ("Edit Buffer Slot", juce::dontSendNotification);
    for (auto* label : { &bankLabel, &programLabel, &editSlotLabel })
        label->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.68f));

    refreshBankSelector();
    bank.setSelectedId (1, juce::dontSendNotification);
    bank.onChange = [this] { refreshProgramSelector(); };
    refreshProgramSelector();

    editSlot.addItemList ({ "Edit 1", "Edit 2", "Edit 3", "Edit 4" }, 1);
    editSlot.setSelectedId (1, juce::dontSendNotification);

    requestPatch.onClick = [this] { requestSelectedPatch(); };
    requestBank.onClick = [this] { requestSelectedBank(); };
    armWrites.onClick = [this] { updateState(); };
    sendEditBuffer.onClick = [this] { sendCurrentToEditBuffer(); };
    closeButton.onClick = [this]
    {
        armWrites.setToggleState (false, juce::dontSendNotification);
        updateState();
        if (onClose)
            onClose();
    };

    sourceStatus.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    transferStatus.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    transferStatus.setJustificationType (juce::Justification::centredRight);

    for (auto* component : std::initializer_list<juce::Component*> { static_cast<juce::Component*> (&title), &warning, &sourceStatus, &transferStatus,
                             &bankLabel, &programLabel, &editSlotLabel, &bank, &program, &editSlot,
                             &requestPatch, &requestBank, &armWrites, &sendEditBuffer, &closeButton })
        addAndMakeVisible (component);

    state.addListener (this);
    updateState();
}

HardwareTransferPanel::~HardwareTransferPanel()
{
    state.removeListener (this);
}

void HardwareTransferPanel::setDeviceProfile (IonFamilyDevice deviceToUse)
{
    device = deviceToUse;
    armWrites.setToggleState (false, juce::dontSendNotification);
    refreshBankSelector();
    refreshProgramSelector();

    const auto& profile = profileFor (device);
    title.setText (juce::String (profile.displayName.data()) + " Hardware Transfer", juce::dontSendNotification);
    requestBank.setEnabled (profile.supportsBankDumpRequest);
    requestBank.setTooltip (profile.supportsBankDumpRequest
                                ? "Request the selected Ion bank using the candidate bank-dump request framing."
                                : "Disabled for Micron until bank-stream request behavior is independently verified.");

    editSlotLabel.setVisible (profile.supportsIonEditBuffers);
    editSlot.setVisible (profile.supportsIonEditBuffers);
    armWrites.setVisible (profile.supportsIonEditBuffers);
    sendEditBuffer.setVisible (profile.supportsIonEditBuffers);

    if (device == IonFamilyDevice::micron)
    {
        warning.setText ("Micron single-program requests are independently corroborated: request product ID 0x26, shared Program dumps 0x22/378 bytes. "
                         "Bank-dump requests and full Program writes stay disabled until Micron storage/destination semantics are hardware-verified.",
                         juce::dontSendNotification);
    }
    else
    {
        warning.setText ("Candidate Ion protocol: patch requests and full-patch writes use the shared 378-byte Program format. "
                         "Writes stay disabled until explicitly armed and the current program has a captured/imported source template.",
                         juce::dontSendNotification);
    }

    updateState();
    resized();
}

void HardwareTransferPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillAll();

    const auto panel = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour::fromRGB (38, 38, 38));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);
}

void HardwareTransferPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    constexpr int gap = 8;

    auto header = area.removeFromTop (32);
    title.setBounds (header.removeFromLeft (juce::jmax (200, header.getWidth() / 2)));
    transferStatus.setBounds (header);

    area.removeFromTop (6);
    warning.setBounds (area.removeFromTop (56));
    area.removeFromTop (8);

    auto selectors = area.removeFromTop (52);
    const auto selectorCount = device == IonFamilyDevice::ion ? 3 : 2;
    const auto selectorWidth = (selectors.getWidth() - gap * (selectorCount - 1)) / selectorCount;

    auto bankArea = selectors.removeFromLeft (selectorWidth);
    selectors.removeFromLeft (gap);
    auto programArea = device == IonFamilyDevice::ion ? selectors.removeFromLeft (selectorWidth) : selectors;
    juce::Rectangle<int> editArea;
    if (device == IonFamilyDevice::ion)
    {
        selectors.removeFromLeft (gap);
        editArea = selectors;
    }

    bankLabel.setBounds (bankArea.removeFromTop (18));
    bank.setBounds (bankArea.removeFromTop (30));
    programLabel.setBounds (programArea.removeFromTop (18));
    program.setBounds (programArea.removeFromTop (30));
    if (device == IonFamilyDevice::ion)
    {
        editSlotLabel.setBounds (editArea.removeFromTop (18));
        editSlot.setBounds (editArea.removeFromTop (30));
    }

    area.removeFromTop (10);
    sourceStatus.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);

    auto requestRow = area.removeFromTop (30);
    const auto half = (requestRow.getWidth() - gap) / 2;
    requestPatch.setBounds (requestRow.removeFromLeft (half));
    requestRow.removeFromLeft (gap);
    requestBank.setBounds (requestRow);

    area.removeFromTop (12);
    armWrites.setBounds (area.removeFromTop (28));
    area.removeFromTop (6);
    sendEditBuffer.setBounds (area.removeFromTop (34));

    closeButton.setBounds (area.removeFromBottom (32).removeFromRight (96));
}

void HardwareTransferPanel::refreshBankSelector()
{
    const auto previous = juce::jmax (0, bank.getSelectedItemIndex());
    bank.clear (juce::dontSendNotification);

    if (device == IonFamilyDevice::ion)
    {
        bank.addItemList ({ "Red", "Green", "Blue", "Yellow/User", "Edit" }, 1);
    }
    else
    {
        for (int index = 1; index <= micronDeviceProfile.requestBankCount; ++index)
            bank.addItem ("Bank " + juce::String (index), index);
    }

    bank.setSelectedItemIndex (juce::jmin (previous, juce::jmax (0, bank.getNumItems() - 1)),
                               juce::dontSendNotification);
}

void HardwareTransferPanel::refreshProgramSelector()
{
    const auto previous = juce::jmax (0, program.getSelectedItemIndex());
    program.clear (juce::dontSendNotification);

    const bool editBank = device == IonFamilyDevice::ion
                       && bank.getSelectedItemIndex() == static_cast<int> (IonBank::edit);
    const auto count = editBank ? 4 : 128;
    for (int slot = 0; slot < count; ++slot)
        program.addItem (juce::String (slot + 1) + "/" + juce::String (count), slot + 1);

    program.setSelectedItemIndex (juce::jmin (previous, count - 1), juce::dontSendNotification);
}

void HardwareTransferPanel::requestSelectedPatch()
{
    const auto bankIndex = bank.getSelectedItemIndex();
    const auto slot = program.getSelectedItemIndex();
    const auto& profile = profileFor (device);
    if (bankIndex < 0 || bankIndex >= profile.requestBankCount || slot < 0)
        return;

    midi.sendNow (IonSysExCodec::makeSinglePatchRequest (device, bankIndex, slot));
    transferStatus.setText (juce::String (profile.displayName.data()) + " patch request sent", juce::dontSendNotification);
}

void HardwareTransferPanel::requestSelectedBank()
{
    if (device != IonFamilyDevice::ion)
    {
        transferStatus.setText ("Micron bank request not enabled", juce::dontSendNotification);
        return;
    }

    const auto bankIndex = bank.getSelectedItemIndex();
    if (! IonSysExCodec::isValidBank (bankIndex))
        return;

    midi.sendNow (IonSysExCodec::makeBankRequest (static_cast<IonBank> (bankIndex)));
    transferStatus.setText ("Ion bank request sent", juce::dontSendNotification);
}

void HardwareTransferPanel::sendCurrentToEditBuffer()
{
    if (! armWrites.getToggleState() || ! hasPatchTemplate())
    {
        updateState();
        return;
    }

    IonPatchDump sourcePatch;
    sourcePatch.decodedBytes = state.program().getSourcePatchBytes();

    const auto slot = juce::jlimit (0, 3, editSlot.getSelectedItemIndex());
    if (const auto retarget = IonSysExCodec::retargetDecodedPatch (sourcePatch.decodedBytes, IonBank::edit, slot); retarget.failed())
    {
        transferStatus.setText (retarget.getErrorMessage(), juce::dontSendNotification);
        return;
    }

    juce::MidiMessage message;
    if (const auto encoded = IonProgramEncoder::encodeOntoTemplate (state.program(), registry, sourcePatch, message); encoded.failed())
    {
        transferStatus.setText (encoded.getErrorMessage(), juce::dontSendNotification);
        return;
    }

    midi.sendNow (message);
    transferStatus.setText ("sent to Edit " + juce::String (slot + 1) + " (candidate)", juce::dontSendNotification);

    // Make each full write a deliberate action. Re-arm for another send.
    armWrites.setToggleState (false, juce::dontSendNotification);
    updateState();
}

void HardwareTransferPanel::updateState()
{
    const auto templateReady = hasPatchTemplate();
    const auto ionWriteMode = device == IonFamilyDevice::ion;

    sourceStatus.setText (templateReady
                            ? "Source template: ready — unknown bytes/bits will be preserved"
                            : "Source template: none — capture/import a valid Ion-family Program before full writes",
                          juce::dontSendNotification);
    sourceStatus.setColour (juce::Label::textColourId,
                            templateReady ? juce::Colour::fromRGB (105, 210, 120)
                                          : juce::Colour::fromRGB (225, 165, 80));

    sendEditBuffer.setEnabled (ionWriteMode && templateReady && armWrites.getToggleState());
    armWrites.setEnabled (ionWriteMode && templateReady);
    if (! ionWriteMode || ! templateReady)
        armWrites.setToggleState (false, juce::dontSendNotification);
}

bool HardwareTransferPanel::hasPatchTemplate() const noexcept
{
    return state.program().getSourcePatchBytes().size() == IonSysExCodec::decodedSinglePatchSize;
}
}
