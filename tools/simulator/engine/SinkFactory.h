#pragma once

#include "IByteSink.h"

#include <functional>
#include <memory>

namespace vsm::simulator {

/// Where the simulated device sends its data.
struct OutputSettings {
    enum class Kind { Tcp, Serial };

    Kind kind{Kind::Tcp};
    quint16 tcpPort{5555};
    QString serialPortName; ///< e.g. "COM5" or "/dev/ttyUSB0"
    qint32 baudRate{115200};
};

/// Creates a sink for the given settings. Injected into the GUI controller so
/// that tests can substitute an in-memory sink (Dependency Inversion).
using SinkFactory = std::function<std::unique_ptr<IByteSink>(const OutputSettings&)>;

/// Default factory: TcpBroadcastSink (localhost only) or SerialSink.
[[nodiscard]] std::unique_ptr<IByteSink> createDefaultSink(const OutputSettings& settings);

} // namespace vsm::simulator
