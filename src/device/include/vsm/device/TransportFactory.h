#pragma once

#include "vsm/device/ConnectionSettings.h"
#include "vsm/device/ITransport.h"

#include <functional>
#include <memory>

namespace vsm::device {

/// Creates a transport for the given settings. Injected into DeviceController
/// so tests can supply fake transports.
using TransportFactory = std::function<std::unique_ptr<ITransport>(const ConnectionSettings&)>;

/// Production factory: TcpTransport or SerialTransport depending on settings.kind.
[[nodiscard]] std::unique_ptr<ITransport> createDefaultTransport(const ConnectionSettings& settings);

} // namespace vsm::device
