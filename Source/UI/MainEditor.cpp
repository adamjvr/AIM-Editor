#include "MainEditor.h"
#include "Midi/IonProtocol.h"

namespace aim
{
MainEditor::MainEditor (const ParameterRegistry& registryToUse, IonMidiService& midiService)
    : registry (registryToUse),
      midi (midiService),
      programState (registryToUse),
      parameterTransmitter (programState, registryToUse, midiService),
      controlBar (midiService),
      sysExInspector (midiService, registryToUse)
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
    controlBar.onLiveEditingChanged = [this] (bool enabled)
    {
        parameterTransmitter.setEnabled (enabled);
    };
    controlBar.onMidiChannelChanged = [this] (int channel)
    {
        parameterTransmitter.setMidiChannel (channel);
    };
    addAndMakeVisible (controlBar);

    sysExInspector.onClose = [this] { hideSysExInspector(); };
    sysExInspector.onLoadCandidateProgram = [this] (const IonProgram& program)
    {
        programState.replaceProgram (program, ProgramChangeOrigin::protocolInput);
    };
    addChildComponent (sysExInspector);

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
    sysExInspector.setBounds (getLocalBounds().reduced (margin));
}

void MainEditor::showPage (int pageIndex)
{
    if (! juce::isPositiveAndBelow (pageIndex, static_cast<int> (pages.size())))
        return;

    currentPage = pageIndex;
    viewport.setViewedComponent (pages[static_cast<std::size_t> (currentPage)].get(), false);
    viewport.setViewPosition (0, 0);
    updateViewedPageSize();
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
    sysExInspector.setVisible (true);
    sysExInspector.toFront (true);
}

void MainEditor::hideSysExInspector()
{
    sysExInspector.setVisible (false);
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
