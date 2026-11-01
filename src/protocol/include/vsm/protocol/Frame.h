#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// @file Frame.h
/// Binary frame layout (all multi-byte fields little-endian):
///
/// | Offset | Size | Field                                   |
/// |-------:|-----:|-----------------------------------------|
/// |      0 |    2 | Sync bytes 0xAA 0x55                    |
/// |      2 |    1 | Protocol version (currently 1)          |
/// |      3 |    1 | Message type (see MessageType)          |
/// |      4 |    2 | Sequence number (wraps at 65535)        |
/// |      6 |    2 | Payload length N (0..kMaxPayloadSize)   |
/// |      8 |    N | Payload                                 |
/// |  8 + N |    2 | CRC-16/CCITT-FALSE over bytes 2..8+N-1  |
///
/// See docs/PROTOCOL.md for the payload of each message type.

namespace vsm::protocol {

inline constexpr std::uint8_t kSyncByte1 = 0xAA;
inline constexpr std::uint8_t kSyncByte2 = 0x55;
inline constexpr std::uint8_t kProtocolVersion = 1;

inline constexpr std::size_t kHeaderSize = 8;       ///< Sync + version + type + sequence + length.
inline constexpr std::size_t kCrcSize = 2;
inline constexpr std::size_t kMaxPayloadSize = 1024; ///< Upper bound that protects the decoder.

/// Identifies the payload carried by a frame.
enum class MessageType : std::uint8_t {
    Heartbeat = 0x01,   ///< Device is alive; carries device uptime.
    Numerics = 0x10,    ///< Heart rate, SpO2 and sensor status flags (1 Hz).
    EcgWaveform = 0x20, ///< Block of ECG samples (typically every 40 ms).
};

/// One validated protocol frame (header fields + raw payload).
struct Frame {
    MessageType type{MessageType::Heartbeat};
    std::uint16_t sequence{0};
    std::vector<std::uint8_t> payload;

    friend bool operator==(const Frame&, const Frame&) = default;
};

/// Serialises @p frame into its on-wire representation (header, payload, CRC).
/// @throws std::length_error if the payload is larger than kMaxPayloadSize.
[[nodiscard]] std::vector<std::uint8_t> encodeFrame(const Frame& frame);

} // namespace vsm::protocol
