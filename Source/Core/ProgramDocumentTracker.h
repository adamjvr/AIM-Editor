#pragma once

#include "Core/ProgramState.h"

#include <functional>

namespace aim
{
/** Tracks whether the semantic program differs from the last clean baseline.

    The tracker is deliberately transport-agnostic. Interactive edits and
    internal undo/redo navigation are compared against a saved/imported
    baseline. Imports and hardware captures establish a new clean baseline.
*/
class ProgramDocumentTracker final : private ProgramState::Listener
{
public:
    explicit ProgramDocumentTracker (ProgramState& stateToUse);
    ~ProgramDocumentTracker() override;

    [[nodiscard]] bool isDirty() const noexcept { return dirty; }
    [[nodiscard]] const IonProgram& cleanProgram() const noexcept { return cleanBaseline; }

    /** Mark the current semantic program as saved/clean. */
    void markClean();

    std::function<void (bool)> onDirtyChanged;

private:
    void parameterValueChanged (std::string_view,
                                const juce::var&,
                                ProgramChangeOrigin origin) override;
    void programReplaced (ProgramChangeOrigin origin) override;
    void programMetadataChanged (ProgramChangeOrigin origin) override;

    void establishExternalBaseline();
    void recomputeDirty();
    void setDirty (bool shouldBeDirty);

    ProgramState& state;
    IonProgram cleanBaseline;
    bool dirty = false;
};
}
