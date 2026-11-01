#include "vsm/protocol/FrameDecoder.h"

#include "vsm/protocol/Crc16.h"

namespace vsm::protocol {

namespace {

[[nodiscard]] std::uint16_t readUint16(const std::vector<std::uint8_t>& buffer, std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(buffer[offset] | (buffer[offset + 1] << 8));
}

} // namespace

FrameDecoder::FrameDecoder(std::size_t maxBufferedBytes)
    : m_maxBufferedBytes(maxBufferedBytes)
{
}

std::vector<Frame> FrameDecoder::feed(std::span<const std::uint8_t> bytes)
{
    m_buffer.insert(m_buffer.end(), bytes.begin(), bytes.end());

    std::vector<Frame> frames;
    std::size_t pos = 0;

    while (pos < m_buffer.size()) {
        // 1. Skip to the next sync pattern.
        const std::size_t sync = findSync(pos);
        m_statistics.bytesDiscarded += sync - pos;
        pos = sync;

        // 2. Wait for a complete header.
        const std::size_t available = m_buffer.size() - pos;
        if (available < kHeaderSize) {
            break;
        }

        // 3. Validate the header before trusting the length field.
        const std::uint8_t version = m_buffer[pos + 2];
        const std::size_t payloadSize = readUint16(m_buffer, pos + 6);
        if (version != kProtocolVersion || payloadSize > kMaxPayloadSize) {
            ++m_statistics.invalidHeaders;
            ++m_statistics.bytesDiscarded;
            ++pos; // resynchronise: the real frame may start inside this one
            continue;
        }

        // 4. Wait for the complete frame.
        const std::size_t frameSize = kHeaderSize + payloadSize + kCrcSize;
        if (available < frameSize) {
            break;
        }

        // 5. Verify the CRC (covers version .. end of payload).
        const auto covered = std::span(m_buffer).subspan(pos + 2, kHeaderSize - 2 + payloadSize);
        const std::uint16_t expectedCrc = readUint16(m_buffer, pos + kHeaderSize + payloadSize);
        if (crc16CcittFalse(covered) != expectedCrc) {
            ++m_statistics.crcErrors;
            ++m_statistics.bytesDiscarded;
            ++pos;
            continue;
        }

        // 6. Valid frame.
        Frame frame;
        frame.type = static_cast<MessageType>(m_buffer[pos + 3]);
        frame.sequence = readUint16(m_buffer, pos + 4);
        const auto payloadBegin = m_buffer.begin() + static_cast<std::ptrdiff_t>(pos + kHeaderSize);
        frame.payload.assign(payloadBegin, payloadBegin + static_cast<std::ptrdiff_t>(payloadSize));
        frames.push_back(std::move(frame));

        ++m_statistics.framesDecoded;
        pos += frameSize;
    }

    // Keep only the unconsumed tail (a partial frame or a partial sync byte).
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(pos));

    if (m_buffer.size() > m_maxBufferedBytes) {
        m_statistics.bytesDiscarded += m_buffer.size();
        m_buffer.clear();
    }

    return frames;
}

void FrameDecoder::reset() noexcept
{
    m_buffer.clear();
    m_statistics = {};
}

std::size_t FrameDecoder::findSync(std::size_t from) const noexcept
{
    for (std::size_t i = from; i < m_buffer.size(); ++i) {
        if (m_buffer[i] != kSyncByte1) {
            continue;
        }
        // A lone first sync byte at the very end may be the start of a frame
        // whose second byte has not arrived yet - keep it.
        if (i + 1 == m_buffer.size() || m_buffer[i + 1] == kSyncByte2) {
            return i;
        }
    }
    return m_buffer.size();
}

} // namespace vsm::protocol
