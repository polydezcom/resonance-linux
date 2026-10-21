#include "core/audio/audio_engine.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/player_controller.h"
#include "ui/controllers/track_list_model.h"

#include <QtTest>

#include <QDataStream>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <cmath>
#include <cstdint>
#include <numbers>

using resonance::AppSettings;
using resonance::AudioEngine;
using resonance::PlayerController;
using resonance::TrackListModel;

namespace {

void writeTestWav(const QString &path, double seconds, double frequency)
{
    constexpr std::uint32_t sampleRate = 48000;
    constexpr std::uint16_t channels = 2;
    const std::uint32_t frameCount =
        static_cast<std::uint32_t>(sampleRate * seconds);
    const std::uint32_t dataSize = frameCount * channels * 2;

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataSize);
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << quint32(16) << quint16(1) << quint16(channels) << quint32(sampleRate)
        << quint32(sampleRate * channels * 2) << quint16(channels * 2)
        << quint16(16);
    out.writeRawData("data", 4);
    out << quint32(dataSize);
    for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
        const double t = static_cast<double>(frame) / sampleRate;
        const auto sample = static_cast<qint16>(
            std::sin(2.0 * std::numbers::pi * frequency * t) * 8000.0);
        out << sample << sample;
    }
}

} // namespace

class TestPlayerController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void queueInsertAndNavigation();
};

void TestPlayerController::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QStandardPaths::writableLocation(
                           QStandardPaths::TempLocation));
    QCoreApplication::setOrganizationName(QStringLiteral("ResonanceTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ResonanceTests"));
}

void TestPlayerController::queueInsertAndNavigation()
{
    qputenv("RESONANCE_NULL_AUDIO", "1");

    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QStringList files;
    for (int i = 0; i < 3; ++i) {
        const QString path = temp.filePath(QStringLiteral("track%1.wav").arg(i));
        writeTestWav(path, 3.0, 300.0 + i * 100.0);
        files.append(path);
    }

    AudioEngine engine;
    std::string error;
    if (!engine.initialize(&error))
        QSKIP(qPrintable(QString::fromStdString(error)));

    AppSettings settings;
    PlayerController player(&engine, &settings);
    TrackListModel model;
    model.setFiles(QString(), files);
    player.setTrackModel(&model);

    player.setQueue(files, 0, 0, true);
    QTRY_VERIFY_WITH_TIMEOUT(player.playing(), 2000);
    QCOMPARE(player.queueSize(), 3);
    QCOMPARE(player.queueIndex(), 0);
    QCOMPARE(player.path(), files.at(0));

    player.playNext(files.at(2));
    QCOMPARE(player.queueSize(), 4);
    QCOMPARE(player.path(), files.at(0));

    player.next();
    QTRY_COMPARE_WITH_TIMEOUT(player.path(), files.at(2), 2000);
    QCOMPARE(player.queueIndex(), 1);
    player.stop();
}

QTEST_MAIN(TestPlayerController)
#include "test_player_controller.moc"
