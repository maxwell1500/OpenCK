#include "editor.hpp"
#include "logger.hpp"
#include "filepaths.hpp"
#include "cli.hpp"
#include "view/window/thememanager.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QString logDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    const QString logDirOverride = QString::fromLocal8Bit(qgetenv("OPENCK_LOG_DIR"));
    if (!logDirOverride.isEmpty())
        logDir = logDirOverride;
    QString logFile = logDir + "/openck_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";

    OpenCK::Logging::Logger::instance().init(logFile);
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Info, "=== OpenCK Starting ===");
    OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Info, QString("Log file: %1").arg(logFile));

    // Headless command-line mode: no GUI is created, so use a QCoreApplication.
    QStringList rawArgs;
    for (int i = 1; i < argc; ++i)
    {
        rawArgs << QString::fromLocal8Bit(argv[i]);
    }
    if (rawArgs.contains(QStringLiteral("--cli")))
    {
        QCoreApplication a(argc, argv);
        a.setApplicationName("OpenCK");
        a.setOrganizationName("OpenCK");
        return OpenCK::Cli::run(argc, argv);
    }
    
    try
    {
        QApplication a(argc, argv);
        a.setApplicationName("OpenCK");
        a.setOrganizationName("OpenCK");

        QString configPath = FilePaths::configFilePath();
        QSettings conf(configPath, QSettings::IniFormat);
        conf.beginGroup("OpenCK");
        QString themeName = conf.value("Theme", "Dark").toString();
        QString language = conf.value("Language", QString()).toString();
        conf.endGroup();

        // Load the UI translation if one is available. The language code is
        // taken from settings (e.g. "fr"), falling back to the system locale.
        if (language.isEmpty()) {
            language = QLocale::system().name();
        }
        QString qmPath = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath()
            + QStringLiteral("/OpenCK_") + language + QStringLiteral(".qm");
        if (language != QLatin1String("en") && language != QLatin1String("en_US")
            && QFileInfo::exists(qmPath)) {
            auto* translator = new QTranslator(&a);
            if (translator->load(qmPath)) {
                a.installTranslator(translator);
            }
        }

        ThemeManager::Theme theme = ThemeManager::themeFromName(themeName);
        ThemeManager::applyTheme(a, theme);
        
        Editor w(argc, argv);
        return a.exec();
    }
    catch (const std::exception& e)
    {
        OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Error, QString("Exception: %1").arg(e.what()));
        return 1;
    }
    catch (...)
    {
        OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Fatal, "Unknown exception");
        return 1;
    }
}
