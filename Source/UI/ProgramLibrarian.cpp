#include "ProgramLibrarian.h"

namespace aim
{
namespace
{
juce::String hardwareBankId (int bank)
{
    switch (bank)
    {
        case 0: return "red";
        case 1: return "green";
        case 2: return "blue";
        case 3: return "yellow";
        case 4: return "edit";
        default: return {};
    }
}

int hardwareBankValue (const juce::String& bank)
{
    if (bank == "red") return 0;
    if (bank == "green") return 1;
    if (bank == "blue") return 2;
    if (bank == "yellow") return 3;
    if (bank == "edit") return 4;
    return -1;
}
}
ProgramLibrarian::ProgramLibrarian (const ParameterRegistry& registryToUse,
                                    ProgramState& programState,
                                    ProgramDocumentTracker& tracker)
    : registry (registryToUse), state (programState), documentTracker (tracker), cleanBankBaseline (bank)
{
    title.setText ("Program Librarian", juce::dontSendNotification);
    title.setFont (juce::FontOptions (20.0f));
    title.setColour (juce::Label::textColourId, juce::Colours::white);

    bankSummary.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.72f));
    bankSummary.setJustificationType (juce::Justification::centredRight);

    status.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 160, 70));
    status.setJustificationType (juce::Justification::centredLeft);

    bankNameLabel.setText ("BANK NAME", juce::dontSendNotification);
    hardwareBankLabel.setText ("HARDWARE BANK", juce::dontSendNotification);
    nameLabel.setText ("PROGRAM NAME", juce::dontSendNotification);
    categoryLabel.setText ("CATEGORY", juce::dontSendNotification);
    for (auto* label : { &bankNameLabel, &hardwareBankLabel, &nameLabel, &categoryLabel })
    {
        label->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        label->setFont (juce::FontOptions (10.0f));
    }

    bankName.setInputRestrictions (64);
    bankName.setText (bank.getName(), false);
    bankName.onTextChange = [this]
    {
        bank.setName (bankName.getText().trim().isNotEmpty() ? bankName.getText().trim() : "Untitled Bank");
        updateStatus();
    };

    hardwareBank.addItem ("Unspecified", 1);
    hardwareBank.addItem ("Red", 2);
    hardwareBank.addItem ("Green", 3);
    hardwareBank.addItem ("Blue", 4);
    hardwareBank.addItem ("Yellow", 5);
    hardwareBank.addItem ("Edit", 6);
    hardwareBank.setSelectedId (1, juce::dontSendNotification);
    hardwareBank.onChange = [this]
    {
        const juce::String ids[] { {}, "red", "green", "blue", "yellow", "edit" };
        const auto index = hardwareBank.getSelectedItemIndex();
        bank.setHardwareBank (juce::isPositiveAndBelow (index, 6) ? ids[index] : juce::String{});
        updateStatus();
    };

    programName.setInputRestrictions (15);
    programName.setTooltip ("Human-readable program name. Candidate .syx export writes this into the preserved patch template.");
    category.setInputRestrictions (32);

    programName.onTextChange = [this]
    {
        state.setName (programName.getText(), ProgramChangeOrigin::interactive);
    };
    category.onTextChange = [this]
    {
        state.setCategory (category.getText(), ProgramChangeOrigin::interactive);
    };

    slotList.setRowHeight (26);
    slotList.setMultipleSelectionEnabled (false);
    slotList.selectRow (0);
    slotList.setColour (juce::ListBox::backgroundColourId, juce::Colour::fromRGB (24, 24, 24));
    slotList.setColour (juce::ListBox::outlineColourId, juce::Colours::black);

    for (auto* component : { static_cast<juce::Component*> (&title), &bankSummary, &status,
                             &bankNameLabel, &hardwareBankLabel, &bankName, &hardwareBank,
                             &nameLabel, &categoryLabel, &programName, &category, &slotList,
                             &storeButton, &loadButton, &copySlotButton, &pasteSlotButton, &clearSlotButton, &newBankButton,
                             &importProgramButton, &exportProgramButton, &importBankButton,
                             &exportBankButton, &importSyxButton, &exportSyxButton, &exportBankSyxButton, &closeButton })
        addAndMakeVisible (component);

    storeButton.onClick = [this] { storeCurrentInSelectedSlot(); };
    loadButton.onClick = [this] { loadSelectedSlot(); };
    clearSlotButton.onClick = [this] { clearSelectedSlot(); };
    copySlotButton.onClick = [this] { copySelectedSlot(); };
    pasteSlotButton.onClick = [this] { pasteIntoSelectedSlot(); };
    pasteSlotButton.setEnabled (false);
    copySlotButton.setTooltip ("Copy the selected native bank slot, including preserved source-patch bytes.");
    pasteSlotButton.setTooltip ("Paste the copied native program into the selected slot without altering its source template.");
    newBankButton.onClick = [this] { newBank(); };
    importProgramButton.onClick = [this] { importProgramJson(); };
    exportProgramButton.onClick = [this] { exportProgramJson(); };
    importBankButton.onClick = [this] { importBankJson(); };
    exportBankButton.onClick = [this] { exportBankJson(); };
    importSyxButton.onClick = [this] { importSyx(); };
    exportSyxButton.onClick = [this] { exportSyx(); };
    exportBankSyxButton.onClick = [this] { exportBankSyx(); };
    closeButton.onClick = [this]
    {
        if (onClose)
            onClose();
    };

    exportSyxButton.setEnabled (false);
    exportSyxButton.setTooltip ("A .syx export requires a captured/imported patch template so unknown bytes are preserved safely.");
    exportBankSyxButton.setTooltip ("Export stored source-backed programs as concatenated standard SysEx messages. Every occupied slot must retain a patch template.");

    state.addListener (this);
    syncMetadataFromState();
    updateStatus();
}

