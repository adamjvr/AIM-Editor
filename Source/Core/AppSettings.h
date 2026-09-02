#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace aim
{
/** Persistent, non-destructive editor session state.

    Deliberately excluded from persistence:
      - live NRPN enable state
      - any full-patch write arm state
      - transient undo/redo history

    Restoring a session may reconnect previously selected MIDI endpoints, but it
    must never send MIDI or arm a hardware write.
*/
struct SessionSnapshot
{
    int pageIndex = 0;
    int midiChannel = 1;
    int bankIndex = 0;
    int programIndex = 0;
    juce::String midiInputIdentifier;
    juce::String midiOutputIdentifier;
};

class AppSettings final
{
public:
    AppSettings();
    ~AppSettings();

    [[nodiscard]] SessionSnapshot loadSession();
    void saveSession (const SessionSnapshot& snapshot);
    void flush();

private:
    juce::ApplicationProperties properties;
};
}
