#pragma once

#include "vsm/protocol/Frame.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace vsm::protocol {

/// Counters that describe the quality of the incoming byte stream.
struct DecoderStatistics {
    std::uint64_t framesDecoded{0};  ///< Frames that passed all checks.
    std::uint64_t crcErrors{0};      ///< Frames rejected because of a CRC mismatch.
    std::uint64_t invalidHeaders{0}; ///< Sync found but version/length were invalid.
    std::uint64_t bytesDiscarded{0}; ///< Bytes skipped while searching for a valid frame.

    friend bool operator==(const DecoderStatistics&, const DecoderStatistics&) = default;
};

/// Incremental (streaming) decoder that turns an arbitrary byte stream into
/// validated frames.
///
/// Serial ports and TCP sockets deliver data in chunks that have no relation
/// to frame boundaries, so the decoder buffers partial frames between calls.
/// On any error (bad version, oversized length, CRC mismatch) it drops exactly
/// one byte and searches for the next sync pattern, which lets it re-synchronise
/// after line noise without losing the following valid frames.
///
/// The class is not thread-safe; use one instance per stream.
class FrameDecoder {
public:
    /// @param maxBufferedBytes Hard limit for buffered, not yet decoded bytes.
    ///        Protects memory if a peer sends endless garbage.
    explicit FrameDecoder(std::size_t maxBufferedBytes = 64 * 1024);

    /// Appends @p bytes to the internal buffer and returns every complete,
    /// valid frame that is now available (in stream order).
    [[nodiscard]] std::vector<Frame> feed(std::span<const std::uint8_t> bytes);

    /// Drops buffered bytes and resets the statistics (e.g. after reconnecting).
    void reset() noexcept;

    [[nodiscard]] const DecoderStatistics& statistics() const noexcept { return m_statistics; }
    [[nodiscard]] std::size_t bufferedBytes() const noexcept { return m_buffer.size(); }

private:
    /// Returns the index of the next (possibly partial) sync pattern at or after
    /// @p from, or the buffer size if there is none.
    [[nodiscard]] std::size_t findSync(std::size_t from) const noexcept;

    std::vector<std::uint8_t> m_buffer;
    DecoderStatistics m_statistics;
    std::size_t m_maxBufferedBytes;
};

} // namespace vsm::protocol
