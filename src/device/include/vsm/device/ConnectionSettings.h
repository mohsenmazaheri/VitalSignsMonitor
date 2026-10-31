#pragma once

#include <QString>

namespace vsm::device {

/// How to reach the device. Plain value type, easy to load from settings later.
struct ConnectionSettings {
    enum class Kind {
        Tcp,
        Serial,
    };

    Kind kind{Kind::Tcp};

    // TCP
    QString host{QStringLiteral("127.0.0.1")};
    quint16 tcpPort{5555};

    // Serial
    QString serialPortName;
    qint32 baudRate{115200};
};

} // namespace vsm::device
