#include "editor.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>

Editor::Editor(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("OpenCK");
    QCoreApplication::setOrganizationName("OpenCK");

    docMed.reset(new DocumentMediator());
    connect(this, &Editor::clearFilesSignal, docMed.get(), &DocumentMediator::clearFiles);
    connect(this, &Editor::addDocumentSignal, docMed.get(), &DocumentMediator::addDocument);
    connect(this, &Editor::saveDocumentSignal, docMed.get(), &DocumentMediator::saveFile);

    viewMed.reset(new ViewMediator(*docMed.get()));
    QString dataPath{ getDataPath("OpenCK") };
    viewMed->setUpDataDialog(dataPath);
    connect(viewMed.get(), &ViewMediator::addDocument, this, &Editor::addDocument);
    connect(viewMed.get(), &ViewMediator::saveDocument, this, &Editor::saveDocument);
}

Editor::~Editor()
{
}

QString Editor::getDataPath(const QString& applicationName)
{
    FilePaths paths{ applicationName };
    QSettings conf{ paths.configPath, QSettings::IniFormat };

    // Try to read saved GameId first, then use game-specific key
    conf.beginGroup(applicationName);
    int savedGameId = conf.value("GameId", -1).toInt();
    QString dataPath;

    if (savedGameId >= 0 && savedGameId < Game_NumGames)
    {
        GameId gameId = static_cast<GameId>(savedGameId);
        QString specificKey = FilePaths::dataDirKey(gameId);
        dataPath = conf.value(specificKey, paths.dataDir.path()).toString();
    }
    else
    {
        dataPath = conf.value("DataDirectory", paths.dataDir.path()).toString();
    }
    conf.endGroup();

    // If the stored path doesn't exist, try auto-detection
    if (!QDir(dataPath).exists())
    {
        auto detected = FilePaths::detectGames();
        for (const auto& game : detected)
        {
            if (QDir(game.dataPath).exists())
            {
                dataPath = game.dataPath;
                // Save detected path with game-specific key
                QSettings writer(paths.configPath, QSettings::IniFormat);
                writer.beginGroup(applicationName);
                writer.setValue("GameId", static_cast<int>(game.gameId));
                writer.setValue(FilePaths::dataDirKey(game.gameId), dataPath);
                writer.endGroup();
                break;
            }
        }
    }

    return dataPath;
}

void Editor::addDocument(const QStringList& files, const QString& savePath, bool isNew)
{
    emit clearFilesSignal();
    emit addDocumentSignal(files, savePath, isNew);
}

void Editor::saveDocument(const QString& path)
{
    emit saveDocumentSignal(path);
}