ProgramLibrarian::~ProgramLibrarian()
{
    state.removeListener (this);
}

void ProgramLibrarian::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillAll();

    const auto panel = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour::fromRGB (38, 38, 38));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);
}

void ProgramLibrarian::resized()
{
    auto area = getLocalBounds().reduced (14);
    constexpr int gap = 7;

    auto header = area.removeFromTop (32);
    title.setBounds (header.removeFromLeft (juce::jmax (180, header.getWidth() / 2)));
    bankSummary.setBounds (header);

    area.removeFromTop (8);
    auto bankMetadata = area.removeFromTop (54);
    auto bankNameArea = bankMetadata.removeFromLeft (juce::jmax (180, bankMetadata.getWidth() * 2 / 3));
    bankMetadata.removeFromLeft (gap);
    bankNameLabel.setBounds (bankNameArea.removeFromTop (16));
    bankName.setBounds (bankNameArea.removeFromTop (30));
    hardwareBankLabel.setBounds (bankMetadata.removeFromTop (16));
    hardwareBank.setBounds (bankMetadata.removeFromTop (30));

    area.removeFromTop (6);
    auto metadata = area.removeFromTop (54);
    auto nameArea = metadata.removeFromLeft (juce::jmax (180, metadata.getWidth() * 2 / 3));
    metadata.removeFromLeft (gap);
    nameLabel.setBounds (nameArea.removeFromTop (16));
    programName.setBounds (nameArea.removeFromTop (30));
    categoryLabel.setBounds (metadata.removeFromTop (16));
    category.setBounds (metadata.removeFromTop (30));

    area.removeFromTop (8);
    auto footer = area.removeFromBottom (110);
    slotList.setBounds (area);

    footer.removeFromTop (8);
    auto row1 = footer.removeFromTop (28);
    auto row2 = footer.removeFromTop (28);
    footer.removeFromTop (7);
    auto row3 = footer.removeFromTop (28);

    auto layoutButtons = [] (juce::Rectangle<int> row, std::initializer_list<juce::Button*> buttons)
    {
        constexpr int localGap = 6;
        const auto count = static_cast<int> (buttons.size());
        const auto width = count > 0 ? (row.getWidth() - localGap * (count - 1)) / count : 0;
        for (auto* button : buttons)
        {
            button->setBounds (row.removeFromLeft (width));
            row.removeFromLeft (localGap);
        }
    };

    layoutButtons (row1, { &storeButton, &loadButton, &copySlotButton, &pasteSlotButton, &clearSlotButton, &newBankButton });
    layoutButtons (row2, { &importProgramButton, &exportProgramButton, &importBankButton, &exportBankButton });

    status.setBounds (row3.removeFromLeft (juce::jmax (150, row3.getWidth() / 4)));
    row3.removeFromLeft (gap);
    const auto actionWidth = juce::jmax (76, (row3.getWidth() - gap * 3) / 4);
    importSyxButton.setBounds (row3.removeFromLeft (actionWidth));
    row3.removeFromLeft (gap);
    exportSyxButton.setBounds (row3.removeFromLeft (actionWidth));
    row3.removeFromLeft (gap);
    exportBankSyxButton.setBounds (row3.removeFromLeft (actionWidth));
    row3.removeFromLeft (gap);
    closeButton.setBounds (row3);
}

