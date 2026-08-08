#include "documentmediator.hpp"

#include "../../view/messageboxhelper.hpp"
#include "logger.hpp"

DocumentMediator::DocumentMediator()
{
    LOG_INFO("DocumentMediator initializing...");
    loader.moveToThread(&loaderThread);
    loaderThread.start();

    // The tick timer lives on the mediator's (main) thread and drives the
    // loader through a queued connection, so no timer is ever owned by the
    // loader thread itself. Destroying a timer started in another thread
    // QFATALs in Qt ("Cannot send events to objects owned by a different
    // thread"), so the loader must not own one.
    tickTimer.setInterval(20);
    connect(&tickTimer, &QTimer::timeout, &loader, &Loader::load);
    tickTimer.start();

    connect(&loader, &Loader::documentLoaded,
        this, &DocumentMediator::documentLoaded);

    connect(&loader, &Loader::documentNotLoaded,
        this, &DocumentMediator::documentNotLoaded);

    connect(this, &DocumentMediator::loadRequest, 
        &loader, &Loader::loadDocument);

    connect(&loader, &Loader::nextStage,
        this, &DocumentMediator::nextStage);

    connect(&loader, &Loader::nextRecord,
        this, &DocumentMediator::nextRecord);

    connect(this, &DocumentMediator::cancelLoading, 
        &loader, &Loader::abortLoading);

    connect(&loader, &Loader::loadMessage, 
        this, &DocumentMediator::loadMessage);
    
    LOG_INFO("DocumentMediator initialized");
}

DocumentMediator::~DocumentMediator()
{
    LOG_INFO("DocumentMediator shutting down...");
    tickTimer.stop();
    loaderThread.quit();
    loader.hasThingsToDo().wakeAll();
    loaderThread.wait();
    LOG_INFO("DocumentMediator shut down");
}

void DocumentMediator::clearFiles()
{
    LOG_INFO(QString("Clearing %1 document(s)").arg(documents.size()));
    documents.clear();
}

void DocumentMediator::addDocument(const QStringList& files, const QString& savePath, bool isNew)
{
    LOG_INFO(QString("Adding document: %1 files, isNew=%2").arg(files.size()).arg(isNew ? "true" : "false"));
    Document* document = makeDocument(files, savePath, isNew);
    
    insertDocument(document);
}

Document* DocumentMediator::makeDocument(const QStringList& files, const QString& savePath, bool isNew)
{
    LOG_DEBUG(QString("Creating document with %1 files").arg(files.size()));
    Document* doc = new Document(files, savePath, isNew);

    return doc;
}

void DocumentMediator::insertDocument(Document* document)
{
    LOG_DEBUG(QString("Inserting document: %1").arg(document->getSavePath()));
    documents.push_back(std::shared_ptr<Document>(document));

    emit loadRequest(document);

    loader.hasThingsToDo().wakeAll();
}

Document* DocumentMediator::getDocument(int index)
{
    LOG_DEBUG(QString("Getting document at index %1 (total: %2)").arg(index).arg(documents.size()));
    if (documents.size() <= index)
    {
        return nullptr;
    }

    return documents[index].get();
}

Document* DocumentMediator::getCurrentDocument()
{
    if (documents.isEmpty())
    {
        return nullptr;
    }

    Document* doc = documents.last().get();
    LOG_DEBUG(QString("Getting current document: %1").arg(doc->getSavePath()));
    return doc;
}

void DocumentMediator::saveFile(const QString& path)
{
    LOG_INFO(QString("Saving file to: %1").arg(path));
    if (documents.empty())
    {
        LOG_ERROR("Cannot save: no document open");
        msgBoxCritical("No file open, cannot save.");
    }
    else
    {
        documents.back().get()->save(path);
        LOG_INFO("File saved successfully");
    }
}

void DocumentMediator::setPaths(const FilePaths& filePaths)
{
    paths = filePaths;
}

void DocumentMediator::documentLoaded(Document* document)
{
    emit loadingStopped(document, true, QString(""));
}

void DocumentMediator::documentNotLoaded(Document* document, const QString& error)
{
    emit loadingStopped(document, false, error);

    if (error.isEmpty())
    {
        removeDocument(document);
    }
}

void DocumentMediator::removeDocument(Document* document)
{
    int index = -1;

    for (int i = 0; i < documents.length(); i++)
    {
        if (documents[i].get() == document)
        {
            index = i;
        }
    }

    documents.remove(index);
}
