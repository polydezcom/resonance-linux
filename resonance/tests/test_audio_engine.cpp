#include "core/audio/audio_engine.h"

#include <QtTest>

#include <QDataStream>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>
#include <cstdint>
#include <numbers>

using resonance::AudioEngine;

namespace {

void writeTestWav(const QString &path, double seconds)
{
    constexpr std::uint32_t sampleRate = 48000;
    constexpr std::uint16_t channels = 2;
    constexpr std::uint16_t bitsPerSample = 16;
    const std::uint32_t frameCount =
        static_cast<std::uint32_t>(sampleRate * seconds);
    const std::uint32_t dataSize =
        frameCount * channels * (bitsPerSample / 8);

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataSize);
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << quint32(16);
    out << quint16(1);
    out << quint16(channels);
    out << quint32(sampleRate);
    out << quint32(sampleRate * channels * (bitsPerSample / 8));
    out << quint16(channels * (bitsPerSample / 8));
    out << quint16(bitsPerSample);
    out.writeRawData("data", 4);
    out << quint32(dataSize);

    for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
        const double t = static_cast<double>(frame) / sampleRate;
        const auto sample = static_cast<qint16>(
            std::sin(2.0 * std::numbers::pi * 440.0 * t) * 12000.0);
        out << sample << sample;
    }
}

} // namespace

class TestAudioEngine : public QObject
{
    Q_OBJECT

private slots:
    void playbackLifecycle();
};

void TestAudioEngine::playbackLifecycle()
{
    qputenv("RESONANCE_NULL_AUDIO", "1");

    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString wavPath = temp.filePath(QStringLiteral("tone.wav"));
    writeTestWav(wavPath, 3.0);

    AudioEngine engine;
    std::string error;
    if (!engine.initialize(&error))
        QSKIP(qPrintable(QString::fromStdString(error)));

    QVERIFY(engine.isInitialized());
    QVERIFY(engine.sampleRate() > 0);

    engine.setVolume(0.35f);
    QCOMPARE(engine.volume(), 0.35f);

    engine.playFile(wavPath.toStdString());
    QTRY_VERIFY_WITH_TIMEOUT(engine.isPlaying(), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(engine.durationSeconds() > 2.9, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(engine.positionSeconds() > 0.1, 3000);

    engine.pause();
    QTRY_VERIFY_WITH_TIMEOUT(engine.isPaused(), 1000);
    const double pausedPosition = engine.positionSeconds();
    QTest::qWait(150);
    QVERIFY2(std::abs(engine.positionSeconds() - pausedPosition) < 0.10,
             "position moved while paused");

    engine.resume();
    QTRY_VERIFY_WITH_TIMEOUT(engine.isPlaying(), 1000);

    engine.seekToSeconds(1.5);
    QTRY_VERIFY_WITH_TIMEOUT(
        std::abs(engine.positionSeconds() - 1.5) < 0.25, 3000);

    QTRY_VERIFY_WITH_TIMEOUT(engine.hasEnded(), 6000);
    QVERIFY(std::abs(engine.positionSeconds() - engine.durationSeconds()) < 0.10);

    engine.stop();
    QVERIFY(engine.positionSeconds() < 0.01);
    QVERIFY(engine.takeError().empty());
}

QTEST_GUILESS_MAIN(TestAudioEngine)
#include "test_audio_engine.moc"