void ProgramLibrarian::paintListBoxItem (int rowNumber,
                                         juce::Graphics& g,
                                         int width,
                                         int height,
                                         bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.setColour (juce::Colour::fromRGB (90, 25, 25));
        g.fillRect (0, 0, width, height);
    }
    else if ((rowNumber & 1) != 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.fillRect (0, 0, width, height);
    }

    g.setFont (juce::FontOptions (12.5f));
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawText (juce::String (rowNumber + 1).paddedLeft ('0', 3), 8, 0, 44, height, juce::Justification::centredLeft);

    if (const auto* program = bank.programAt (rowNumber))
    {
        g.setColour (juce::Colours::white);
        g.drawText (program->getName().isNotEmpty() ? program->getName() : "Untitled",
                    54, 0, juce::jmax (0, width - 190), height, juce::Justification::centredLeft, true);

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawText (program->getCategory(), juce::jmax (54, width - 130), 0, 120, height,
                    juce::Justification::centredRight, true);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawText ("— empty —", 54, 0, width - 64, height, juce::Justification::centredLeft);
    }
}

void ProgramLibrarian::selectedRowsChanged (int)
{
    const auto occupied = bank.isOccupied (slotList.getSelectedRow());
    loadButton.setEnabled (occupied);
    clearSlotButton.setEnabled (occupied);
    copySlotButton.setEnabled (occupied);
    pasteSlotButton.setEnabled (slotClipboard.has_value() && ProgramBank::isValidSlot (slotList.getSelectedRow()));
}

void ProgramLibrarian::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    slotList.selectRow (row);
    loadSelectedSlot();
}

void ProgramLibrarian::parameterValueChanged (std::string_view, const juce::var&, ProgramChangeOrigin)
{
    updateStatus();
}

void ProgramLibrarian::programReplaced (ProgramChangeOrigin)
{
    syncMetadataFromState();
    updateStatus();
}

void ProgramLibrarian::programMetadataChanged (ProgramChangeOrigin)
{
    syncMetadataFromState();
    updateStatus();
}

void ProgramLibrarian::syncMetadataFromState()
{
    programName.setText (state.program().getName(), false);
    category.setText (state.program().getCategory(), false);
}

void ProgramLibrarian::updateStatus()
{
    if (bankName.getText() != bank.getName())
        bankName.setText (bank.getName(), false);

    const auto bankId = bank.getHardwareBank();
    const int hardwareSelection = bankId == "red" ? 2
                                : bankId == "green" ? 3
                                : bankId == "blue" ? 4
                                : bankId == "yellow" ? 5
                                : bankId == "edit" ? 6 : 1;
    hardwareBank.setSelectedId (hardwareSelection, juce::dontSendNotification);

    const auto bankDirtyMarker = hasUnsavedBankChanges() ? " *" : "";
    bankSummary.setText (bank.getName() + bankDirtyMarker + "  •  " + juce::String (bank.occupiedCount()) + "/128 stored",
                         juce::dontSendNotification);

    const auto templateReady = hasPatchTemplate();
    juce::String statusText;
    if (hasUnsavedProgramChanges())
        statusText << "PROGRAM *  •  ";
    statusText << (templateReady ? "SysEx template: ready" : "SysEx template: none");
    status.setText (statusText, juce::dontSendNotification);
    status.setColour (juce::Label::textColourId,
                      hasUnsavedProgramChanges() ? juce::Colour::fromRGB (235, 185, 80)
                                                : (templateReady ? juce::Colour::fromRGB (100, 205, 115)
                                                                 : juce::Colour::fromRGB (220, 160, 70)));

    exportSyxButton.setEnabled (templateReady);
    exportBankSyxButton.setEnabled (bank.occupiedCount() > 0);
    selectedRowsChanged (slotList.getSelectedRow());
    slotList.updateContent();
    slotList.repaint();
}

