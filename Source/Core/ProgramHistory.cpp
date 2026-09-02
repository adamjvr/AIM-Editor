#include "ProgramHistory.h"

#include <algorithm>

namespace aim
{
namespace
{
constexpr double coalesceWindowMs = 450.0;
}

ProgramHistory::ProgramHistory (ProgramState& stateToUse, std::size_t maximumStates)
    : state (stateToUse), maxStates (std::max<std::size_t> (2, maximumStates))
{
    states.push_back (state.snapshot());
    state.addListener (this);
}

ProgramHistory::~ProgramHistory()
{
    state.removeListener (this);
}

bool ProgramHistory::canUndo() const noexcept
{
    return ! states.empty() && currentIndex > 0;
}

bool ProgramHistory::canRedo() const noexcept
{
    return ! states.empty() && currentIndex + 1 < states.size();
}

bool ProgramHistory::undo()
{
    if (! canUndo())
        return false;

    navigating = true;
    --currentIndex;
    state.replaceProgram (states[currentIndex], ProgramChangeOrigin::internal);
    navigating = false;
    breakCoalescing();
    notifyAvailability();
    return true;
}

bool ProgramHistory::redo()
{
    if (! canRedo())
        return false;

    navigating = true;
    ++currentIndex;
    state.replaceProgram (states[currentIndex], ProgramChangeOrigin::internal);
    navigating = false;
    breakCoalescing();
    notifyAvailability();
    return true;
}

void ProgramHistory::resetBaseline()
{
    states.clear();
    states.push_back (state.snapshot());
    currentIndex = 0;
    breakCoalescing();
    notifyAvailability();
}

void ProgramHistory::parameterValueChanged (std::string_view id,
                                             const juce::var&,
                                             ProgramChangeOrigin origin)
{
    if (navigating)
        return;

    if (origin == ProgramChangeOrigin::interactive)
    {
        recordInteractiveSnapshot (ChangeKind::parameter, std::string (id));
        return;
    }

    handleNonInteractiveChange (origin);
}

void ProgramHistory::programReplaced (ProgramChangeOrigin origin)
{
    if (navigating)
        return;

    if (origin == ProgramChangeOrigin::interactive)
    {
        const auto snapshot = state.snapshot();

        // Some purpose-built editors expose a local "restore previous" action
        // (the Randomizer did this before global history existed). If that
        // action restores an adjacent history snapshot, interpret it as real
        // history navigation instead of appending a duplicate A -> B -> A
        // branch. This keeps local restoration and global Undo/Redo coherent.
        if (currentIndex > 0 && snapshot == states[currentIndex - 1])
        {
            --currentIndex;
            breakCoalescing();
            notifyAvailability();
            return;
        }
        if (currentIndex + 1 < states.size() && snapshot == states[currentIndex + 1])
        {
            ++currentIndex;
            breakCoalescing();
            notifyAvailability();
            return;
        }

        // Whole-program operations such as Randomizer should otherwise always
        // be one explicit history step rather than coalescing with a knob edit.
        recordInteractiveSnapshot (ChangeKind::program);
        breakCoalescing();
        return;
    }

    handleNonInteractiveChange (origin);
}

void ProgramHistory::programMetadataChanged (ProgramChangeOrigin origin)
{
    if (navigating)
        return;

    if (origin == ProgramChangeOrigin::interactive)
    {
        recordInteractiveSnapshot (ChangeKind::metadata, "metadata");
        return;
    }

    handleNonInteractiveChange (origin);
}

void ProgramHistory::recordInteractiveSnapshot (ChangeKind kind, std::string key)
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto canCoalesce = currentIndex + 1 == states.size()
                          && states.size() > 1
                          && kind != ChangeKind::program
                          && kind == lastKind
                          && key == lastKey
                          && now - lastChangeMs <= coalesceWindowMs;

    if (canCoalesce)
    {
        states[currentIndex] = state.snapshot();
    }
    else
    {
        if (currentIndex + 1 < states.size())
            states.erase (states.begin() + static_cast<std::ptrdiff_t> (currentIndex + 1), states.end());

        states.push_back (state.snapshot());
        currentIndex = states.size() - 1;

        if (states.size() > maxStates)
        {
            const auto overflow = states.size() - maxStates;
            states.erase (states.begin(), states.begin() + static_cast<std::ptrdiff_t> (overflow));
            currentIndex -= overflow;
        }
    }

    lastKind = kind;
    lastKey = std::move (key);
    lastChangeMs = now;
    notifyAvailability();
}

void ProgramHistory::handleNonInteractiveChange (ProgramChangeOrigin origin)
{
    // Captures/imports represent authoritative external state. Starting a new
    // baseline avoids undoing into the previous patch and, critically, avoids
    // implying that an editor undo has also changed the hardware.
    if (origin == ProgramChangeOrigin::import || origin == ProgramChangeOrigin::protocolInput)
        resetBaseline();
}

void ProgramHistory::notifyAvailability()
{
    if (onAvailabilityChanged)
        onAvailabilityChanged (canUndo(), canRedo());
}

void ProgramHistory::breakCoalescing() noexcept
{
    lastKind = ChangeKind::none;
    lastKey.clear();
    lastChangeMs = 0.0;
}
}
