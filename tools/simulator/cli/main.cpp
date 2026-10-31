// VitalSigns Simulator - a fake bedside device that streams SYNTHETIC data.
//
// Examples:
//   VitalSignsSimulator                          TCP server on port 5555, "demo" scenario
//   VitalSignsSimulator --scenario tachycardia
//   VitalSignsSimulator --serial COM5 --baud 115200
//   VitalSignsSimulator --corrupt 0.02           corrupt ~2 % of the frames
//
// For an interactive front panel use VitalSignsPatientSimulator instead.

#include "DeviceSimulator.h"
#include "SinkFactory.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

namespace {

using namespace vsm::simulator;

QString scenarioList()
{
    QStringList names;
    for (const std::string_view name : scenarioNames()) {
        names.append(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
    }
    return names.join(QStringLiteral(", "));
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("VitalSignsSimulator"));
    QCoreApplication::setApplicationVersion(QStringLiteral(VSM_VERSION));

    QTextStream out(stdout);
    QTextStream err(stderr);

    // --- Command line -----------------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Simulated bedside monitor device. Streams SYNTHETIC ECG, heart rate and SpO2 "
                       "using the VitalSigns protocol. Not real patient data."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption portOption({QStringLiteral("p"), QStringLiteral("port")},
                                        QStringLiteral("TCP port to listen on (default 5555)."),
                                        QStringLiteral("port"), QStringLiteral("5555"));
    const QCommandLineOption serialOption(QStringLiteral("serial"),
                                          QStringLiteral("Send over this serial port instead of TCP (e.g. COM5)."),
                                          QStringLiteral("name"));
    const QCommandLineOption baudOption(QStringLiteral("baud"), QStringLiteral("Serial baud rate (default 115200)."),
                                        QStringLiteral("rate"), QStringLiteral("115200"));
    const QCommandLineOption scenarioOption({QStringLiteral("s"), QStringLiteral("scenario")},
                                            QStringLiteral("Scenario: %1 (default demo).").arg(scenarioList()),
                                            QStringLiteral("name"), QStringLiteral("demo"));
    const QCommandLineOption rateOption(QStringLiteral("rate"), QStringLiteral("ECG sample rate in Hz (default 250)."),
                                        QStringLiteral("hz"), QStringLiteral("250"));
    const QCommandLineOption corruptOption(QStringLiteral("corrupt"),
                                           QStringLiteral("Probability 0..1 that a frame is corrupted (default 0)."),
                                           QStringLiteral("p"), QStringLiteral("0"));
    parser.addOptions({portOption, serialOption, baudOption, scenarioOption, rateOption, corruptOption});
    parser.process(app);

    // --- Scenario ---------------------------------------------------------------
    const QString scenarioName = parser.value(scenarioOption);
    std::optional<Scenario> scenario = makeScenario(scenarioName.toStdString());
    if (!scenario) {
        err << "Unknown scenario '" << scenarioName << "'. Available: " << scenarioList() << Qt::endl;
        return EXIT_FAILURE;
    }

    bool ok = false;
    const int sampleRate = parser.value(rateOption).toInt(&ok);
    if (!ok || sampleRate < 50 || sampleRate > 1000) {
        err << "Sample rate must be between 50 and 1000 Hz." << Qt::endl;
        return EXIT_FAILURE;
    }

    // --- Output -----------------------------------------------------------------
    OutputSettings settings;
    if (parser.isSet(serialOption)) {
        settings.kind = OutputSettings::Kind::Serial;
        settings.serialPortName = parser.value(serialOption);
        settings.baudRate = parser.value(baudOption).toInt(&ok);
        if (!ok || settings.baudRate <= 0) {
            err << "Invalid baud rate." << Qt::endl;
            return EXIT_FAILURE;
        }
    } else {
        settings.tcpPort = parser.value(portOption).toUShort(&ok);
        if (!ok || settings.tcpPort == 0) {
            err << "Invalid TCP port." << Qt::endl;
            return EXIT_FAILURE;
        }
    }

    const std::unique_ptr<IByteSink> sink = createDefaultSink(settings);
    QString error;
    if (!sink->open(error)) {
        err << "Cannot open " << sink->description() << ": " << error << Qt::endl;
        return EXIT_FAILURE;
    }
    QObject::connect(sink.get(), &IByteSink::statusChanged, [&] { out << "Output: " << sink->status() << Qt::endl; });
    QObject::connect(sink.get(), &IByteSink::errorOccurred,
                     [&](const QString& message) { err << "Output error: " << message << Qt::endl; });

    // --- Device -------------------------------------------------------------------
    DeviceSimulator simulator(sampleRate);
    simulator.setSink(sink.get());
    simulator.setCorruptionProbability(parser.value(corruptOption).toDouble());
    QObject::connect(&simulator, &DeviceSimulator::phaseChanged, [&](const QString& label) {
        const PatientTargets& t = simulator.targets();
        out << "[" << QString::number(static_cast<double>(simulator.scenarioElapsedMs()) / 1000.0, 'f', 1) << " s] "
            << label << " -> HR " << t.heartRateBpm << " bpm, SpO2 " << t.spo2Percent << " %"
            << (t.ecgLeadsOff ? ", ECG leads off" : "") << (t.spo2ProbeOff ? ", SpO2 probe off" : "") << Qt::endl;
    });

    out << "VitalSigns Simulator " << QCoreApplication::applicationVersion() << " - SYNTHETIC DATA ONLY\n"
        << "Output:   " << sink->description() << '\n'
        << "Scenario: " << scenarioName << ", ECG " << sampleRate << " Hz\n"
        << "Press Ctrl+C to stop." << Qt::endl;

    simulator.runScenario(std::move(*scenario));
    simulator.start();
    return QCoreApplication::exec();
}