bool ProgramLibrarian::hasPatchTemplate() const noexcept
{
    return state.program().getSourcePatchBytes().size() == IonSysExCodec::decodedSinglePatchSize;
}

bool ProgramLibrarian::hasUnsavedProgramChanges() const noexcept
{
    return documentTracker.isDirty();
}

bool ProgramLibrarian::hasUnsavedBankChanges() const noexcept
{
    return bank != cleanBankBaseline;
}

juce::String ProgramLibrarian::unsavedSummary() const
{
    juce::StringArray items;
    if (hasUnsavedProgramChanges())
        items.add ("current program");
    if (hasUnsavedBankChanges())
        items.add ("librarian bank");
    return items.joinIntoString (" and ");
}

void ProgramLibrarian::storeCurrentInSelectedSlot()
{
    const auto slot = slotList.getSelectedRow();
    if (! ProgramBank::isValidSlot (slot))
        return;

    auto snapshot = state.snapshot();
    if (snapshot.getName().isEmpty())
        snapshot.setName ("Program " + juce::String (slot + 1));

    if (const auto result = bank.setProgram (slot, snapshot); result.failed())
    {
        showError (result.getErrorMessage());
        return;
    }

    updateStatus();
    slotList.scrollToEnsureRowIsOnscreen (slot);
}

void ProgramLibrarian::loadSelectedSlot()
{
    const auto slot = slotList.getSelectedRow();
    if (bank.programAt (slot) == nullptr)
        return;

    confirmDiscardProgramChanges ([safe = juce::Component::SafePointer<ProgramLibrarian> (this), slot]
    {
        if (safe == nullptr)
            return;
        if (const auto* program = safe->bank.programAt (slot))
        {
            safe->programFile = {};
            safe->state.replaceProgram (*program, ProgramChangeOrigin::import);
            safe->updateStatus();
        }
    });
}

void ProgramLibrarian::clearSelectedSlot()
{
    if (const auto result = bank.clearProgram (slotList.getSelectedRow()); result.failed())
    {
        showError (result.getErrorMessage());
        return;
    }
    updateStatus();
}

void ProgramLibrarian::copySelectedSlot()
{
    const auto* program = bank.programAt (slotList.getSelectedRow());
    if (program == nullptr)
        return;

    slotClipboard = *program;
    pasteSlotButton.setEnabled (true);
    status.setText ("slot copied", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, juce::Colour::fromRGB (105, 190, 220));
}

void ProgramLibrarian::pasteIntoSelectedSlot()
{
    const auto slot = slotList.getSelectedRow();
    if (! slotClipboard || ! ProgramBank::isValidSlot (slot))
        return;

    if (const auto result = bank.setProgram (slot, *slotClipboard); result.failed())
    {
        showError (result.getErrorMessage());
        return;
    }

    updateStatus();
    slotList.scrollToEnsureRowIsOnscreen (slot);
}

void ProgramLibrarian::newBank()
{
    confirmDiscardBankChanges ([safe = juce::Component::SafePointer<ProgramLibrarian> (this)]
    {
        if (safe == nullptr)
            return;
        safe->bank.clear();
        safe->cleanBankBaseline = safe->bank;
        safe->bankFile = {};
        safe->updateStatus();
    });
}

void ProgramLibrarian::importProgramJson()
{
    confirmDiscardProgramChanges ([safe = juce::Component::SafePointer<ProgramLibrarian> (this)]
    {
        if (safe == nullptr)
            return;

        safe->chooseFileToOpen ("Import AIM Editor program JSON", "*.json",
                                [safe] (const juce::File& file)
                                {
                                    if (safe == nullptr)
                                        return;

                                    IonProgram program;
                                    if (const auto result = ProgramJson::decode (file.loadFileAsString(), program); result.failed())
                                    {
                                        safe->showError (result.getErrorMessage());
                                        return;
                                    }

                                    safe->programFile = file;
                                    safe->state.replaceProgram (program, ProgramChangeOrigin::import);
                                    safe->updateStatus();
                                });
    });
}

