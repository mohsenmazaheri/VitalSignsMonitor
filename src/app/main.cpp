// VitalSigns Monitor - composition root.
//
// This is the only place that knows the concrete classes: it builds the
// object graph (device I/O, alarm rules, view model) and hands the view model
// to QML. Everything else depends on interfaces or injected objects.

#include "MonitorViewModel.h"

#include "vsm/core/AlarmEvaluator.h"
#include "vsm/core/AlarmLimits.h"
#include "vsm/core/AlarmRules.h"
#include "vsm/device/DeviceController.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQuickStyle>

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
    parser.process(app);

    // --- Object graph -------------------------------------------------------
    vsm::device::DeviceController device; // owns the I/O thread
    vsm::core::AlarmEvaluator evaluator(vsm::core::makeDefaultAlarmRules(vsm::core::AlarmLimits{}));
    vsm::app::MonitorViewModel monitor(device, std::move(evaluator));

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

    return QGuiApplication::exec();
}
