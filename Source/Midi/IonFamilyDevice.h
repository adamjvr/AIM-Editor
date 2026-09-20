#pragma once

#include <cstdint>
#include <string_view>

namespace aim
{
/** Hardware shell around the shared Ion-family Program format.

    Both devices use the common 0x22/378-byte Program image for dumps, while
    the Micron uses product ID 0x26 for single-program request messages.
    Capability flags intentionally describe only behavior backed by current
    evidence; unsupported/unknown operations remain disabled rather than guessed.
*/
enum class IonFamilyDevice
{
    ion,
    micron
};

struct IonFamilyDeviceProfile
{
    IonFamilyDevice device = IonFamilyDevice::ion;
    std::string_view id;
    std::string_view displayName;
    std::uint8_t requestProductId = 0x22;
    std::uint8_t programDumpProductId = 0x22;
    int requestBankCount = 0;
    bool ionNamedBanks = false;
    bool supportsBankDumpRequest = false;
    bool supportsIonEditBuffers = false;
    bool supportsMicronFx2 = false;
    bool supportsMicronXyz = false;
};

inline constexpr IonFamilyDeviceProfile ionDeviceProfile {
    IonFamilyDevice::ion,
    "ion",
    "Alesis Ion",
    0x22,
    0x22,
    5,
    true,
    true,
    true,
    false,
    false
};

inline constexpr IonFamilyDeviceProfile micronDeviceProfile {
    IonFamilyDevice::micron,
    "micron",
    "Alesis Micron",
    0x26,
    0x22,
    8,
    false,
    false,
    false,
    true,
    true
};

[[nodiscard]] inline constexpr const IonFamilyDeviceProfile& profileFor (IonFamilyDevice device) noexcept
{
    return device == IonFamilyDevice::micron ? micronDeviceProfile : ionDeviceProfile;
}

[[nodiscard]] inline constexpr IonFamilyDevice deviceFromId (std::string_view id) noexcept
{
    return id == micronDeviceProfile.id ? IonFamilyDevice::micron : IonFamilyDevice::ion;
}
}