void ProgramLibrarian::exportProgramJson()
{
    const auto stem = safeFilenameStem (state.program().getName());
    const auto suggested = programFile != juce::File{} ? programFile
                         : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (stem + ".aimprogram.json");

    chooseFileToSave ("Save AIM Editor program JSON", suggested, "*.json",
                      [safe = juce::Component::SafePointer<ProgramLibrarian> (this)] (const juce::File& file)
                      {
                          if (safe != nullptr)
                              safe->saveProgramJsonTo (file);
                      });
}

void ProgramLibrarian::saveProgramJsonTo (const juce::File& file)
{
    if (! file.replaceWithText (ProgramJson::encode (state.program())))
    {
        showError ("Could not write program JSON.");
        return;
    }

    programFile = file;
    documentTracker.markClean();
    updateStatus();
}

void ProgramLibrarian::importBankJson()
{
    confirmDiscardBankChanges ([safe = juce::Component::SafePointer<ProgramLibrarian> (this)]
    {
        if (safe == nullptr)
            return;

        safe->chooseFileToOpen ("Import AIM Editor bank JSON", "*.json",
                                [safe] (const juce::File& file)
                                {
                                    if (safe == nullptr)
                                        return;

                                    ProgramBank decoded;
                                    if (const auto result = BankJson::decode (file.loadFileAsString(), decoded); result.failed())
                                    {
                                        safe->showError (result.getErrorMessage());
                                        return;
                                    }

                                    safe->bank = std::move (decoded);
                                    safe->cleanBankBaseline = safe->bank;
                                    safe->bankFile = file;
                                    safe->updateStatus();
                                });
    });
}

void ProgramLibrarian::exportBankJson()
{
    const auto stem = safeFilenameStem (bank.getName());
    const auto suggested = bankFile != juce::File{} ? bankFile
                         : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (stem + ".aimbank.json");

    chooseFileToSave ("Save AIM Editor bank JSON", suggested, "*.json",
                      [safe = juce::Component::SafePointer<ProgramLibrarian> (this)] (const juce::File& file)
                      {
                          if (safe != nullptr)
                              safe->saveBankJsonTo (file);
                      });
}

void ProgramLibrarian::saveBankJsonTo (const juce::File& file)
{
    if (! file.replaceWithText (BankJson::encode (bank)))
    {
        showError ("Could not write bank JSON.");
        return;
    }

    bankFile = file;
    cleanBankBaseline = bank;
    updateStatus();
}

void ProgramLibrarian::importSyx()
{
    confirmDiscardAllChanges ([safe = juce::Component::SafePointer<ProgramLibrarian> (this)]
    {
        if (safe == nullptr)
            return;

        safe->chooseFileToOpen ("Import Ion SysEx patch or bank", "*.syx",
                                [safe] (const juce::File& file)
                              {
                                  if (safe == nullptr)
                                      return;

                                  std::vector<juce::MidiMessage> messages;
                                  if (const auto result = IonSyxFileCodec::loadMessagesFromFile (file, messages); result.failed())
                                  {
                                      safe->showError (result.getErrorMessage());
                                      return;
                                  }

                                  ProgramBank importedBank;
                                  importedBank.setName (file.getFileNameWithoutExtension());
                                  int accepted = 0;
                                  int ignored = 0;
                                  int firstSlot = -1;
                                  int firstBank = -1;
                                  bool mixedBanks = false;

                                  for (const auto& message : messages)
                                  {
                                      IonPatchDump patch;
                                      if (IonSysExCodec::decodeSinglePatchDump (message, patch).failed() || ! patch.checksumValid)
                                      {
                                          ++ignored;
                                          continue;
                                      }

                                      IonProgram program;
                                      if (IonProgramDecoder::decode (patch, safe->registry, program).failed()
                                          || ! ProgramBank::isValidSlot (patch.slot))
                                      {
                                          ++ignored;
                                          continue;
                                      }

                                      if (firstBank < 0)
                                          firstBank = patch.bank;
                                      else if (patch.bank != firstBank)
                                          mixedBanks = true;

                                      if (firstSlot < 0)
                                          firstSlot = patch.slot;

                                      (void) importedBank.setProgram (patch.slot, program);
                                      ++accepted;
                                  }

                                  if (accepted == 0)
                                  {
                                      safe->showError ("No checksum-valid candidate Ion single-program dumps were found in this .syx file.");
                                      return;
                                  }

                                  if (! mixedBanks)
                                      importedBank.setHardwareBank (hardwareBankId (firstBank));

                                  // One program behaves like a normal patch import. A
                                  // concatenated .syx populates the native librarian by
                                  // hardware slot without assuming bank-stream framing.
                                  if (accepted == 1 && messages.size() == 1u)
                                  {
                                      const auto* program = importedBank.programAt (firstSlot);
                                      if (program != nullptr)
                                      {
                                          safe->programFile = {};
                                          safe->state.replaceProgram (*program, ProgramChangeOrigin::import);
                                          safe->updateStatus();
                                      }
                                  }
                                  else
                                  {
                                      safe->bank = std::move (importedBank);
                                      safe->cleanBankBaseline = safe->bank;
                                      safe->bankFile = {};
                                      safe->slotList.selectRow (juce::jmax (0, firstSlot));
                                      if (const auto* program = safe->bank.programAt (firstSlot))
                                      {
                                          safe->programFile = {};
                                          safe->state.replaceProgram (*program, ProgramChangeOrigin::import);
                                      }
                                      safe->updateStatus();

                                      if (ignored > 0)
                                          juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                                                  "AIM Editor",
                                                                                  "Imported " + juce::String (accepted)
                                                                                    + " candidate Ion patches; ignored "
                                                                                    + juce::String (ignored)
                                                                                    + " non-patch or invalid SysEx messages.");
                                  }
                              });
    });
}

