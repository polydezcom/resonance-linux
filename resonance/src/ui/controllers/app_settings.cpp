#include "ui/controllers/app_settings.h"

#include <QColor>
#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>

#include <algorithm>
#include <cmath>

namespace resonance {

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    QSettings settings;

    if (settings.contains(QStringLiteral("ui/dark"))) {
        m_dark = settings.value(QStringLiteral("ui/dark")).toBool();
    } else {
        const auto scheme = QGuiApplication::styleHints()->colorScheme();
        m_dark = scheme != Qt::ColorScheme::Light;
    }

    const QString storedAccent =
        settings.value(QStringLiteral("ui/accent"), m_accent).toString();
    if (QColor(storedAccent).isValid())
        m_accent = storedAccent;

    m_volume = std::clamp(
        settings.value(QStringLiteral("player/volume"), m_volume).toDouble(),
        0.0, 1.0);
    m_lastFolder =
        settings.value(QStringLiteral("library/lastFolder")).toString();
    m_lastSelectedFolder =
        settings.value(QStringLiteral("library/lastSelectedFolder")).toString();
    m_lastTrack =
        settings.value(QStringLiteral("library/lastTrack")).toString();
    m_lastPositionMs =
        settings.value(QStringLiteral("library/lastPositionMs"), 0).toLongLong();
    m_shuffle = settings.value(QStringLiteral("player/shuffle"), false).toBool();
    m_repeatMode = std::clamp(
        settings.value(QStringLiteral("player/repeatMode"), 0).toInt(), 0, 2);
    m_toolsOpen = settings.value(QStringLiteral("ui/toolsOpen"), false).toBool();
}

void AppSettings::setDark(bool dark)
{
    if (m_dark == dark)
        return;

    m_dark = dark;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/dark"), m_dark);
    emit darkChanged();
}

void AppSettings::setAccent(const QString &accent)
{
    const QColor color(accent);
    if (!color.isValid())
        return;

    const QString normalized = color.name(QColor::HexRgb);
    if (m_accent.compare(normalized, Qt::CaseInsensitive) == 0)
        return;

    m_accent = normalized;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/accent"), m_accent);
    emit accentChanged();
}

void AppSettings::setVolume(double volume)
{
    const double clamped = std::clamp(volume, 0.0, 1.0);
    if (std::abs(m_volume - clamped) < 1.0e-6)
        return;

    m_volume = clamped;
    QSettings settings;
    settings.setValue(QStringLiteral("player/volume"), m_volume);
    emit volumeChanged();
}

void AppSettings::setLastFolder(const QString &folder)
{
    if (m_lastFolder == folder)
        return;

    m_lastFolder = folder;
    QSettings settings;
    settings.setValue(QStringLiteral("library/lastFolder"), m_lastFolder);
    emit lastFolderChanged();
}

void AppSettings::setLastSelectedFolder(const QString &folder)
{
    if (m_lastSelectedFolder == folder)
        return;

    m_lastSelectedFolder = folder;
    QSettings settings;
    settings.setValue(QStringLiteral("library/lastSelectedFolder"),
                      m_lastSelectedFolder);
    emit lastSelectedFolderChanged();
}

void AppSettings::setLastTrack(const QString &path)
{
    if (m_lastTrack == path)
        return;

    m_lastTrack = path;
    QSettings settings;
    settings.setValue(QStringLiteral("library/lastTrack"), m_lastTrack);
    emit lastTrackChanged();
}

void AppSettings::setLastPositionMs(qint64 positionMs)
{
    const qint64 clamped = std::max<qint64>(0, positionMs);
    if (m_lastPositionMs == clamped)
        return;

    m_lastPositionMs = clamped;
    QSettings settings;
    settings.setValue(QStringLiteral("library/lastPositionMs"), m_lastPositionMs);
    emit lastPositionChanged();
}

void AppSettings::setShuffle(bool shuffle)
{
    if (m_shuffle == shuffle)
        return;

    m_shuffle = shuffle;
    QSettings settings;
    settings.setValue(QStringLiteral("player/shuffle"), m_shuffle);
    emit shuffleChanged();
}

void AppSettings::setRepeatMode(int mode)
{
    const int clamped = std::clamp(mode, 0, 2);
    if (m_repeatMode == clamped)
        return;

    m_repeatMode = clamped;
    QSettings settings;
    settings.setValue(QStringLiteral("player/repeatMode"), m_repeatMode);
    emit repeatModeChanged();
}

void AppSettings::setToolsOpen(bool open)
{
    if (m_toolsOpen == open)
        return;

    m_toolsOpen = open;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/toolsOpen"), m_toolsOpen);
    emit toolsOpenChanged();
}

} // namespace resonance
