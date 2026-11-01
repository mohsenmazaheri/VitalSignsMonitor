#include "QtAlarmSound.h"

#include "vsm/core/AlarmTone.h"

#include <QFile>
#include <QLoggingCategory>
#include <QUrl>

Q_LOGGING_CATEGORY(lcSound, "vsm.app.sound")

namespace vsm::app {

namespace {

constexpr int kSampleRateHz = 22050;
constexpr float kVolume = 0.7F;

[[nodiscard]] std::size_t indexOf(core::AlarmPriority priority)
{
    return static_cast<std::size_t>(priority) - 1U; // Low = 1 .. High = 3
}

} // namespace

QtAlarmSound::QtAlarmSound(QObject* parent)
    : QObject(parent)
{
    using enum core::AlarmPriority;
    createEffect(Low);
    createEffect(Medium);
    createEffect(High);
}

void QtAlarmSound::createEffect(core::AlarmPriority priority)
{
    const auto samples = core::makeAlarmTone(priority, kSampleRateHz);
    const auto wav = core::encodeWav(samples, kSampleRateHz);

    const QString path = m_folder.filePath(QStringLiteral("alarm-%1.wav").arg(static_cast<int>(priority)));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(reinterpret_cast<const char*>(wav.data()), static_cast<qint64>(wav.size())) < 0) {
        qCWarning(lcSound) << "Could not write the alarm sound" << path;
        return;
    }
    file.close();

    auto* effect = new QSoundEffect(this);
    effect->setLoopCount(QSoundEffect::Infinite);
    effect->setVolume(kVolume);
    effect->setSource(QUrl::fromLocalFile(path));
    connect(effect, &QSoundEffect::statusChanged, this, [effect, priority] {
        if (effect->status() == QSoundEffect::Error) {
            qCWarning(lcSound) << "Alarm sound is not available for priority" << static_cast<int>(priority);
        }
    });
    m_effects[indexOf(priority)] = effect;
}

QSoundEffect* QtAlarmSound::effectFor(core::AlarmPriority priority) const
{
    return m_effects[indexOf(priority)];
}

void QtAlarmSound::setPriority(std::optional<core::AlarmPriority> priority)
{
    if (priority == m_priority) {
        return;
    }
    if (m_priority) {
        if (QSoundEffect* current = effectFor(*m_priority)) {
            current->stop();
        }
    }
    m_priority = priority;
    if (m_priority) {
        if (QSoundEffect* next = effectFor(*m_priority)) {
            qCInfo(lcSound) << "Playing alarm sound, priority" << static_cast<int>(*m_priority) << "status"
                            << next->status();
            next->play();
        }
    } else {
        qCInfo(lcSound) << "Alarm sound off";
    }
}

} // namespace vsm::app
