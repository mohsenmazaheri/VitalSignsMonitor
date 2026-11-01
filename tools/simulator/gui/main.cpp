// VitalSigns Patient Simulator - composition root of the front-panel GUI.
//
// A desktop stand-in for a hardware patient simulator: it generates SYNTHETIC
// ECG / HR / SpO2 and sends it to the monitor over a serial port or TCP.
//
// Examples:
//   VitalSignsPatientSimulator                       TCP server on 127.0.0.1:5555
//   VitalSignsPatientSimulator --serial COM5         serial output (e.g. one end of a com0com pair)
//   VitalSignsPatientSimulator --scenario demo       start a scripted scenario

#include "PatientSimulatorController.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQuickStyle>

// VitalSigns.Ui is a static QML module; its plugin must be imported explicitly.
Q_IMPORT_QML_PLUGIN(VitalSigns_UiPlugin)

int main(int argc, char* argv[])
{
    using namespace vsm::simulator;

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("VitalSigns Patient Simulator"));
    QGuiApplication::setOrganizationName(QStringLiteral("Mohsen Mazaheri"));
    QGuiApplication::setApplicationVersion(QStringLiteral(VSM_VERSION));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    // --- Command line -------------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Front panel of a simulated patient simulator. Sends SYNTHETIC data only."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption portOption({QStringLiteral("p"), QStringLiteral("port")},
                                        QStringLiteral("TCP port at start-up (default 5555)."), QStringLiteral("port"),
                                        QStringLiteral("5555"));
    const QCommandLineOption serialOption(QStringLiteral("serial"),
                                          QStringLiteral("Open this serial port at start-up instead of TCP."),
                                          QStringLiteral("name"));
    const QCommandLineOption baudOption(QStringLiteral("baud"), QStringLiteral("Serial baud rate (default 115200)."),
                                        QStringLiteral("rate"), QStringLiteral("115200"));
    const QCommandLineOption scenarioOption({QStringLiteral("s"), QStringLiteral("scenario")},
                                            QStringLiteral("Start this scenario (default: manual mode)."),
                                            QStringLiteral("name"));
    parser.addOptions({portOption, serialOption, baudOption, scenarioOption});
    parser.process(app);

    // --- Object graph -------------------------------------------------------
    DeviceSimulator simulator(250);
    gui::PatientSimulatorController controller(simulator, &createDefaultSink);

    OutputSettings output;
    if (parser.isSet(serialOption)) {
        output.kind = OutputSettings::Kind::Serial;
        output.serialPortName = parser.value(serialOption);
        output.baudRate = parser.value(baudOption).toInt();
    } else {
        output.tcpPort = parser.value(portOption).toUShort();
    }
    controller.openOutput(output); // an error is shown on the panel, not fatal
    if (parser.isSet(scenarioOption)) {
        controller.runScenario(parser.value(scenarioOption));
    }
    simulator.start();

    // --- UI -----------------------------------------------------------------
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("device"), QVariant::fromValue(&controller)}});
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.loadFromModule("VitalSigns.PatientSimulator", "Main");

    return QGuiApplication::exec();
}
