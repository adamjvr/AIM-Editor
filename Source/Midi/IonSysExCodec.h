#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace aim
{
enum class IonBank : std::uint8_t
{
    red = 0,
    green = 1,
    blue = 2,
    yellow = 3,
    edit = 4
};

struct IonPatchDump
{
    std::vector<std::uint8_t> decodedBytes;

    int bank = -1;
    bool multiple = false;
    int slot = -1;
    juce::String name;
    juce::String firmwareVersion;

    std::uint32_t storedChecksum = 0;
    std::uint32_t expectedChecksum = 0;
    bool checksumValid = false;

    [[nodiscard]] juce::var toJsonSummary() const;
};

/** Candidate Ion/Micron patch SysEx codec.

    The framing and byte layout implemented here are based on the community
    reverse-engineered 2008 BEE document. They intentionally remain marked as
    candidate until verified against Ion hardware and/or independent captures.
*/
class IonSysExCodec
{
public:
    static constexpr std::uint8_t productId = 0x22;
    static constexpr std::uint8_t requestPatchOpcode = 0x41;
    static constexpr std::size_t encodedSinglePatchPayloadSize = 432;
    static constexpr std::size_t decodedSinglePatchSize = 378;
    static constexpr std::size_t patchDataOffset = 63;
    static constexpr std::size_t patchDataSize = 315;

    [[nodiscard]] static juce::MidiMessage makeSinglePatchRequest (IonBank bank, int slot);
    [[nodiscard]] static juce::MidiMessage makeBankRequest (IonBank bank);

    /** Convert arbitrary full-width bytes to the Alesis 7-of-8 transport form.
        The final group is padded with zero bytes to seven decoded bytes.
    */
    [[nodiscard]] static std::vector<std::uint8_t> encode7Of8 (const std::vector<std::uint8_t>& decoded);

    /** Decode the Alesis 7-of-8 transport form. Encoded input must contain
        complete 8-byte transport groups.
    */
    [[nodiscard]] static juce::Result decode7Of8 (const std::uint8_t* encoded,
                                                   std::size_t encodedSize,
                                                   std::vector<std::uint8_t>& decoded);

    [[nodiscard]] static juce::Result decode7Of8 (const std::vector<std::uint8_t>& encoded,
                                                   std::vector<std::uint8_t>& decoded)
    {
        return decode7Of8 (encoded.data(), encoded.size(), decoded);
    }

    /** Parse a single 434-byte-on-the-wire patch dump represented as a JUCE
        SysEx message (JUCE excludes F0/F7 from getSysExData()).
    */
    [[nodiscard]] static juce::Result decodeSinglePatchDump (const juce::MidiMessage& message,
                                                              IonPatchDump& patch);

    /** Encode a decoded 378-byte candidate patch image back to SysEx. The
        checksum is recalculated in the copied image before 7-of-8 packing.
        Unknown/unmapped bytes are therefore preserved byte-for-byte.
    */
    [[nodiscard]] static juce::Result encodeSinglePatchDump (const std::vector<std::uint8_t>& decoded,
                                                              juce::MidiMessage& message);

    [[nodiscard]] static std::uint32_t computePatchChecksumComplement (const std::vector<std::uint8_t>& decoded);
    [[nodiscard]] static bool validatePatchChecksum (const std::vector<std::uint8_t>& decoded);

    [[nodiscard]] static juce::String bankName (IonBank bank);
    [[nodiscard]] static bool isValidBank (int value) noexcept;

private:
    [[nodiscard]] static std::uint32_t readU32BE (const std::uint8_t* p) noexcept;
    [[nodiscard]] static juce::String readAscii (const std::vector<std::uint8_t>& bytes,
                                                  std::size_t offset,
                                                  std::size_t length,
                                                  bool stopAtNul = true);
};
}
