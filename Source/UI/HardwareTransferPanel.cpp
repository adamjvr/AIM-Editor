#include "HardwareTransferPanel.h"
#include <initializer_list>

namespace aim
{
HardwareTransferPanel::HardwareTransferPanel (IonMidiService& midiService,
                                              const ParameterRegistry& parameterRegistry,
                                              ProgramState& programState)
    : midi (midiService), registry (parameterRegistry), state (programState)
{
    title.setText ("Ion Hardware Transfer", juce::dontSendNotification);
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

    bank.addItemList ({ "Red", "Green", "Blue", "Yellow/User", "Edit" }, 1);
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
    auto column = [&selectors] (int count)
    {
        constexpr int localGap = 8;
        return (selectors.getWidth() - localGap * (count - 1)) / count;
    };
    const auto selectorWidth = column (3);

    auto bankArea = selectors.removeFromLeft (selectorWidth);
    selectors.removeFromLeft (gap);
    auto programArea = selectors.removeFromLeft (selectorWidth);
    selectors.removeFromLeft (gap);
    auto editArea = selectors;

    bankLabel.setBounds (bankArea.removeFromTop (18));
    bank.setBounds (bankArea.removeFromTop (30));
    programLabel.setBounds (programArea.removeFromTop (18));
    program.setBounds (programArea.removeFromTop (30));
    editSlotLabel.setBounds (editArea.removeFromTop (18));
    editSlot.setBounds (editArea.removeFromTop (30));

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

void HardwareTransferPanel::refreshProgramSelector()
{
    const auto previous = juce::jmax (0, program.getSelectedItemIndex());
    program.clear (juce::dontSendNotification);

    const bool editBank = bank.getSelectedItemIndex() == static_cast<int> (IonBank::edit);
    const auto count = editBank ? 4 : 128;
    for (int slot = 0; slot < count; ++slot)
        program.addItem (juce::String (slot + 1) + "/" + juce::String (count), slot + 1);

    program.setSelectedItemIndex (juce::jmin (previous, count - 1), juce::dontSendNotification);
}

void HardwareTransferPanel::requestSelectedPatch()
{
    const auto bankIndex = bank.getSelectedItemIndex();
    const auto slot = program.getSelectedItemIndex();
    if (! IonSysExCodec::isValidBank (bankIndex) || slot < 0)
        return;

    midi.sendNow (IonSysExCodec::makeSinglePatchRequest (static_cast<IonBank> (bankIndex), slot));
    transferStatus.setText ("patch request sent", juce::dontSendNotification);
}

void HardwareTransferPanel::requestSelectedBank()
{
    const auto bankIndex = bank.getSelectedItemIndex();
    if (! IonSysExCodec::isValidBank (bankIndex))
        return;

    midi.sendNow (IonSysExCodec::makeBankRequest (static_cast<IonBank> (bankIndex)));
    transferStatus.setText ("bank request sent", juce::dontSendNotification);
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
    sourceStatus.setText (templateReady
                            ? "Source template: ready — unknown bytes/bits will be preserved"
                            : "Source template: none — capture/import a valid Ion patch before full writes",
                          juce::dontSendNotification);
    sourceStatus.setColour (juce::Label::textColourId,
                            templateReady ? juce::Colour::fromRGB (105, 210, 120)
                                          : juce::Colour::fromRGB (225, 165, 80));

    sendEditBuffer.setEnabled (templateReady && armWrites.getToggleState());
    if (! templateReady)
        armWrites.setToggleState (false, juce::dontSendNotification);
}

bool HardwareTransferPanel::hasPatchTemplate() const noexcept
{
    return state.program().getSourcePatchBytes().size() == IonSysExCodec::decodedSinglePatchSize;
}
}
