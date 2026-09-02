#pragma once

#include "Core/BankJson.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramBank.h"
#include "Core/ProgramJson.h"
#include "Core/ProgramState.h"
#include "Core/ProgramDocumentTracker.h"
#include "Midi/IonProgramDecoder.h"
#include "Midi/IonProgramEncoder.h"
#include "Midi/IonSyxFileCodec.h"
#include "Midi/IonSysExCodec.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <optional>

namespace aim
{
/** Native program/bank librarian.

    JSON is the canonical editable format. SysEx import/export is intentionally
    template-preserving: a .syx can only be exported after a real candidate
    patch dump has supplied the unknown bytes AIM Editor cannot yet construct.
*/
class ProgramLibrarian final : public juce::Component,
                               private juce::ListBoxModel,
                               private ProgramState::Listener
{
public:
    ProgramLibrarian (const ParameterRegistry& registry,
                      ProgramState& programState,
                      ProgramDocumentTracker& documentTracker);
    ~ProgramLibrarian() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    [[nodiscard]] bool hasPatchTemplate() const noexcept;
    [[nodiscard]] bool hasUnsavedProgramChanges() const noexcept;
    [[nodiscard]] bool hasUnsavedBankChanges() const noexcept;
    [[nodiscard]] juce::String unsavedSummary() const;

    /** Save every dirty native JSON document, asking for a path when needed.
        completion(true) means all requested saves succeeded; false also covers
        a cancelled chooser so callers never continue a destructive action.
    */
    void saveUnsavedChanges (std::function<void (bool)> completion);

    std::function<void()> onClose;

private:
    int getNumRows() override { return ProgramBank::slotCount; }
    void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

    void parameterValueChanged (std::string_view, const juce::var&, ProgramChangeOrigin) override;
    void programReplaced (ProgramChangeOrigin) override;
    void programMetadataChanged (ProgramChangeOrigin) override;

    void syncMetadataFromState();
    void updateStatus();
    void newProgram();
    void storeCurrentInSelectedSlot();
    void loadSelectedSlot();
    void clearSelectedSlot();
    void copySelectedSlot();
    void pasteIntoSelectedSlot();
    void newBank();

    void importProgramJson();
    void exportProgramJson();
    void saveProgramJsonTo (const juce::File& file);
    bool writeProgramJsonTo (const juce::File& file);
    void saveUnsavedProgram (std::function<void (bool)> completion);
    void importBankJson();
    void exportBankJson();
    void saveBankJsonTo (const juce::File& file);
    bool writeBankJsonTo (const juce::File& file);
    void saveUnsavedBank (std::function<void (bool)> completion);
    void importSyx();
    void exportSyx();
    void exportBankSyx();

    void chooseFileToOpen (juce::String title,
                           juce::String wildcard,
                           std::function<void (const juce::File&)> completion);
    void chooseFileToSave (juce::String title,
                           juce::File suggested,
                           juce::String wildcard,
                           std::function<void (const juce::File&)> completion);

    static juce::String safeFilenameStem (juce::String name);
    void showError (const juce::String& message);
    void confirmDiscardProgramChanges (std::function<void()> action);
    void confirmDiscardBankChanges (std::function<void()> action);
    void confirmDiscardAllChanges (std::function<void()> action);

    const ParameterRegistry& registry;
    ProgramState& state;
    ProgramDocumentTracker& documentTracker;
    ProgramBank bank;
    ProgramBank cleanBankBaseline;
    juce::File programFile;
    juce::File bankFile;
    juce::Label title;
    juce::Label bankSummary;
    juce::Label status;
    juce::Label bankNameLabel;
    juce::Label hardwareBankLabel;
    juce::TextEditor bankName;
    juce::ComboBox hardwareBank;
    juce::Label nameLabel;
    juce::Label categoryLabel;
    juce::TextEditor programName;
    juce::TextEditor category;
    juce::ListBox slotList { "Programs", this };

    juce::TextButton newProgramButton { "New Program" };
    juce::TextButton storeButton { "Store Current" };
    juce::TextButton loadButton { "Load Slot" };
    juce::TextButton clearSlotButton { "Clear Slot" };
    juce::TextButton copySlotButton { "Copy Slot" };
    juce::TextButton pasteSlotButton { "Paste Slot" };
    juce::TextButton newBankButton { "New Bank" };

    juce::TextButton importProgramButton { "Import Program JSON" };
    juce::TextButton exportProgramButton { "Save Program JSON" };
    juce::TextButton importBankButton { "Import Bank JSON" };
    juce::TextButton exportBankButton { "Save Bank JSON" };
    juce::TextButton importSyxButton { "Import .syx" };
    juce::TextButton exportSyxButton { "Export .syx" };
    juce::TextButton exportBankSyxButton { "Export Bank .syx" };
    juce::TextButton closeButton { "Close" };

    std::optional<IonProgram> slotClipboard;
    std::unique_ptr<juce::FileChooser> fileChooser;
};
}
