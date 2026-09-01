#pragma once

#include "MidiCaptureEvent.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <functional>
#include <memory>
#include <mutex>

namespace aim
{
class IonMidiService final : private juce::MidiInputCallback
{
public:
    using MessageHandler = std::function<void (const juce::MidiMessage&)>;
    using MonitorHandler = std::function<void (const MidiCaptureEvent&)>;

    IonMidiService() = default;
    ~IonMidiService() override;

    [[nodiscard]] juce::Array<juce::MidiDeviceInfo> availableInputs() const;
    [[nodiscard]] juce::Array<juce::MidiDeviceInfo> availableOutputs() const;

    juce::Result openInput (const juce::String& identifier);
    juce::Result openOutput (const juce::String& identifier);
    void closeInput();
    void closeOutput();

    [[nodiscard]] juce::String currentInputIdentifier() const;
    [[nodiscard]] juce::String currentOutputIdentifier() const;

    void sendNow (const juce::MidiMessage& message);
    void setMessageHandler (MessageHandler handler);
    void setMonitorHandler (MonitorHandler handler);

private:
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    void emitMonitorEvent (MidiDirection direction,
                           const juce::String& deviceIdentifier,
                           const juce::MidiMessage& message);

    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;
    std::mutex handlerMutex;
    MessageHandler messageHandler;
    MonitorHandler monitorHandler;
};
}
