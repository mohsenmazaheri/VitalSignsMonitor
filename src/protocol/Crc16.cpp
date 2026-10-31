#include "vsm/protocol/Crc16.h"

namespace vsm::protocol {

std::uint16_t crc16CcittFalse(std::span<const std::uint8_t> data) noexcept
{
    constexpr std::uint16_t kPolynomial = 0x1021;
    std::uint16_t crc = 0xFFFF;

    // Bit-wise implementation: frames are small (< 1 KiB), so a lookup table
    // would add code without a measurable benefit.
    for (const std::uint8_t byte : data) {
        crc = static_cast<std::uint16_t>(crc ^ (static_cast<std::uint16_t>(byte) << 8));
        for (int bit = 0; bit < 8; ++bit) {
            const bool msbSet = (crc & 0x8000U) != 0;
            crc = static_cast<std::uint16_t>(crc << 1);
            if (msbSet) {
                crc = static_cast<std::uint16_t>(crc ^ kPolynomial);
            }
        }
    }
    return crc;
}

} // namespace vsm::protocol