void ProgramLibrarian::exportSyx()
{
    if (! hasPatchTemplate())
    {
        showError ("A .syx export requires an imported or captured patch template so unknown bytes can be preserved.");
        return;
    }

    IonPatchDump sourcePatch;
    sourcePatch.decodedBytes = state.program().getSourcePatchBytes();

    juce::MidiMessage encoded;
    if (const auto result = IonProgramEncoder::encodeOntoTemplate (state.program(), registry, sourcePatch, encoded); result.failed())
    {
        showError (result.getErrorMessage());
        return;
    }

    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (safeFilenameStem (state.program().getName()) + ".syx");

    chooseFileToSave ("Export template-preserving Ion SysEx patch", suggested, "*.syx",
                      [safe = juce::Component::SafePointer<ProgramLibrarian> (this), encoded] (const juce::File& file)
                      {
                          if (safe == nullptr)
                              return;

                          if (const auto result = IonSyxFileCodec::saveToFile (file, encoded); result.failed())
                              safe->showError (result.getErrorMessage());
                      });
}

void ProgramLibrarian::exportBankSyx()
{
    if (bank.occupiedCount() == 0)
    {
        showError ("The native librarian bank is empty.");
        return;
    }

    const auto targetBank = hardwareBankValue (bank.getHardwareBank());
    if (bank.getHardwareBank().isNotEmpty() && targetBank < 0)
    {
        showError ("The bank hardware_bank value is not a recognized Ion bank.");
        return;
    }

    std::vector<juce::MidiMessage> messages;
    messages.reserve (static_cast<std::size_t> (bank.occupiedCount()));

    for (const auto slot : bank.occupiedSlots())
    {
        const auto* program = bank.programAt (slot);
        if (program == nullptr)
            continue;

        if (program->getSourcePatchBytes().size() != IonSysExCodec::decodedSinglePatchSize)
        {
            showError ("Bank slot " + juce::String (slot + 1)
                       + " has no 378-byte source patch template. Export the bank as JSON, or import real .syx templates before hardware SysEx export.");
            return;
        }

        if (targetBank == static_cast<int> (IonBank::edit) && slot > 3)
        {
            showError ("The Ion Edit bank only has four program slots; native slot " + juce::String (slot + 1) + " cannot be exported there.");
            return;
        }

        IonPatchDump sourcePatch;
        sourcePatch.decodedBytes = program->getSourcePatchBytes();
        sourcePatch.decodedBytes[6] = static_cast<std::uint8_t> (slot);
        if (targetBank >= 0)
            sourcePatch.decodedBytes[4] = static_cast<std::uint8_t> (targetBank);

        juce::MidiMessage encoded;
        if (const auto result = IonProgramEncoder::encodeOntoTemplate (*program, registry, sourcePatch, encoded); result.failed())
        {
            showError ("Could not encode bank slot " + juce::String (slot + 1) + ": " + result.getErrorMessage());
            return;
        }
        messages.push_back (encoded);
    }

    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (safeFilenameStem (bank.getName()) + ".syx");

    chooseFileToSave ("Export template-preserving Ion SysEx bank", suggested, "*.syx",
                      [safe = juce::Component::SafePointer<ProgramLibrarian> (this), messages = std::move (messages)] (const juce::File& file)
                      {
                          if (safe == nullptr)
                              return;

                          if (const auto result = IonSyxFileCodec::saveToFile (file, messages); result.failed())
                              safe->showError (result.getErrorMessage());
                      });
}

