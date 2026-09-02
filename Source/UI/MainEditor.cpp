#include "MainEditor.h"
#include "Midi/IonProtocol.h"

namespace aim
{
MainEditor::MainEditor (const ParameterRegistry& registryToUse, IonMidiService& midiService, AppSettings& settings)
    : registry (registryToUse),
      midi (midiService),
      appSettings (settings),
      programState (registryToUse),
      programHistory (programState),
      documentTracker (programState),
      parameterTransmitter (programState, registryToUse, midiService),
      controlBar (midiService),
      sysExInspector (midiService, registryToUse),
      programLibrarian (registryToUse, programState, documentTracker),
      hardwareTools (midiService, registryToUse, programState)
{
    pages[0] = std::make_unique<EditorPage> ("front", "Front", registryToUse, programState);
    pages[1] = std::make_unique<EditorPage> ("dual1", "Dual 1", registryToUse, programState);
    pages[2] = std::make_unique<EditorPage> ("dual2", "Dual 2", registryToUse, programState);
    pages[3] = std::make_unique<EditorPage> ("randomizer", "Randomizer", registryToUse, programState);
    pages[4] = std::make_unique<EditorPage> ("rear", "Rear", registryToUse, programState);

    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    viewport.setViewedComponent (pages[0].get(), false);
    addAndMakeVisible (viewport);

    controlBar.onPageChanged = [this] (int pageIndex) { showPage (pageIndex); };
    controlBar.onSysExToolsRequested = [this] { showSysExInspector(); };
    controlBar.onLibrarianRequested = [this] { showProgramLibrarian(); };
    controlBar.onHardwareToolsRequested = [this] { showHardwareTools(); };
    controlBar.onUndoRequested = [this] { (void) programHistory.undo(); };
    controlBar.onRedoRequested = [this] { (void) programHistory.redo(); };
    programHistory.onAvailabilityChanged = [safe = juce::Component::SafePointer<MainEditor> (this)] (bool canUndo, bool canRedo) mutable
    {
        if (safe != nullptr)
            safe->controlBar.setHistoryAvailability (canUndo, canRedo);
    };
    controlBar.setHistoryAvailability (programHistory.canUndo(), programHistory.canRedo());
    controlBar.onLiveEditingChanged = [this] (bool enabled)
    {
        parameterTransmitter.setEnabled (enabled);
    };
    controlBar.onMidiChannelChanged = [this] (int channel)
    {
        parameterTransmitter.setMidiChannel (channel);
    };
    controlBar.onPersistentContextChanged = [this] { persistSession(); };

    const auto restoredSession = appSettings.loadSession();
    controlBar.restoreSession (restoredSession);
    addAndMakeVisible (controlBar);
    showPage (restoredSession.pageIndex);

    setWantsKeyboardFocus (true);

    sysExInspector.onClose = [this] { hideSysExInspector(); };
    sysExInspector.onLoadCandidateProgram = [this] (const IonProgram& program, const IonPatchDump&)
    {
        programState.replaceProgram (program, ProgramChangeOrigin::protocolInput);
    };
    addChildComponent (sysExInspector);

    programLibrarian.onClose = [this] { hideProgramLibrarian(); };
    addChildComponent (programLibrarian);

    hardwareTools.onClose = [this] { hideHardwareTools(); };
    addChildComponent (hardwareTools);

    midi.setMessageHandler ([safe = juce::Component::SafePointer<MainEditor> (this)] (const juce::MidiMessage& message) mutable
    {
        if (safe == nullptr)
            return;

        const auto decoded = safe->nrpnDecoder.push (message);
        if (! decoded)
            return;

        juce::MessageManager::callAsync ([safe, value = *decoded]() mutable
        {
            if (safe != nullptr)
                safe->applyIncomingNrpn (value);
        });
    });
}

MainEditor::~MainEditor()
{
    persistSession();
    appSettings.flush();
    midi.setMessageHandler ({});
}

void MainEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (155, 155, 152));
}

void MainEditor::resized()
{
    auto area = getLocalBounds();

    const auto controlHeight = getWidth() < 1000 ? 126 : 102;
    controlBar.setBounds (area.removeFromBottom (controlHeight));
    viewport.setBounds (area);
    updateViewedPageSize();

    const auto margin = juce::jlimit (10, 32, juce::jmin (getWidth(), getHeight()) / 24);
    const auto overlayBounds = getLocalBounds().reduced (margin);
    sysExInspector.setBounds (overlayBounds);
    programLibrarian.setBounds (overlayBounds);
    hardwareTools.setBounds (overlayBounds);
}

