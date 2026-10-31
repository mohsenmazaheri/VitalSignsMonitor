#pragma once

#include <cstdint>
#include <span>

namespace vsm::protocol {

/// Computes CRC-16/CCITT-FALSE over @p data.
///
/// Parameters: polynomial 0x1021, initial value 0xFFFF, no input/output
/// reflection, no final XOR. The standard check value for the ASCII string
/// "123456789" is 0x29B1.
[[nodiscard]] std::uint16_t crc16CcittFalse(std::span<const std::uint8_t> data) noexcept;

} // namespace vsm::protocol
