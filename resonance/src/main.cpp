#include "core/audio/audio_engine.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/library_controller.h"
#include "ui/controllers/player_controller.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QDebug>
#include <QFileInfo>
#include <qqml.h>

using namespace resonance;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Resonance"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("resonance.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Resonance"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Resonance"));
    QGuiApplication::setDesktopFileName(QStringLiteral("resonance"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    AudioEngine audioEngine;
    std::string audioError;
    if (!audioEngine.initialize(&audioError))
        qWarning().noquote() << "Audio unavailable:" << QString::fromStdString(audioError);

    AppSettings appSettings;
    PlayerController player(&audioEngine, &appSettings);
    LibraryController library(&player, &appSettings);

    QString startupFolder;
    for (int i = 1; i < argc; ++i) {
        const QFileInfo info(QString::fromLocal8Bit(argv[i]));
        if (info.isDir()) {
            startupFolder = info.absoluteFilePath();
            break;
        }
    }
    library.setStartupFolder(startupFolder, true);

    qmlRegisterSingletonInstance("Resonance", 1, 0, "Player", &player);
    qmlRegisterSingletonInstance("Resonance", 1, 0, "Library", &library);
    qmlRegisterSingletonInstance("Resonance", 1, 0, "AppSettings", &appSettings);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Resonance", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
