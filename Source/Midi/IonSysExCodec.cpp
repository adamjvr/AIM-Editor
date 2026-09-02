#include "IonSysExCodec.h"

#include <algorithm>
#include <array>
#include <limits>

namespace aim
{
namespace
{
constexpr std::array<std::uint8_t, 4> requestPrefix { 0x00, 0x00, 0x0e, 0x22 };
constexpr std::array<std::uint8_t, 8> synthTag { 'Q', '0', '1', 'S', 'Y', 'N', 'T', 'H' };

std::vector<std::uint8_t> requestPayload (IonBank bank, bool multiple, int slot)
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve (8);
    bytes.insert (bytes.end(), requestPrefix.begin(), requestPrefix.end());
    bytes.push_back (IonSysExCodec::requestPatchOpcode);
    bytes.push_back (static_cast<std::uint8_t> (bank));
    bytes.push_back (multiple ? 0x01 : 0x00);
    bytes.push_back (static_cast<std::uint8_t> (slot));
    return bytes;
}
}

juce::var IonPatchDump::toJsonSummary() const
{
    auto* object = new juce::DynamicObject();
    object->setProperty ("format", "aim-editor.ion-patch-dump-summary");
    object->setProperty ("schema_version", 1);
    object->setProperty ("bank", bank);
    object->setProperty ("multiple", multiple);
    object->setProperty ("slot", slot);
    object->setProperty ("name", name);
    object->setProperty ("firmware_version", firmwareVersion);
    object->setProperty ("decoded_size", static_cast<juce::int64> (decodedBytes.size()));
    object->setProperty ("checksum_valid", checksumValid);
    object->setProperty ("stored_checksum", static_cast<juce::int64> (storedChecksum));
    object->setProperty ("expected_checksum", static_cast<juce::int64> (expectedChecksum));

    juce::Array<juce::var> raw;
    raw.ensureStorageAllocated (static_cast<int> (decodedBytes.size()));
    for (const auto byte : decodedBytes)
        raw.add (static_cast<int> (byte));
    object->setProperty ("decoded_bytes", raw);

    return juce::var (object);
}

juce::MidiMessage IonSysExCodec::makeSinglePatchRequest (IonBank bank, int slot)
{
    const auto bankValue = static_cast<int> (bank);
    if (! isValidBank (bankValue))
        bank = IonBank::red;

    const auto maxSlot = bank == IonBank::edit ? 3 : 127;
    slot = std::clamp (slot, 0, maxSlot);
    const auto payload = requestPayload (bank, false, slot);
    return juce::MidiMessage::createSysExMessage (payload.data(), static_cast<int> (payload.size()));
}

juce::MidiMessage IonSysExCodec::makeBankRequest (IonBank bank)
{
    if (! isValidBank (static_cast<int> (bank)))
        bank = IonBank::red;

    const auto payload = requestPayload (bank, true, 0);
    return juce::MidiMessage::createSysExMessage (payload.data(), static_cast<int> (payload.size()));
}

juce::Result IonSysExCodec::retargetDecodedPatch (std::vector<std::uint8_t>& decoded,
                                                   IonBank bank,
                                                   int slot,
                                                   bool multiple)
{
    if (decoded.size() != decodedSinglePatchSize)
        return juce::Result::fail ("Decoded patch must contain exactly 378 bytes");

    if (! isValidBank (static_cast<int> (bank)))
        return juce::Result::fail ("Invalid Ion bank");

    const auto maxSlot = bank == IonBank::edit ? 3 : 127;
    if (slot < 0 || slot > maxSlot)
        return juce::Result::fail ("Patch slot is outside the selected bank");

    // Candidate header fields from the same community document used by the
    // decoder/request codec. Payload bytes remain untouched.
    decoded[4] = static_cast<std::uint8_t> (bank);
    decoded[5] = multiple ? 0x01u : 0x00u;
    decoded[6] = static_cast<std::uint8_t> (slot);
    return juce::Result::ok();
}

