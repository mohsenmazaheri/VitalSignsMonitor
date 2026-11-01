#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

namespace vsm::core {

/// Fixed-size buffer for a "sweep" waveform display, as used on bedside
/// monitors: new samples overwrite the screen from left to right, and the
/// write position wraps around to the left edge when the screen is full.
///
/// Unlike a classic ring buffer, samples() returns the data in *screen order*
/// (index 0 = left edge), not in time order. writeIndex() is the column that
/// receives the next sample - the "erase bar" is drawn just after it.
///
/// @tparam T sample type (e.g. float millivolts).
template <typename T>
class SweepBuffer {
public:
    /// @throws std::invalid_argument if @p capacity is 0.
    explicit SweepBuffer(std::size_t capacity)
        : m_samples(capacity, T{})
    {
        if (capacity == 0) {
            throw std::invalid_argument("SweepBuffer: capacity must be > 0");
        }
    }

    void push(T sample) noexcept
    {
        m_samples[m_writeIndex] = sample;
        m_writeIndex = (m_writeIndex + 1) % m_samples.size();
        if (m_filled < m_samples.size()) {
            ++m_filled;
        }
    }

    void push(std::span<const T> samples) noexcept
    {
        for (const T& sample : samples) {
            push(sample);
        }
    }

    void clear() noexcept
    {
        std::fill(m_samples.begin(), m_samples.end(), T{});
        m_writeIndex = 0;
        m_filled = 0;
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return m_samples.size(); }
    [[nodiscard]] std::size_t writeIndex() const noexcept { return m_writeIndex; }
    /// Number of valid samples; equals capacity() after the first full sweep.
    [[nodiscard]] std::size_t filledCount() const noexcept { return m_filled; }
    [[nodiscard]] bool isFull() const noexcept { return m_filled == m_samples.size(); }
    [[nodiscard]] std::span<const T> samples() const noexcept { return m_samples; }

private:
    std::vector<T> m_samples;
    std::size_t m_writeIndex{0};
    std::size_t m_filled{0};
};

} // namespace vsm::core
