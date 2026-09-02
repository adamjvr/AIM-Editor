#pragma once

#include "Core/ProgramState.h"

#include <functional>
#include <string>
#include <vector>

namespace aim
{
/** Bounded semantic undo/redo history for one ProgramState.

    Only deliberate editor changes are added to the history. Imported/captured
    hardware state becomes a new baseline instead of creating an undo path into
    an unrelated patch. Undo/redo applies ProgramChangeOrigin::internal so it
    can never echo back through the live NRPN transmitter.
*/
class ProgramHistory final : private ProgramState::Listener
{
public:
    explicit ProgramHistory (ProgramState& stateToUse, std::size_t maximumStates = 256);
    ~ProgramHistory() override;

    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return states.size(); }

    bool undo();
    bool redo();

    /** Treat the current program as a fresh baseline and discard history. */
    void resetBaseline();

    std::function<void (bool canUndo, bool canRedo)> onAvailabilityChanged;

private:
    enum class ChangeKind
    {
        none,
        parameter,
        metadata,
        program
    };

    void parameterValueChanged (std::string_view id,
                                const juce::var&,
                                ProgramChangeOrigin origin) override;
    void programReplaced (ProgramChangeOrigin origin) override;
    void programMetadataChanged (ProgramChangeOrigin origin) override;

    void recordInteractiveSnapshot (ChangeKind kind, std::string key = {});
    void handleNonInteractiveChange (ProgramChangeOrigin origin);
    void notifyAvailability();
    void breakCoalescing() noexcept;

    ProgramState& state;
    const std::size_t maxStates;
    std::vector<IonProgram> states;
    std::size_t currentIndex = 0;
    bool navigating = false;

    ChangeKind lastKind = ChangeKind::none;
    std::string lastKey;
    double lastChangeMs = 0.0;
};
}
