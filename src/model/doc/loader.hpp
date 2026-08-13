#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QPair>
#include <QVector>

#include <memory>

class Document;

class Loader : public QObject
{
    Q_OBJECT

    struct Stage
    {
        int file;
        int recordsLoaded;
        bool recordsLeft;

        Stage();
    };

    // Owned by the loader thread only: loadDocument()/abortLoading()/load()
    // all run on that thread via queued connections, so no lock is needed.
    QVector<QPair<Document*, Stage>> documents;

public:
    Loader();

public slots:
    void loadDocument(Document* document);
    void abortLoading(Document* document);

signals:
    void documentLoaded(Document* document);
    void documentNotLoaded(Document* document, const QString& error);
    void nextRecord(Document* document, int records);
    void nextStage(Document* document, const QString& name, int records);
    void loadMessage(Document* document, const QString& message);

public slots:
    void load();
};

#endif // LOADER_H