#include "SinkFactory.h"

#include "SerialSink.h"
#include "TcpBroadcastSink.h"

namespace vsm::simulator {

std::unique_ptr<IByteSink> createDefaultSink(const OutputSettings& settings)
{
    switch (settings.kind) {
    case OutputSettings::Kind::Serial:
        return std::make_unique<SerialSink>(settings.serialPortName, settings.baudRate);
    case OutputSettings::Kind::Tcp:
        break;
    }
    // Localhost only: the simulator is a development tool and must not be
    // reachable from the network by default.
    return std::make_unique<TcpBroadcastSink>(QHostAddress::LocalHost, settings.tcpPort);
}

} // namespace vsm::simulator
