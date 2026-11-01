#include "vsm/device/TransportFactory.h"

#include "vsm/device/SerialTransport.h"
#include "vsm/device/TcpTransport.h"

namespace vsm::device {

std::unique_ptr<ITransport> createDefaultTransport(const ConnectionSettings& settings)
{
    switch (settings.kind) {
    case ConnectionSettings::Kind::Tcp:
        return std::make_unique<TcpTransport>(settings.host, settings.tcpPort);
    case ConnectionSettings::Kind::Serial:
        return std::make_unique<SerialTransport>(settings.serialPortName, settings.baudRate);
    }
    return nullptr;
}

} // namespace vsm::device