std::vector<std::uint8_t> IonSysExCodec::encode7Of8 (const std::vector<std::uint8_t>& decoded)
{
    if (decoded.empty())
        return {};

    const auto groupCount = (decoded.size() + 6u) / 7u;
    std::vector<std::uint8_t> encoded;
    encoded.reserve (groupCount * 8u);

    for (std::size_t group = 0; group < groupCount; ++group)
    {
        std::uint8_t highBits = 0;
        std::array<std::uint8_t, 7> lowBits {};

        for (std::size_t i = 0; i < 7; ++i)
        {
            const auto sourceIndex = group * 7u + i;
            const auto value = sourceIndex < decoded.size() ? decoded[sourceIndex] : std::uint8_t { 0 };

            // The MSB of decoded byte 0 occupies bit 6 of the prefix, byte 1
            // occupies bit 5, ... byte 6 occupies bit 0.
            if ((value & 0x80u) != 0)
                highBits = static_cast<std::uint8_t> (highBits | (1u << (6u - i)));

            lowBits[i] = static_cast<std::uint8_t> (value & 0x7fu);
        }

        encoded.push_back (highBits);
        encoded.insert (encoded.end(), lowBits.begin(), lowBits.end());
    }

    return encoded;
}

juce::Result IonSysExCodec::decode7Of8 (const std::uint8_t* encoded,
                                         std::size_t encodedSize,
                                         std::vector<std::uint8_t>& decoded)
{
    decoded.clear();

    if (encodedSize == 0)
        return juce::Result::ok();

    if (encoded == nullptr)
        return juce::Result::fail ("7-of-8 input pointer is null");

    if ((encodedSize % 8u) != 0u)
        return juce::Result::fail ("7-of-8 payload size must be a multiple of 8 bytes");

    decoded.reserve ((encodedSize / 8u) * 7u);

    for (std::size_t offset = 0; offset < encodedSize; offset += 8u)
    {
        const auto highBits = encoded[offset];
        if ((highBits & 0x80u) != 0u)
            return juce::Result::fail ("7-of-8 prefix contains a MIDI status bit");

        for (std::size_t i = 0; i < 7u; ++i)
        {
            const auto low = encoded[offset + i + 1u];
            if ((low & 0x80u) != 0u)
                return juce::Result::fail ("7-of-8 data contains a MIDI status bit");

            const auto msb = static_cast<std::uint8_t> (((highBits >> (6u - i)) & 0x01u) << 7u);
            decoded.push_back (static_cast<std::uint8_t> (msb | low));
        }
    }

    return juce::Result::ok();
}

juce::Result IonSysExCodec::decodeSinglePatchDump (const juce::MidiMessage& message,
                                                    IonPatchDump& patch)
{
    patch = {};

    if (! message.isSysEx())
        return juce::Result::fail ("Message is not SysEx");

    const auto* payload = message.getSysExData();
    const auto payloadSize = static_cast<std::size_t> (message.getSysExDataSize());

    if (payloadSize != encodedSinglePatchPayloadSize)
        return juce::Result::fail ("Single patch SysEx payload must be 432 bytes (434 including F0/F7)");

    if (const auto result = decode7Of8 (payload, payloadSize, patch.decodedBytes); result.failed())
        return result;

    if (patch.decodedBytes.size() != decodedSinglePatchSize)
        return juce::Result::fail ("Decoded single patch must contain 378 bytes");

    const auto& bytes = patch.decodedBytes;

    if (bytes[0] != 0x00 || bytes[1] != 0x0e)
        return juce::Result::fail ("Patch dump does not contain the candidate Alesis manufacturer ID 00 0E");

    if (bytes[2] != productId)
        return juce::Result::fail ("Patch dump product ID is not 0x22");

    if (! std::equal (synthTag.begin(), synthTag.end(), bytes.begin() + 7))
        return juce::Result::fail ("Patch dump is missing Q01SYNTH tag");

    if (readU32BE (bytes.data() + 51) != patchDataSize)
        return juce::Result::fail ("Patch dump check-size field is not 315 bytes");

    patch.bank = bytes[4];
    patch.multiple = bytes[5] != 0;
    patch.slot = bytes[6];
    patch.name = readAscii (bytes, 63, 15, true);
    patch.firmwareVersion = readAscii (bytes, 19, 4, false);
    patch.storedChecksum = readU32BE (bytes.data() + 15);
    patch.expectedChecksum = computePatchChecksumComplement (bytes);
    patch.checksumValid = patch.storedChecksum == patch.expectedChecksum;

    return juce::Result::ok();
}

