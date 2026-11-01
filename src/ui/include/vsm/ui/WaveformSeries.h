#pragma once

#include "vsm/core/SweepBuffer.h"

#include <QList>
#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace vsm::ui {

/// Data source for one sweep waveform (e.g. ECG lead II).
///
/// Holds exactly one screen width of samples (sweepSeconds * sampleRate) in a
/// core::SweepBuffer. WaveformItem reads it while rendering; both live in the
/// GUI thread, and Qt Quick blocks the GUI thread while the render thread
/// syncs, so no locking is needed.
class WaveformSeries : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("WaveformSeries is created in C++ and passed to QML")

    Q_PROPERTY(int sampleRateHz READ sampleRateHz NOTIFY layoutChanged)
    Q_PROPERTY(double sweepSeconds READ sweepSeconds CONSTANT)

public:
    WaveformSeries(int sampleRateHz, double sweepSeconds, QObject* parent = nullptr);

    /// Appends samples. If @p sampleRateHz differs from the current rate, the
    /// buffer is resized so the sweep still covers sweepSeconds.
    void append(const QList<float>& samples, int sampleRateHz);
    void clear();

    [[nodiscard]] int sampleRateHz() const noexcept { return m_sampleRateHz; }
    [[nodiscard]] double sweepSeconds() const noexcept { return m_sweepSeconds; }
    [[nodiscard]] const core::SweepBuffer<float>& buffer() const noexcept { return m_buffer; }

signals:
    void samplesAppended();
    void layoutChanged(); ///< sample rate / capacity changed

private:
    [[nodiscard]] static std::size_t capacityFor(int sampleRateHz, double sweepSeconds);

    int m_sampleRateHz;
    double m_sweepSeconds;
    core::SweepBuffer<float> m_buffer;
};

} // namespace vsm::ui