void ProgramLibrarian::chooseFileToOpen (juce::String titleText,
                                         juce::String wildcard,
                                         std::function<void (const juce::File&)> completion)
{
    fileChooser = std::make_unique<juce::FileChooser> (std::move (titleText),
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                       std::move (wildcard),
                                                       true);

    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags,
                              [safe = juce::Component::SafePointer<ProgramLibrarian> (this), completion = std::move (completion)] (const juce::FileChooser& chooser)
                              {
                                  if (safe == nullptr)
                                      return;

                                  const auto file = chooser.getResult();
                                  if (file != juce::File{})
                                      completion (file);
                              });
}

void ProgramLibrarian::chooseFileToSave (juce::String titleText,
                                         juce::File suggested,
                                         juce::String wildcard,
                                         std::function<void (const juce::File&)> completion)
{
    fileChooser = std::make_unique<juce::FileChooser> (std::move (titleText), suggested, std::move (wildcard), true);

    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags,
                              [safe = juce::Component::SafePointer<ProgramLibrarian> (this), completion = std::move (completion)] (const juce::FileChooser& chooser)
                              {
                                  if (safe == nullptr)
                                      return;

                                  const auto file = chooser.getResult();
                                  if (file != juce::File{})
                                      completion (file);
                              });
}

juce::String ProgramLibrarian::safeFilenameStem (juce::String name)
{
    name = name.trim();
    if (name.isEmpty())
        name = "Untitled";

    const juce::String forbidden = "<>:\"/\\|?*";
    for (int i = 0; i < forbidden.length(); ++i)
        name = name.replaceCharacter (forbidden[i], '_');

    return name.substring (0, 64);
}

void ProgramLibrarian::confirmDiscardProgramChanges (std::function<void()> action)
{
    if (! hasUnsavedProgramChanges())
    {
        action();
        return;
    }

    const auto options = juce::MessageBoxOptions()
                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                           .withTitle ("Unsaved program changes")
                           .withMessage ("The current program has unsaved semantic edits. Continue and discard those edits?")
                           .withButton ("Discard & Continue")
                           .withButton ("Cancel")
                           .withAssociatedComponent (this);
    juce::AlertWindow::showAsync (options,
                                  [action = std::move (action)] (int buttonIndex) mutable
                                  {
                                      if (buttonIndex == 0)
                                          action();
                                  });
}

void ProgramLibrarian::confirmDiscardBankChanges (std::function<void()> action)
{
    if (! hasUnsavedBankChanges())
    {
        action();
        return;
    }

    const auto options = juce::MessageBoxOptions()
                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                           .withTitle ("Unsaved bank changes")
                           .withMessage ("The librarian bank has unsaved changes. Continue and discard those changes?")
                           .withButton ("Discard & Continue")
                           .withButton ("Cancel")
                           .withAssociatedComponent (this);
    juce::AlertWindow::showAsync (options,
                                  [action = std::move (action)] (int buttonIndex) mutable
                                  {
                                      if (buttonIndex == 0)
                                          action();
                                  });
}

void ProgramLibrarian::confirmDiscardAllChanges (std::function<void()> action)
{
    if (! hasUnsavedProgramChanges() && ! hasUnsavedBankChanges())
    {
        action();
        return;
    }

    const auto options = juce::MessageBoxOptions()
                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                           .withTitle ("Unsaved AIM Editor changes")
                           .withMessage ("Importing SysEx may replace the " + unsavedSummary()
                                         + ". Continue and discard those unsaved changes?")
                           .withButton ("Discard & Continue")
                           .withButton ("Cancel")
                           .withAssociatedComponent (this);
    juce::AlertWindow::showAsync (options,
                                  [action = std::move (action)] (int buttonIndex) mutable
                                  {
                                      if (buttonIndex == 0)
                                          action();
                                  });
}

void ProgramLibrarian::showError (const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "AIM Editor", message);
}
}
