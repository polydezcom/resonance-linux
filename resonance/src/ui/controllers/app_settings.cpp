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
