#include "IonMidiService.h"

namespace aim
{
IonMidiService::~IonMidiService()
{
    closeInput();
    closeOutput();
}

juce::Array<juce::MidiDeviceInfo> IonMidiService::availableInputs() const
{
    return juce::MidiInput::getAvailableDevices();
}

juce::Array<juce::MidiDeviceInfo> IonMidiService::availableOutputs() const
{
    return juce::MidiOutput::getAvailableDevices();
}

juce::Result IonMidiService::openInput (const juce::String& identifier)
{
    closeInput();

    auto newInput = juce::MidiInput::openDevice (identifier, this);
    if (newInput == nullptr)
        return juce::Result::fail ("Could not open MIDI input: " + identifier);

    newInput->start();
    input = std::move (newInput);
    return juce::Result::ok();
}

juce::Result IonMidiService::openOutput (const juce::String& identifier)
{
    closeOutput();

    auto newOutput = juce::MidiOutput::openDevice (identifier);
    if (newOutput == nullptr)
        return juce::Result::fail ("Could not open MIDI output: " + identifier);

    output = std::move (newOutput);
    return juce::Result::ok();
}

void IonMidiService::closeInput()
{
    if (input != nullptr)
        input->stop();

    input.reset();
}

void IonMidiService::closeOutput()
{
    output.reset();
}

juce::String IonMidiService::currentInputIdentifier() const
{
    return input != nullptr ? input->getIdentifier() : juce::String{};
}

juce::String IonMidiService::currentOutputIdentifier() const
{
    return output != nullptr ? output->getIdentifier() : juce::String{};
}

void IonMidiService::sendNow (const juce::MidiMessage& message)
{
    if (output != nullptr)
        output->sendMessageNow (message);
}

void IonMidiService::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (messageHandler)
        messageHandler (message);
}
}
