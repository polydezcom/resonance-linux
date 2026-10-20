#include "library/cover_art_cache.h"
#include "library/metadata_reader.h"

#include <QtTest>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <cmath>
#include <cstdint>
#include <numbers>

using resonance::CoverArtCache;
using resonance::MetadataReader;

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
    out << quint32(16) << quint16(1) << quint16(channels) << quint32(sampleRate)
        << quint32(sampleRate * channels * (bitsPerSample / 8))
        << quint16(channels * (bitsPerSample / 8)) << quint16(bitsPerSample);
    out.writeRawData("data", 4);
    out << quint32(dataSize);

    for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
        const double t = static_cast<double>(frame) / sampleRate;
        const auto sample = static_cast<qint16>(
            std::sin(2.0 * std::numbers::pi * 440.0 * t) * 8000.0);
        out << sample << sample;
    }
}

} // namespace

class TestMetadata : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void fallbackTitleAndDuration();
    void sidecarCoverIsCached();
};

void TestMetadata::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("ResonanceTests"));
}

void TestMetadata::fallbackTitleAndDuration()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("01 - My Song.wav"));
    writeTestWav(path, 1.0);

    const QString fallback = MetadataReader::fallbackTitle(path);
    QCOMPARE(fallback, QStringLiteral("My Song"));

    const resonance::TrackMetadata metadata = MetadataReader::read(path);
    QCOMPARE(metadata.title, QStringLiteral("My Song"));
    QVERIFY(metadata.artist.isEmpty());
#ifdef RESONANCE_HAS_TAGLIB
    QVERIFY2(metadata.durationMs > 900,
             qPrintable(QStringLiteral("duration was %1 ms")
                            .arg(metadata.durationMs)));
#endif
}

void TestMetadata::sidecarCoverIsCached()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("song.wav"));
    writeTestWav(path, 0.2);

    QImage cover(32, 32, QImage::Format_RGB32);
    cover.fill(Qt::red);
    QVERIFY(cover.save(temp.filePath(QStringLiteral("cover.png"))));

    const QString cachePath = CoverArtCache::coverForTrack(path);
    QVERIFY(!cachePath.isEmpty());
    QVERIFY(QFileInfo::exists(cachePath));

    QImage cached(cachePath);
    QVERIFY(!cached.isNull());
    QCOMPARE(cached.size(), QSize(32, 32));

    // A second call should hit the cache path without changing it.
    QCOMPARE(CoverArtCache::coverForTrack(path), cachePath);
}

QTEST_GUILESS_MAIN(TestMetadata)
#include "test_metadata.moc"
