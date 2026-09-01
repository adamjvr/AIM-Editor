#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <functional>
#include <memory>

namespace aim
{
class IonMidiService final : private juce::MidiInputCallback
{
public:
    using MessageHandler = std::function<void (const juce::MidiMessage&)>;

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
    void setMessageHandler (MessageHandler handler) { messageHandler = std::move (handler); }

private:
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;
    MessageHandler messageHandler;
};
}
