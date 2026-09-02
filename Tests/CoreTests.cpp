#include "Core/IonProgram.h"
#include "Core/ParameterRegistry.h"
#include "Core/ProgramJson.h"
#include "Core/ProgramBank.h"
#include "Core/BankJson.h"
#include "Core/ProgramState.h"
#include "Midi/IonProtocol.h"
#include "Midi/IonSysExCodec.h"
#include "Midi/IonProgramDecoder.h"
#include "Midi/IonProgramEncoder.h"
#include "Midi/IonParameterTransmitter.h"
#include "Midi/IonNrpnDecoder.h"
#include "Midi/MidiCaptureEvent.h"
#include "Midi/IonSyxFileCodec.h"

#include <BinaryData.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
int fail (const juce::String& message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    aim::ParameterRegistry registry;
    const auto parameterJson = juce::String::fromUTF8 (AIMBinaryData::parameters_json,
                                                        AIMBinaryData::parameters_jsonSize);
    if (const auto result = registry.loadFromJson (parameterJson); result.failed())
        return fail (result.getErrorMessage());

    const struct EnumBlob { const char* data; int size; } enumBlobs[] {
        { AIMBinaryData::modulation_sources_json, AIMBinaryData::modulation_sources_jsonSize },
        { AIMBinaryData::modulation_destinations_json, AIMBinaryData::modulation_destinations_jsonSize },
        { AIMBinaryData::filter_types_json, AIMBinaryData::filter_types_jsonSize },
    };
    for (const auto& blob : enumBlobs)
        if (const auto result = registry.loadEnumTableFromJson (juce::String::fromUTF8 (blob.data, blob.size)); result.failed())
            return fail ("Enum table load failed: " + result.getErrorMessage());

    if (registry.size() != 178)
        return fail ("Expected 178 initial parameter definitions, got " + juce::String (registry.size()));

    const auto* filter1FrequencyDefinition = registry.find ("filter1.frequency");
    if (filter1FrequencyDefinition == nullptr)
        return fail ("filter1.frequency missing from registry");
    if (! filter1FrequencyDefinition->nrpn.has_value() || *filter1FrequencyDefinition->nrpn != 44
        || filter1FrequencyDefinition->nrpnValueEncoding != "unsigned_14")
        return fail ("filter1.frequency candidate NRPN metadata missing from registry");

    if (registry.find ("mod_matrix.slot12.destination") == nullptr)
        return fail ("mod_matrix.slot12.destination missing from registry");

    const auto* filterTypeDefinition = registry.find ("filter1.type");
    if (filterTypeDefinition == nullptr || filterTypeDefinition->enumValues.size() != 21
        || ! filterTypeDefinition->enumValuesComplete
        || filterTypeDefinition->enumValues.front().id != "bypass"
        || filterTypeDefinition->enumValues.back().raw != 20)
        return fail ("filter1.type enum domain was not loaded from JSON");

    const auto* modSourceDefinition = registry.find ("mod_matrix.slot01.source");
    const auto* modDestinationDefinition = registry.find ("mod_matrix.slot01.destination");
    if (modSourceDefinition == nullptr || modSourceDefinition->enumValues.size() != 115
        || modSourceDefinition->enumValuesComplete
        || modSourceDefinition->enumValues.front().id != "note_on_velocity"
        || modDestinationDefinition == nullptr || modDestinationDefinition->enumValues.size() != 79
        || ! modDestinationDefinition->enumValuesComplete
        || modDestinationDefinition->enumValues.back().raw != 78)
        return fail ("External modulation enum tables were not attached to parameter definitions");

    aim::ProgramState state (registry);
    if (const auto* initialFilterType = state.valueFor ("filter1.type");
        initialFilterType == nullptr || static_cast<int> (*initialFilterType) != 0)
        return fail ("ProgramState did not choose a deterministic enum fallback");

    if (const auto result = state.setValue ("mod_matrix.slot01.source", 115); result.failed())
        return fail ("ProgramState rejected in-range value missing from an explicitly incomplete enum table");
    if (const auto* incompleteEnum = state.valueFor ("mod_matrix.slot01.source");
        incompleteEnum == nullptr || static_cast<int> (*incompleteEnum) != 115)
        return fail ("ProgramState destroyed unknown in-range value from incomplete enum evidence");

    if (const auto result = state.setValue ("filter1.type", 999); result.failed())
        return fail ("ProgramState unexpectedly rejected known parameter");
    if (const auto* clampedFilterType = state.valueFor ("filter1.type");
        clampedFilterType == nullptr || static_cast<int> (*clampedFilterType) != 0)
        return fail ("ProgramState did not reject an invalid enum raw value to a stable fallback");

    if (const auto result = state.setValue ("mod_matrix.slot01.level", 5000); result.failed())
        return fail ("ProgramState rejected numeric parameter");
    if (const auto* clampedLevel = state.valueFor ("mod_matrix.slot01.level");
        clampedLevel == nullptr || static_cast<int> (static_cast<double> (*clampedLevel)) != 1000)
        return fail ("ProgramState did not clamp numeric values to JSON raw_max");

    if (state.setValue ("does.not.exist", 1).wasOk())
        return fail ("ProgramState accepted an unknown parameter ID");

    aim::IonProgram source;
    source.setName ("Round Trip");
    source.setCategory ("Test");
    source.setParameter ("filter1.frequency", 101);
    source.setParameter ("voice.unison", 2);
    source.setParameter ("output.bypass", false);
    source.preserveUnknownByte (206, 37);

    const auto encoded = aim::ProgramJson::encode (source);

    aim::IonProgram decoded;
    if (const auto result = aim::ProgramJson::decode (encoded, decoded); result.failed())
        return fail (result.getErrorMessage());

    if (decoded.getName() != source.getName() || decoded.getCategory() != source.getCategory())
        return fail ("Program metadata did not round trip");

    const auto* frequency = decoded.getParameter ("filter1.frequency");
    if (frequency == nullptr || static_cast<int> (*frequency) != 101)
        return fail ("filter1.frequency did not round trip");

    const auto unknown = decoded.getUnknownBytes().find (206);
    if (unknown == decoded.getUnknownBytes().end() || unknown->second != 37)
        return fail ("Unknown byte preservation failed");

    state.setName ("Metadata Test", aim::ProgramChangeOrigin::import);
    state.setCategory ("Lead", aim::ProgramChangeOrigin::import);
    if (state.program().getName() != "Metadata Test" || state.program().getCategory() != "Lead")
        return fail ("ProgramState metadata setters failed");

    aim::ProgramBank nativeBank;
    nativeBank.setName ("Test Bank");
    nativeBank.setHardwareBank ("yellow");
    if (const auto result = nativeBank.setProgram (0, source); result.failed())
        return fail ("Could not store program in native bank");
    if (const auto result = nativeBank.setProgram (127, decoded); result.failed())
        return fail ("Could not store last program in native bank");
    if (nativeBank.occupiedCount() != 2 || nativeBank.programAt (0) == nullptr || nativeBank.programAt (127) == nullptr)
        return fail ("Native bank slot bookkeeping failed");
    if (nativeBank.setProgram (128, source).wasOk())
        return fail ("Native bank accepted out-of-range slot");

    const auto bankJson = aim::BankJson::encode (nativeBank);
    aim::ProgramBank decodedBank;
    if (const auto result = aim::BankJson::decode (bankJson, decodedBank); result.failed())
        return fail ("Native bank JSON round trip failed: " + result.getErrorMessage());
    if (decodedBank.getName() != "Test Bank" || decodedBank.getHardwareBank() != "yellow" || decodedBank.occupiedCount() != 2)
        return fail ("Native bank JSON metadata/slot round trip failed");
    const auto* bankProgram = decodedBank.programAt (127);
    if (bankProgram == nullptr || bankProgram->getName() != "Round Trip")
        return fail ("Native bank JSON lost program content");

    const auto nrpn = aim::IonProtocol::makeNrpnSequence (2, 0x0123, 0x0234);
    if (! nrpn[0].isController() || nrpn[0].getChannel() != 2
        || nrpn[0].getControllerNumber() != 99 || nrpn[0].getControllerValue() != 2)
        return fail ("NRPN MSB framing failed");

    if (nrpn[1].getControllerNumber() != 98 || nrpn[1].getControllerValue() != 35
        || nrpn[2].getControllerNumber() != 6 || nrpn[2].getControllerValue() != 4
        || nrpn[3].getControllerNumber() != 38 || nrpn[3].getControllerValue() != 52)
        return fail ("NRPN payload framing failed");

    if (aim::IonProtocol::encodeIonSigned14 (-100) != 16284
        || aim::IonProtocol::decodeIonSigned14 (16284) != -100
        || aim::IonProtocol::encodeIonSigned14 (100) != 100)
        return fail ("Ion signed-14 candidate encoding failed");

    const auto ionNegativeNrpn = aim::IonProtocol::makeIonNrpnSequence (1, 47, -100);
    if (ionNegativeNrpn[0].getControllerValue() != 0
        || ionNegativeNrpn[1].getControllerValue() != 47
        || ionNegativeNrpn[2].getControllerValue() != 127
        || ionNegativeNrpn[3].getControllerValue() != 28)
        return fail ("Ion signed NRPN framing failed");

    const auto liveFrequency = aim::IonParameterTransmitter::messagesForParameter (*filter1FrequencyDefinition,
                                                                                   juce::var (777),
                                                                                   3);
    if (! liveFrequency
        || (*liveFrequency)[0].getChannel() != 3
        || (*liveFrequency)[0].getControllerNumber() != 99
        || (*liveFrequency)[0].getControllerValue() != 0
        || (*liveFrequency)[1].getControllerNumber() != 98
        || (*liveFrequency)[1].getControllerValue() != 44
        || (*liveFrequency)[2].getControllerValue() != 6
        || (*liveFrequency)[3].getControllerValue() != 9)
        return fail ("Live unsigned candidate NRPN mapping failed");

    const auto* modLevelDefinition = registry.find ("mod_matrix.slot01.level");
    if (modLevelDefinition == nullptr)
        return fail ("mod_matrix.slot01.level missing from registry");
    const auto liveNegative = aim::IonParameterTransmitter::messagesForParameter (*modLevelDefinition,
                                                                                  juce::var (-100),
                                                                                  1);
    if (! liveNegative
        || (*liveNegative)[0].getControllerValue() != 1
        || (*liveNegative)[1].getControllerValue() != 54
        || (*liveNegative)[2].getControllerValue() != 127
        || (*liveNegative)[3].getControllerValue() != 28)
        return fail ("Live signed candidate NRPN mapping failed");

    aim::ParameterDefinition unmappedDefinition;
    unmappedDefinition.id = "test.unmapped";
    if (aim::IonParameterTransmitter::messagesForParameter (unmappedDefinition, juce::var (1), 1))
        return fail ("Live transmitter created MIDI for an unmapped parameter");

    aim::IonNrpnDecoder nrpnDecoder;
    std::optional<aim::DecodedNrpn> decodedNrpn;
    for (const auto& message : *liveNegative)
        if (const auto value = nrpnDecoder.push (message))
            decodedNrpn = value;
    if (! decodedNrpn
        || decodedNrpn->midiChannel != 1
        || decodedNrpn->parameter != 182
        || decodedNrpn->value14Bit != 16284
        || aim::IonProtocol::decodeIonSigned14 (decodedNrpn->value14Bit) != -100)
        return fail ("Incoming NRPN state machine did not reconstruct signed candidate value");

    nrpnDecoder.reset();
    if (nrpnDecoder.push (juce::MidiMessage::controllerEvent (1, 38, 42)))
        return fail ("NRPN decoder accepted Data Entry LSB without address/MSB state");

    // Candidate Ion patch request framing from the community SysEx spec.
    const auto request = aim::IonSysExCodec::makeSinglePatchRequest (aim::IonBank::yellow, 42);
    const std::vector<std::uint8_t> expectedRequestPayload { 0x00, 0x00, 0x0e, 0x22, 0x41, 0x03, 0x00, 0x2a };
    if (! request.isSysEx() || request.getSysExDataSize() != static_cast<int> (expectedRequestPayload.size())
        || ! std::equal (expectedRequestPayload.begin(), expectedRequestPayload.end(), request.getSysExData()))
        return fail ("Ion single-patch request framing failed");

    const auto editRequest = aim::IonSysExCodec::makeSinglePatchRequest (aim::IonBank::edit, 127);
    if (editRequest.getSysExData()[7] != 3)
        return fail ("Ion edit-bank patch request did not clamp slot to 0..3");

    // Known 7-of-8 bit arrangement: MSBs 1,0,1,0,1,0,1 become 0b1010101.
    const std::vector<std::uint8_t> sevenFullBytes { 0x80, 0x01, 0x82, 0x03, 0x84, 0x05, 0x86 };
    const auto eightMidiBytes = aim::IonSysExCodec::encode7Of8 (sevenFullBytes);
    const std::vector<std::uint8_t> expectedPacked { 0x55, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    if (eightMidiBytes != expectedPacked)
        return fail ("Ion 7-of-8 encoder bit arrangement failed");

    std::vector<std::uint8_t> unpacked;
    if (const auto result = aim::IonSysExCodec::decode7Of8 (eightMidiBytes, unpacked); result.failed() || unpacked != sevenFullBytes)
        return fail ("Ion 7-of-8 known-vector decode failed");

    std::vector<std::uint8_t> allByteValues;
    for (int value = 0; value <= 255; ++value)
        allByteValues.push_back (static_cast<std::uint8_t> (value));
    const auto allPacked = aim::IonSysExCodec::encode7Of8 (allByteValues);
    std::vector<std::uint8_t> allRoundTrip;
    if (const auto result = aim::IonSysExCodec::decode7Of8 (allPacked, allRoundTrip); result.failed())
        return fail ("Ion 7-of-8 exhaustive-byte decode failed: " + result.getErrorMessage());
    if (allRoundTrip.size() < allByteValues.size()
        || ! std::equal (allByteValues.begin(), allByteValues.end(), allRoundTrip.begin()))
        return fail ("Ion 7-of-8 exhaustive-byte round trip failed");
    for (std::size_t i = allByteValues.size(); i < allRoundTrip.size(); ++i)
        if (allRoundTrip[i] != 0)
            return fail ("Ion 7-of-8 final-group padding is not zero");

    // Build a synthetic but structurally valid candidate patch dump so the
    // parser/checksum logic is exercised without embedding a copyrighted patch.
    std::vector<std::uint8_t> synthetic (aim::IonSysExCodec::decodedSinglePatchSize, 0);
    synthetic[0] = 0x00;
    synthetic[1] = 0x0e;
    synthetic[2] = 0x22;
    synthetic[3] = 0x01;
    synthetic[4] = 0x03;
    synthetic[5] = 0x00;
    synthetic[6] = 0x07;
    const char tag[] = "Q01SYNTH";
    std::copy_n (reinterpret_cast<const std::uint8_t*> (tag), 8, synthetic.begin() + 7);
    const char version[] = "1.06";
    std::copy_n (reinterpret_cast<const std::uint8_t*> (version), 4, synthetic.begin() + 19);
    synthetic[51] = 0x00; synthetic[52] = 0x00; synthetic[53] = 0x01; synthetic[54] = 0x3b;
    const char patchName[] = "AIM Test";
    std::copy_n (reinterpret_cast<const std::uint8_t*> (patchName), 8, synthetic.begin() + 63);
    synthetic[108] = 73; // non-zero payload data participates in checksum
    synthetic[126] = 0x02; synthetic[127] = 0x00;

    const auto checksum = aim::IonSysExCodec::computePatchChecksumComplement (synthetic);
    synthetic[15] = static_cast<std::uint8_t> ((checksum >> 24) & 0xff);
    synthetic[16] = static_cast<std::uint8_t> ((checksum >> 16) & 0xff);
    synthetic[17] = static_cast<std::uint8_t> ((checksum >> 8) & 0xff);
    synthetic[18] = static_cast<std::uint8_t> (checksum & 0xff);
    if (! aim::IonSysExCodec::validatePatchChecksum (synthetic))
        return fail ("Ion synthetic patch checksum validation failed");

    const auto syntheticPacked = aim::IonSysExCodec::encode7Of8 (synthetic);
    const auto syntheticMessage = juce::MidiMessage::createSysExMessage (syntheticPacked.data(), static_cast<int> (syntheticPacked.size()));
    aim::IonPatchDump parsedPatch;
    if (const auto result = aim::IonSysExCodec::decodeSinglePatchDump (syntheticMessage, parsedPatch); result.failed())
        return fail ("Ion synthetic patch parser failed: " + result.getErrorMessage());
    if (! parsedPatch.checksumValid || parsedPatch.name != "AIM Test" || parsedPatch.bank != 3 || parsedPatch.slot != 7)
        return fail ("Ion synthetic patch parser produced incorrect metadata");

    aim::IonProgram decodedSyntheticProgram;
    if (const auto result = aim::IonProgramDecoder::decode (parsedPatch, registry, decodedSyntheticProgram); result.failed())
        return fail ("Ion candidate program decoder failed: " + result.getErrorMessage());
    const auto* decodedFilter = decodedSyntheticProgram.getParameter ("filter1.frequency");
    const auto* decodedOscLevel = decodedSyntheticProgram.getParameter ("pre_filter_mix.osc1.level");
    if (decodedFilter == nullptr || static_cast<int> (*decodedFilter) != 512
        || decodedOscLevel == nullptr || static_cast<int> (*decodedOscLevel) != 73)
        return fail ("Ion candidate program decoder did not map raw patch fields");
    if (decodedSyntheticProgram.getSourcePatchBytes() != parsedPatch.decodedBytes)
        return fail ("Ion candidate decoder did not preserve complete source patch bytes");

    const auto sourceBackedJson = aim::ProgramJson::encode (decodedSyntheticProgram);
    aim::IonProgram sourceBackedRoundTrip;
    if (const auto result = aim::ProgramJson::decode (sourceBackedJson, sourceBackedRoundTrip); result.failed())
        return fail ("Source-backed program JSON failed to decode: " + result.getErrorMessage());
    if (sourceBackedRoundTrip.getSourcePatchBytes() != parsedPatch.decodedBytes)
        return fail ("Program JSON did not preserve the 378-byte source patch template");

    // Candidate re-encoding overlays semantic values onto the original image
    // instead of constructing a blank patch. This must preserve bytes AIM
    // Editor does not understand while updating checksum and changed fields.
    parsedPatch.decodedBytes[375] = 0x5a; // intentionally unknown/padding evidence
    decodedSyntheticProgram.setName ("Edited Test");
    decodedSyntheticProgram.setParameter ("filter1.frequency", 777);

    juce::MidiMessage reencodedMessage;
    if (const auto result = aim::IonProgramEncoder::encodeOntoTemplate (decodedSyntheticProgram, registry, parsedPatch, reencodedMessage); result.failed())
        return fail ("Ion candidate program encoder failed: " + result.getErrorMessage());

    aim::IonPatchDump reencodedPatch;
    if (const auto result = aim::IonSysExCodec::decodeSinglePatchDump (reencodedMessage, reencodedPatch); result.failed())
        return fail ("Re-encoded Ion patch did not decode: " + result.getErrorMessage());
    if (! reencodedPatch.checksumValid || reencodedPatch.name != "Edited Test" || reencodedPatch.decodedBytes[375] != 0x5a)
        return fail ("Ion encoder did not preserve template bytes/name/checksum");

    aim::IonProgram redecodedProgram;
    if (const auto result = aim::IonProgramDecoder::decode (reencodedPatch, registry, redecodedProgram); result.failed())
        return fail ("Re-encoded candidate program could not be decoded");
    const auto* redecodedFilter = redecodedProgram.getParameter ("filter1.frequency");
    if (redecodedFilter == nullptr || static_cast<int> (*redecodedFilter) != 777)
        return fail ("Ion encode/decode semantic round trip changed filter1.frequency");

    const auto syxFileBytes = aim::IonSyxFileCodec::encodeFileBytes (reencodedMessage);
    if (syxFileBytes.getSize() != static_cast<std::size_t> (reencodedMessage.getSysExDataSize()) + 2u
        || static_cast<const std::uint8_t*> (syxFileBytes.getData())[0] != 0xf0
        || static_cast<const std::uint8_t*> (syxFileBytes.getData())[syxFileBytes.getSize() - 1] != 0xf7)
        return fail ("Standard .syx F0/F7 framing failed");

    juce::MidiMessage decodedSyxFile;
    if (const auto result = aim::IonSyxFileCodec::decodeFileBytes (syxFileBytes.getData(), syxFileBytes.getSize(), decodedSyxFile); result.failed())
        return fail ("Standard .syx decode failed: " + result.getErrorMessage());
    if (! decodedSyxFile.isSysEx()
        || decodedSyxFile.getSysExDataSize() != reencodedMessage.getSysExDataSize()
        || ! std::equal (reencodedMessage.getSysExData(),
                         reencodedMessage.getSysExData() + reencodedMessage.getSysExDataSize(),
                         decodedSyxFile.getSysExData()))
        return fail ("Standard .syx file round trip changed payload bytes");

    const std::vector<juce::MidiMessage> twoMessages { reencodedMessage, syntheticMessage };
    const auto multiSyxBytes = aim::IonSyxFileCodec::encodeFileBytes (twoMessages);
    std::vector<juce::MidiMessage> decodedMultiSyx;
    if (const auto result = aim::IonSyxFileCodec::decodeFileMessages (multiSyxBytes.getData(),
                                                                      multiSyxBytes.getSize(),
                                                                      decodedMultiSyx);
        result.failed() || decodedMultiSyx.size() != 2u)
        return fail ("Concatenated multi-message .syx round trip failed");

    const std::vector<std::uint8_t> testPayload { 0x00, 0x01, 0x7f };
    const auto testSysEx = aim::IonProtocol::makeSysExFromPayload (testPayload);
    const aim::MidiCaptureEvent capture { aim::MidiDirection::input, 123456789, "test-device", testSysEx };
    const auto captureJson = capture.toJson();
    const auto* captureObject = captureJson.getDynamicObject();
    if (captureObject == nullptr
        || captureObject->getProperty ("kind").toString() != "sysex"
        || static_cast<int> (captureObject->getProperty ("sysex_payload_size")) != 3)
        return fail ("MIDI capture JSON serialization failed");

    std::cout << "PASS: AIM Editor core tests\n";
    std::cout << "Loaded " << registry.size() << " JSON parameter definitions\n";
    return 0;
}
