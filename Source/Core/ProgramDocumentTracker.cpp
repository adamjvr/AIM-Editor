#include "ProgramDocumentTracker.h"

namespace aim
{
ProgramDocumentTracker::ProgramDocumentTracker (ProgramState& stateToUse)
    : state (stateToUse), cleanBaseline (stateToUse.snapshot())
{
    state.addListener (this);
}

ProgramDocumentTracker::~ProgramDocumentTracker()
{
    state.removeListener (this);
}

void ProgramDocumentTracker::markClean()
{
    cleanBaseline = state.snapshot();
    setDirty (false);
}

void ProgramDocumentTracker::parameterValueChanged (std::string_view,
                                                     const juce::var&,
                                                     ProgramChangeOrigin origin)
{
    // Partial protocol/import updates are changes relative to the current
    // document baseline. Only a complete replaceProgram() establishes a new
    // authoritative baseline; otherwise a single incoming NRPN could
    // accidentally bless unrelated unsaved local edits as clean.
    (void) origin;
    recomputeDirty();
}

void ProgramDocumentTracker::programReplaced (ProgramChangeOrigin origin)
{
    if (origin == ProgramChangeOrigin::import || origin == ProgramChangeOrigin::protocolInput)
    {
        establishExternalBaseline();
        return;
    }

    recomputeDirty();
}

void ProgramDocumentTracker::programMetadataChanged (ProgramChangeOrigin origin)
{
    (void) origin;
    recomputeDirty();
}

void ProgramDocumentTracker::establishExternalBaseline()
{
    cleanBaseline = state.snapshot();
    setDirty (false);
}

void ProgramDocumentTracker::recomputeDirty()
{
    setDirty (state.program() != cleanBaseline);
}

void ProgramDocumentTracker::setDirty (bool shouldBeDirty)
{
    if (dirty == shouldBeDirty)
        return;

    dirty = shouldBeDirty;
    if (onDirtyChanged)
        onDirtyChanged (dirty);
}
}
