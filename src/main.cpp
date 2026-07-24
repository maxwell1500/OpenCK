#include "editor.hpp"
#include "logger.hpp"
#include "filepaths.hpp"
#include "view/window/thememanager.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QString logDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    QString logFile = logDir + "/openck_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";

    OpenCK::Logging::Logger::instance().init(logFile);
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Info, "=== OpenCK Starting ===");
    OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Info, QString("Log file: %1").arg(logFile));
    
    try
    {
        QApplication a(argc, argv);
        a.setApplicationName("OpenCK");
        a.setOrganizationName("OpenCK");

        QString configPath = FilePaths::configFilePath();
        QSettings conf(configPath, QSettings::IniFormat);
        conf.beginGroup("OpenCK");
        QString themeName = conf.value("Theme", "Dark").toString();
        conf.endGroup();

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
