#include "vsm/device/DeviceController.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcController, "vsm.device.controller")

namespace vsm::device {

DeviceController::DeviceController(TransportFactory transportFactory, QObject* parent)
    : QObject(parent)
    , m_transportFactory(std::move(transportFactory))
{
    qRegisterMetaType<vsm::core::VitalSigns>();
    qRegisterMetaType<vsm::device::LinkStatistics>();
    qRegisterMetaType<vsm::device::ITransport::State>();

    m_ioThread.setObjectName(QStringLiteral("DeviceIO"));
    m_ioThread.start();
}

DeviceController::~DeviceController()
{
    disconnectFromDevice();
    m_ioThread.quit();
    m_ioThread.wait();
}

void DeviceController::connectToDevice(const ConnectionSettings& settings)
{
    disconnectFromDevice();

    std::unique_ptr<ITransport> transport = m_transportFactory ? m_transportFactory(settings) : nullptr;
    if (!transport) {
        emit errorOccurred(tr("Could not create a transport for the selected connection."));
        return;
    }

    // Create in the GUI thread, then hand over to the I/O thread. The session
    // has no parent (required by moveToThread) and is deleted explicitly.
    m_session = new DeviceSession(std::move(transport));
    m_session->moveToThread(&m_ioThread);

    connect(m_session, &DeviceSession::vitalsReceived, this, &DeviceController::vitalsReceived);
    connect(m_session, &DeviceSession::ecgSamplesReceived, this, &DeviceController::ecgSamplesReceived);
    connect(m_session, &DeviceSession::connectionStateChanged, this, &DeviceController::connectionStateChanged);
    connect(m_session, &DeviceSession::linkLostChanged, this, &DeviceController::linkLostChanged);
    connect(m_session, &DeviceSession::statisticsChanged, this, &DeviceController::statisticsChanged);
    connect(m_session, &DeviceSession::errorOccurred, this, &DeviceController::errorOccurred);

    qCInfo(lcController) << "Connecting to" << m_session->description();
    QMetaObject::invokeMethod(m_session, &DeviceSession::start, Qt::QueuedConnection);
}

void DeviceController::disconnectFromDevice()
{
    if (m_session == nullptr) {
        return;
    }

    // Stop the session inside the I/O thread (sockets and timers must be used
    // from their own thread), then move it back so it can be deleted here.
    // BlockingQueuedConnection is safe: the I/O thread never waits on the GUI.
    QThread* const ownerThread = thread();
    DeviceSession* const session = m_session;
    QMetaObject::invokeMethod(
        session,
        [session, ownerThread] {
            session->stop();
            session->moveToThread(ownerThread);
        },
        Qt::BlockingQueuedConnection);

    delete session;
    m_session = nullptr;

    // The final "Disconnected"/"link ok" signals were queued during stop();
    // they still reach us because queued events belong to the receiver.
}

} // namespace vsm::device
