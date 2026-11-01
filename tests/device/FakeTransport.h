#pragma once

#include "vsm/device/ITransport.h"

#include <vector>

namespace vsm::test {

/// In-memory ITransport: the test decides when the "device" connects and
/// which bytes it sends. Demonstrates why DeviceSession depends on the
/// interface and not on QTcpSocket.
class FakeTransport final : public device::ITransport {
    Q_OBJECT

public:
    using ITransport::ITransport;

    void open() override
    {
        ++openCount;
        setState(State::Connected);
    }
    void close() override { setState(State::Disconnected); }
    [[nodiscard]] State state() const override { return m_state; }
    [[nodiscard]] QString description() const override { return QStringLiteral("fake://device"); }

    // --- Test controls -----------------------------------------------------
    void inject(const std::vector<std::uint8_t>& bytes)
    {
        emit dataReceived(QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size())));
    }
    void simulateDrop() { setState(State::Disconnected); }

    int openCount{0};

private:
    void setState(State state)
    {
        if (m_state != state) {
            m_state = state;
            emit stateChanged(state);
        }
    }

    State m_state{State::Disconnected};
};

} // namespace vsm::test
