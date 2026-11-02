#pragma once

#include <QMetaType>
#include <QtGlobal>

namespace vsm::device {

/// Health counters of the device link, shown in the UI and useful in logs.
struct LinkStatistics {
    quint64 framesReceived{0};    ///< Valid frames (CRC OK).
    quint64 crcErrors{0};         ///< Frames dropped because of a CRC mismatch.
    quint64 bytesDiscarded{0};    ///< Garbage bytes skipped while re-synchronising.
    quint64 sequenceGaps{0};      ///< Times the sequence number jumped.
    quint64 framesLost{0};        ///< Frames missing in those jumps (sum of the gap sizes).
    quint64 malformedMessages{0}; ///< Valid frames with an unknown type or bad payload.
    quint64 implausibleValues{0}; ///< Valid numbers outside what a body can produce (device fault).

    friend bool operator==(const LinkStatistics&, const LinkStatistics&) = default;
};

} // namespace vsm::device

Q_DECLARE_METATYPE(vsm::device::LinkStatistics)