bool MainEditor::keyPressed (const juce::KeyPress& key)
{
    const auto modifiers = key.getModifiers();
    const auto command = modifiers.isCommandDown();
    const auto code = key.getKeyCode();

    if (command && (code == 'z' || code == 'Z'))
    {
        if (modifiers.isShiftDown())
            (void) programHistory.redo();
        else
            (void) programHistory.undo();
        return true;
    }

    if (command && (code == 'y' || code == 'Y'))
    {
        (void) programHistory.redo();
        return true;
    }

    if (command && (code == 'l' || code == 'L'))
    {
        showProgramLibrarian();
        return true;
    }

    if (command && (code == 'i' || code == 'I'))
    {
        showSysExInspector();
        return true;
    }

    if (command && code >= '1' && code <= '5')
    {
        showPage (code - '1');
        return true;
    }

    if (code == juce::KeyPress::escapeKey)
    {
        if (sysExInspector.isVisible() || programLibrarian.isVisible() || hardwareTools.isVisible())
        {
            hideSysExInspector();
            hideProgramLibrarian();
            hideHardwareTools();
            return true;
        }
    }

    return false;
}

void MainEditor::showPage (int pageIndex)
{
    if (! juce::isPositiveAndBelow (pageIndex, static_cast<int> (pages.size())))
        return;

    currentPage = pageIndex;
    controlBar.setSelectedPage (pageIndex);
    viewport.setViewedComponent (pages[static_cast<std::size_t> (currentPage)].get(), false);
    viewport.setViewPosition (0, 0);
    updateViewedPageSize();
    persistSession();
}

void MainEditor::updateViewedPageSize()
{
    if (! juce::isPositiveAndBelow (currentPage, static_cast<int> (pages.size())))
        return;

    auto* page = pages[static_cast<std::size_t> (currentPage)].get();
    const auto width = juce::jmax (480, viewport.getWidth() - viewport.getScrollBarThickness());
    const auto height = juce::jmax (viewport.getHeight(), page->preferredHeightForWidth (width));
    page->setSize (width, height);
}

void MainEditor::showSysExInspector()
{
    programLibrarian.setVisible (false);
    hardwareTools.setVisible (false);
    sysExInspector.setVisible (true);
    sysExInspector.toFront (true);
}

void MainEditor::hideSysExInspector()
{
    sysExInspector.setVisible (false);
}

void MainEditor::showProgramLibrarian()
{
    sysExInspector.setVisible (false);
    hardwareTools.setVisible (false);
    programLibrarian.setVisible (true);
    programLibrarian.toFront (true);
}

void MainEditor::hideProgramLibrarian()
{
    programLibrarian.setVisible (false);
}

void MainEditor::showHardwareTools()
{
    sysExInspector.setVisible (false);
    programLibrarian.setVisible (false);
    hardwareTools.setVisible (true);
    hardwareTools.toFront (true);
}

void MainEditor::hideHardwareTools()
{
    hardwareTools.setVisible (false);
}

void MainEditor::persistSession()
{
    SessionSnapshot snapshot;
    snapshot.pageIndex = currentPage;
    controlBar.captureSession (snapshot);
    appSettings.saveSession (snapshot);
}

bool MainEditor::hasUnsavedChanges() const noexcept
{
    return programLibrarian.hasUnsavedProgramChanges()
        || programLibrarian.hasUnsavedBankChanges();
}

juce::String MainEditor::unsavedChangesDescription() const
{
    const auto summary = programLibrarian.unsavedSummary();
    return summary.isNotEmpty() ? summary : juce::String ("unsaved editor changes");
}

void MainEditor::saveUnsavedChanges (std::function<void (bool)> completion)
{
    programLibrarian.saveUnsavedChanges (std::move (completion));
}

void MainEditor::applyIncomingNrpn (const DecodedNrpn& decoded)
{
    // The selected channel is the editor's hardware context. Ignore other
    // channels so unrelated controllers cannot silently move Ion controls.
    if (decoded.midiChannel != parameterTransmitter.getMidiChannel())
        return;

    const auto* definition = registry.findByNrpn (decoded.parameter);
    if (definition == nullptr || definition->mappingStatus == MappingStatus::unmapped)
        return;

    int semanticValue = decoded.value14Bit;
    if (definition->nrpnValueEncoding == "signed_14_wrap")
        semanticValue = IonProtocol::decodeIonSigned14 (decoded.value14Bit);
    else if (definition->nrpnValueEncoding != "unsigned_14")
        return;

    (void) programState.setValue (definition->id, semanticValue, ProgramChangeOrigin::protocolInput);
}
}
