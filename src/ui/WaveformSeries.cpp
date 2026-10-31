#include "vsm/ui/WaveformSeries.h"

#include <algorithm>
#include <cmath>

namespace vsm::ui {

WaveformSeries::WaveformSeries(int sampleRateHz, double sweepSeconds, QObject* parent)
    : QObject(parent)
    , m_sampleRateHz(std::max(1, sampleRateHz))
    , m_sweepSeconds(sweepSeconds > 0.0 ? sweepSeconds : 1.0)
    , m_buffer(capacityFor(m_sampleRateHz, m_sweepSeconds))
{
}

void WaveformSeries::append(const QList<float>& samples, int sampleRateHz)
{
    if (sampleRateHz > 0 && sampleRateHz != m_sampleRateHz) {
        m_sampleRateHz = sampleRateHz;
        m_buffer = core::SweepBuffer<float>(capacityFor(m_sampleRateHz, m_sweepSeconds));
        emit layoutChanged();
    }
    m_buffer.push(std::span<const float>(samples.constData(), static_cast<std::size_t>(samples.size())));
    emit samplesAppended();
}

void WaveformSeries::clear()
{
    m_buffer.clear();
    emit samplesAppended();
}

std::size_t WaveformSeries::capacityFor(int sampleRateHz, double sweepSeconds)
{
    const double samples = std::round(static_cast<double>(sampleRateHz) * sweepSeconds);
    return static_cast<std::size_t>(std::max(2.0, samples));
}

} // namespace vsm::ui
