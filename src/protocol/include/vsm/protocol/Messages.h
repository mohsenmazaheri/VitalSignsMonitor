#pragma once

#include "vsm/protocol/Frame.h"

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace vsm::protocol {

/// Periodic "device is alive" message.
struct HeartbeatMessage {
    std::uint32_t uptimeMs{0}; ///< Milliseconds since the device started.

    friend bool operator==(const HeartbeatMessage&, const HeartbeatMessage&) = default;
};

/// Numeric vital signs as computed by the device.
/// An empty optional means "value not available" (sent as 0xFFFF / 0xFF).
struct NumericsMessage {
    std::optional<std::uint16_t> heartRateBpm;
    std::optional<std::uint8_t> spo2Percent;
    bool ecgLeadsOff{false};  ///< ECG electrodes are disconnected.
    bool spo2ProbeOff{false}; ///< SpO2 finger probe is disconnected.

    friend bool operator==(const NumericsMessage&, const NumericsMessage&) = default;
};

/// A block of consecutive ECG samples.
struct EcgWaveformMessage {
    std::uint16_t sampleRateHz{250};
    std::vector<std::int16_t> samplesMicrovolts; ///< 1 LSB = 1 uV.

    friend bool operator==(const EcgWaveformMessage&, const EcgWaveformMessage&) = default;
};

/// Every application-level message the protocol can carry.
using Message = std::variant<HeartbeatMessage, NumericsMessage, EcgWaveformMessage>;

/// Largest number of ECG samples that fit in one frame.
inline constexpr std::size_t kMaxEcgSamplesPerFrame = (kMaxPayloadSize - 4) / 2;

/// Builds a frame (type + payload) for @p message.
/// The caller still has to pass the result to encodeFrame() to get bytes;
/// encodeFrame() throws if an ECG block exceeds kMaxEcgSamplesPerFrame.
[[nodiscard]] Frame toFrame(const Message& message, std::uint16_t sequence);

/// Parses the payload of a validated frame.
/// @return std::nullopt for unknown message types or malformed payloads.
[[nodiscard]] std::optional<Message> parseMessage(const Frame& frame);

} // namespace vsm::protocol
