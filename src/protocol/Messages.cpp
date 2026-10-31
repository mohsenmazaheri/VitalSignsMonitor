#include "vsm/protocol/Messages.h"

#include <type_traits>

namespace vsm::protocol {

namespace {

constexpr std::uint16_t kInvalidHeartRate = 0xFFFF;
constexpr std::uint8_t kInvalidSpo2 = 0xFF;
constexpr std::uint8_t kFlagEcgLeadsOff = 0x01;
constexpr std::uint8_t kFlagSpo2ProbeOff = 0x02;

constexpr std::size_t kHeartbeatPayloadSize = 4;
constexpr std::size_t kNumericsPayloadSize = 4;
constexpr std::size_t kEcgHeaderSize = 4; // sample rate + sample count

/// Little-endian writer for payload bytes.
class ByteWriter {
public:
    void u8(std::uint8_t value) { m_bytes.push_back(value); }
    void u16(std::uint16_t value)
    {
        m_bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
        m_bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    }
    void u32(std::uint32_t value)
    {
        u16(static_cast<std::uint16_t>(value & 0xFFFFU));
        u16(static_cast<std::uint16_t>(value >> 16));
    }
    void i16(std::int16_t value) { u16(static_cast<std::uint16_t>(value)); }

    [[nodiscard]] std::vector<std::uint8_t> take() { return std::move(m_bytes); }

private:
    std::vector<std::uint8_t> m_bytes;
};

/// Little-endian reader; callers check the payload size up front, so the
/// reader itself does no bounds checking.
class ByteReader {
public:
    explicit ByteReader(const std::vector<std::uint8_t>& bytes) : m_bytes(bytes) {}

    std::uint8_t u8() { return m_bytes[m_pos++]; }
    std::uint16_t u16()
    {
        const auto low = m_bytes[m_pos];
        const auto high = m_bytes[m_pos + 1];
        m_pos += 2;
        return static_cast<std::uint16_t>(low | (high << 8));
    }
    std::uint32_t u32()
    {
        const std::uint32_t low = u16();
        const std::uint32_t high = u16();
        return low | (high << 16);
    }
    std::int16_t i16() { return static_cast<std::int16_t>(u16()); }

private:
    const std::vector<std::uint8_t>& m_bytes;
    std::size_t m_pos{0};
};

[[nodiscard]] std::optional<Message> parseHeartbeat(const Frame& frame)
{
    if (frame.payload.size() != kHeartbeatPayloadSize) {
        return std::nullopt;
    }
    ByteReader reader(frame.payload);
    return HeartbeatMessage{reader.u32()};
}

[[nodiscard]] std::optional<Message> parseNumerics(const Frame& frame)
{
    if (frame.payload.size() != kNumericsPayloadSize) {
        return std::nullopt;
    }
    ByteReader reader(frame.payload);
    NumericsMessage message;

    const std::uint16_t heartRate = reader.u16();
    if (heartRate != kInvalidHeartRate) {
        message.heartRateBpm = heartRate;
    }
    const std::uint8_t spo2 = reader.u8();
    if (spo2 != kInvalidSpo2) {
        message.spo2Percent = spo2;
    }
    const std::uint8_t flags = reader.u8();
    message.ecgLeadsOff = (flags & kFlagEcgLeadsOff) != 0;
    message.spo2ProbeOff = (flags & kFlagSpo2ProbeOff) != 0;
    return message;
}

[[nodiscard]] std::optional<Message> parseEcgWaveform(const Frame& frame)
{
    if (frame.payload.size() < kEcgHeaderSize) {
        return std::nullopt;
    }
    ByteReader reader(frame.payload);
    EcgWaveformMessage message;
    message.sampleRateHz = reader.u16();
    const std::size_t count = reader.u16();

    // The declared sample count must match the payload exactly.
    if (message.sampleRateHz == 0 || frame.payload.size() != kEcgHeaderSize + count * 2) {
        return std::nullopt;
    }
    message.samplesMicrovolts.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        message.samplesMicrovolts.push_back(reader.i16());
    }
    return message;
}

} // namespace

Frame toFrame(const Message& message, std::uint16_t sequence)
{
    Frame frame;
    frame.sequence = sequence;
    ByteWriter writer;

    std::visit(
        [&](const auto& msg) {
            using T = std::decay_t<decltype(msg)>;
            if constexpr (std::is_same_v<T, HeartbeatMessage>) {
                frame.type = MessageType::Heartbeat;
                writer.u32(msg.uptimeMs);
            } else if constexpr (std::is_same_v<T, NumericsMessage>) {
                frame.type = MessageType::Numerics;
                writer.u16(msg.heartRateBpm.value_or(kInvalidHeartRate));
                writer.u8(msg.spo2Percent.value_or(kInvalidSpo2));
                std::uint8_t flags = 0;
                if (msg.ecgLeadsOff) {
                    flags |= kFlagEcgLeadsOff;
                }
                if (msg.spo2ProbeOff) {
                    flags |= kFlagSpo2ProbeOff;
                }
                writer.u8(flags);
            } else if constexpr (std::is_same_v<T, EcgWaveformMessage>) {
                frame.type = MessageType::EcgWaveform;
                writer.u16(msg.sampleRateHz);
                writer.u16(static_cast<std::uint16_t>(msg.samplesMicrovolts.size()));
                for (const std::int16_t sample : msg.samplesMicrovolts) {
                    writer.i16(sample);
                }
            }
        },
        message);

    frame.payload = writer.take();
    return frame;
}

std::optional<Message> parseMessage(const Frame& frame)
{
    switch (frame.type) {
    case MessageType::Heartbeat:
        return parseHeartbeat(frame);
    case MessageType::Numerics:
        return parseNumerics(frame);
    case MessageType::EcgWaveform:
        return parseEcgWaveform(frame);
    }
    return std::nullopt; // unknown type byte - forward compatible, just ignore
}

} // namespace vsm::protocol
