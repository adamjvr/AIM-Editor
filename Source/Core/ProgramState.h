#pragma once

#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"

#include <juce_core/juce_core.h>

#include <string_view>
#include <vector>

namespace aim
{
enum class ProgramChangeOrigin
{
    interactive,
    protocolInput,
    import,
    internal
};

/** Mutable editor state for one Ion program.

    ProgramState is deliberately independent of MIDI and UI. It owns the
    semantic IonProgram currently being edited, validates/clamps values against
    the JSON ParameterRegistry, and broadcasts changes to interested views.
*/
class ProgramState
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void parameterValueChanged (std::string_view id,
                                            const juce::var& value,
                                            ProgramChangeOrigin origin) = 0;
        virtual void programReplaced (ProgramChangeOrigin) {}
        virtual void programMetadataChanged (ProgramChangeOrigin) {}
    };

    explicit ProgramState (const ParameterRegistry& registryToUse);

    [[nodiscard]] const IonProgram& program() const noexcept { return currentProgram; }
    [[nodiscard]] IonProgram snapshot() const { return currentProgram; }

    [[nodiscard]] const juce::var* valueFor (std::string_view id) const;

    void setName (juce::String name, ProgramChangeOrigin origin = ProgramChangeOrigin::interactive);
    void setCategory (juce::String category, ProgramChangeOrigin origin = ProgramChangeOrigin::interactive);

    /** Set one semantic parameter. Unknown IDs are rejected. Numeric values are
        clamped to the known JSON domain when the domain is available.

        The origin is carried to listeners so protocol output can react only to
        deliberate interactive edits and never echo imported/captured state.
    */
    juce::Result setValue (std::string_view id,
                           juce::var value,
                           ProgramChangeOrigin origin = ProgramChangeOrigin::interactive);

    /** Replace the complete semantic program. Imported/captured values are
        preserved exactly so reverse-engineering evidence is not destructively
        normalized by today's incomplete candidate metadata.
    */
    void replaceProgram (const IonProgram& replacement,
                         ProgramChangeOrigin origin = ProgramChangeOrigin::import);

    /** Fill every parameter that has a known default, otherwise use a stable
        neutral fallback derived from its domain. This gives the UI deterministic
        values before a hardware dump has been received.
    */
    void resetToRegistryDefaults();

    void addListener (Listener* listener);
    void removeListener (Listener* listener);

private:
    [[nodiscard]] juce::var normalizedValue (const ParameterDefinition& definition,
                                              const juce::var& value) const;
    [[nodiscard]] juce::var fallbackValue (const ParameterDefinition& definition) const;

    const ParameterRegistry& registry;
    IonProgram currentProgram;
    std::vector<Listener*> listeners;
};
}
