// VitalSigns Monitor - composition root.
//
// This is the only place that knows the concrete classes: it builds the
// object graph (device I/O, alarm rules and manager, audit log, sound, view
// model) and hands the view model to QML. Everything else depends on interfaces
// or injected objects.

#include "AuditLogModel.h"
#include "MonitorViewModel.h"
#include "QtAlarmSound.h"

#include "vsm/audit/FileAuditLog.h"
#include "vsm/core/AlarmEvaluator.h"
#include "vsm/core/AlarmLimits.h"
#include "vsm/core/AlarmManager.h"
#include "vsm/core/AlarmRules.h"
#include "vsm/device/DeviceController.h"

#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQuickStyle>
#include <QStandardPaths>

#include <chrono>
#include <filesystem>
#include <memory>

// VitalSigns.Ui is a static QML module; its plugin must be imported explicitly.
Q_IMPORT_QML_PLUGIN(VitalSigns_UiPlugin)

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("VitalSigns Monitor"));
    QGuiApplication::setOrganizationName(QStringLiteral("Mohsen Mazaheri"));
    QGuiApplication::setApplicationVersion(QStringLiteral(VSM_VERSION));

    // Fusion honours the dark palette set in Main.qml on every platform.
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    // --- Command line -------------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Vital-signs monitor for a simulated device (synthetic data)."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption connectOption(QStringLiteral("connect"),
                                           QStringLiteral("Connect to a TCP device at start-up, e.g. 127.0.0.1:5555."),
                                           QStringLiteral("host:port"));
    parser.addOption(connectOption);
    const QCommandLineOption noSoundOption(QStringLiteral("no-sound"),
                                           QStringLiteral("Do not play alarm sounds (visual alarm signals only)."));
    parser.addOption(noSoundOption);
    const QCommandLineOption auditDirOption(QStringLiteral("audit-dir"),
                                            QStringLiteral("Folder for the audit log (default: application data folder)."),
                                            QStringLiteral("folder"));
    parser.addOption(auditDirOption);
    parser.process(app);

    // --- Object graph -------------------------------------------------------
    const QString auditDir = parser.isSet(auditDirOption)
                                 ? parser.value(auditDirOption)
                                 : QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                       + QStringLiteral("/audit");
    const auto auditFile = std::filesystem::path(auditDir.toStdU16String())
                           / vsm::audit::auditFileName(std::chrono::system_clock::now());
    vsm::audit::FileAuditLog fileLog(auditFile);
    vsm::app::AuditLogModel auditLog(fileLog, QDir::toNativeSeparators(QString::fromStdU16String(auditFile.u16string())));
    vsm::core::AlarmManager alarmManager(auditLog);

    std::unique_ptr<vsm::app::IAlarmSound> sound;
    if (parser.isSet(noSoundOption)) {
        sound = std::make_unique<vsm::app::NullAlarmSound>();
    } else {
        sound = std::make_unique<vsm::app::QtAlarmSound>();
    }

    vsm::device::DeviceController device; // owns the I/O thread
    vsm::core::AlarmEvaluator evaluator(vsm::core::makeDefaultAlarmRules(vsm::core::AlarmLimits{}));
    vsm::app::MonitorViewModel monitor(device, std::move(evaluator), alarmManager, *sound, auditLog);

    auditLog.record({vsm::core::AuditEventType::ApplicationStarted, {}, std::nullopt,
                     "VitalSigns Monitor " VSM_VERSION});
    if (!fileLog.isOk()) {
        monitor.reportError(QObject::tr("Audit log cannot be written: %1").arg(auditLog.location()));
    }

    // --- UI -----------------------------------------------------------------
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("monitor"), QVariant::fromValue(&monitor)}});
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.loadFromModule("VitalSigns.App", "Main");

    if (parser.isSet(connectOption)) {
        const QString endpoint = parser.value(connectOption);
        const qsizetype colon = endpoint.lastIndexOf(QLatin1Char(':'));
        monitor.connectTcp(endpoint.left(colon), endpoint.mid(colon + 1).toInt());
    }

    const int exitCode = QGuiApplication::exec();
    auditLog.record({vsm::core::AuditEventType::ApplicationStopped, {}, std::nullopt, {}});
    return exitCode;
}