juce::Result IonSysExCodec::encodeSinglePatchDump (const std::vector<std::uint8_t>& decodedInput,
                                                    juce::MidiMessage& message)
{
    message = {};

    if (decodedInput.size() != decodedSinglePatchSize)
        return juce::Result::fail ("Decoded single patch must contain exactly 378 bytes");

    if (decodedInput[0] != 0x00 || decodedInput[1] != 0x0e || decodedInput[2] != productId)
        return juce::Result::fail ("Decoded patch does not contain candidate Ion manufacturer/product bytes");

    if (! std::equal (synthTag.begin(), synthTag.end(), decodedInput.begin() + 7))
        return juce::Result::fail ("Decoded patch is missing Q01SYNTH tag");

    if (readU32BE (decodedInput.data() + 51) != patchDataSize)
        return juce::Result::fail ("Decoded patch check-size field is not 315 bytes");

    auto decoded = decodedInput;
    const auto checksum = computePatchChecksumComplement (decoded);
    decoded[15] = static_cast<std::uint8_t> ((checksum >> 24u) & 0xffu);
    decoded[16] = static_cast<std::uint8_t> ((checksum >> 16u) & 0xffu);
    decoded[17] = static_cast<std::uint8_t> ((checksum >> 8u) & 0xffu);
    decoded[18] = static_cast<std::uint8_t> (checksum & 0xffu);

    const auto encoded = encode7Of8 (decoded);
    if (encoded.size() != encodedSinglePatchPayloadSize)
        return juce::Result::fail ("Encoded candidate patch did not produce 432 transport bytes");

    message = juce::MidiMessage::createSysExMessage (encoded.data(), static_cast<int> (encoded.size()));
    return juce::Result::ok();
}

std::uint32_t IonSysExCodec::computePatchChecksumComplement (const std::vector<std::uint8_t>& decoded)
{
    // 78 network-order uint32 values begin at offset 63 and cover bytes
    // 63..374. Offsets 375..377 are outside the checksum sum.
    constexpr std::size_t dwordCount = 78;
    constexpr std::size_t summedBytes = dwordCount * 4u;

    if (decoded.size() < patchDataOffset + summedBytes)
        return 0;

    std::uint32_t sum = 0;
    for (std::size_t offset = patchDataOffset; offset < patchDataOffset + summedBytes; offset += 4u)
        sum += readU32BE (decoded.data() + offset);

    return static_cast<std::uint32_t> (0u - sum);
}

bool IonSysExCodec::validatePatchChecksum (const std::vector<std::uint8_t>& decoded)
{
    if (decoded.size() < decodedSinglePatchSize)
        return false;

    return readU32BE (decoded.data() + 15) == computePatchChecksumComplement (decoded);
}

juce::String IonSysExCodec::bankName (IonBank bank)
{
    switch (bank)
    {
        case IonBank::red:    return "Red";
        case IonBank::green:  return "Green";
        case IonBank::blue:   return "Blue";
        case IonBank::yellow: return "Yellow/User";
        case IonBank::edit:   return "Edit";
    }

    return "Unknown";
}

bool IonSysExCodec::isValidBank (int value) noexcept
{
    return value >= static_cast<int> (IonBank::red) && value <= static_cast<int> (IonBank::edit);
}

std::uint32_t IonSysExCodec::readU32BE (const std::uint8_t* p) noexcept
{
    return (static_cast<std::uint32_t> (p[0]) << 24u)
         | (static_cast<std::uint32_t> (p[1]) << 16u)
         | (static_cast<std::uint32_t> (p[2]) << 8u)
         | static_cast<std::uint32_t> (p[3]);
}

juce::String IonSysExCodec::readAscii (const std::vector<std::uint8_t>& bytes,
                                       std::size_t offset,
                                       std::size_t length,
                                       bool stopAtNul)
{
    if (offset >= bytes.size())
        return {};

    const auto end = std::min (bytes.size(), offset + length);
    juce::String result;

    for (auto i = offset; i < end; ++i)
    {
        const auto c = bytes[i];
        if (stopAtNul && c == 0)
            break;

        if (c >= 0x20 && c <= 0x7e)
            result += juce::String::charToString (static_cast<juce::juce_wchar> (c));
        else if (! stopAtNul)
            result += "?";
    }

    return result;
}
}
