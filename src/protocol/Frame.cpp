#include "vsm/protocol/Frame.h"

#include "vsm/protocol/Crc16.h"

#include <span>
#include <stdexcept>

namespace vsm::protocol {

namespace {

void appendUint16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

} // namespace

std::vector<std::uint8_t> encodeFrame(const Frame& frame)
{
    if (frame.payload.size() > kMaxPayloadSize) {
        throw std::length_error("vsm::protocol::encodeFrame: payload exceeds kMaxPayloadSize");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(kHeaderSize + frame.payload.size() + kCrcSize);

    bytes.push_back(kSyncByte1);
    bytes.push_back(kSyncByte2);
    bytes.push_back(kProtocolVersion);
    bytes.push_back(static_cast<std::uint8_t>(frame.type));
    appendUint16(bytes, frame.sequence);
    appendUint16(bytes, static_cast<std::uint16_t>(frame.payload.size()));
    bytes.insert(bytes.end(), frame.payload.begin(), frame.payload.end());

    // The CRC covers everything after the two sync bytes.
    const auto crc = crc16CcittFalse(std::span(bytes).subspan(2));
    appendUint16(bytes, crc);
    return bytes;
}

} // namespace vsm::protocol
