#include "loader.hpp"

#include "document.hpp"
#include "documentstate.hpp"
#include "messages.hpp"
#include "../tools/reports.hpp"
#include "logger.hpp"

#include <iostream>
#include <stdexcept>

Loader::Stage::Stage() :
    file(0), recordsLoaded(0), recordsLeft(false)
{
}

Loader::Loader() :
    shouldStop(false)
{
    LOG_INFO("Loader initialized");
    timer = std::make_unique<QTimer>(this);

    connect(timer.get(), &QTimer::timeout, this, &Loader::load);
    timer->start();
}

QWaitCondition& Loader::hasThingsToDo()
{
    return toDo;
}

void Loader::stop()
{
    shouldStop = true;
}

void Loader::load()
{
    if (documents.empty())
    {
        mutex.lock();
        toDo.wait(&mutex);
        mutex.unlock();

        if (shouldStop)
        {
            LOG_DEBUG("Loader stopping due to shouldStop flag");
            timer->stop();
        }

        return;
    }

    auto it = documents.begin();
    Document* document = it->first;

    int size = static_cast<int>(document->getContentFiles().size());
    int editedIndex = size - 1;
    bool done = false;

    LOG_DEBUG(QString("Loader processing %1 document(s)").arg(documents.size()));

    try
    {
        if (it->second.recordsLeft)
        {
            LOG_DEBUG(QString("Loading batch for %1, recordsLoaded=%2").arg(document->getSavePath()).arg(it->second.recordsLoaded));
            Messages messages(Message::Error);
            const int batchSize = 50;

            for (int i = 0; i < batchSize; i++)
            {
                if (document->getData().continueLoading(messages))
                {
                    LOG_DEBUG(QString("Batch loading complete for %1").arg(document->getSavePath()));
                    it->second.recordsLeft = false;
                    break;
                }
                else
                {
                    ++(it->second.recordsLoaded);
                }
            }

            for (Messages::Iterator messageIt = messages.begin(); messageIt != messages.end(); ++messageIt)
            {
                document->getReport()->add(*messageIt);
                emit loadMessage(document, messageIt->message);
            }

            emit nextRecord(document, it->second.recordsLoaded);

            return;
        }

        if (it->second.file < size)
        {
            QString file = document->getContentFiles()[it->second.file];
            LOG_INFO(QString("Loading file: %1 (master=%2)").arg(file).arg(it->second.file == editedIndex ? "no" : "yes"));
            int recordCount = document->getData().preload(file, it->second.file != editedIndex);
            LOG_INFO(QString("File loaded: %1, %2 records").arg(file).arg(recordCount));

            it->second.recordsLeft = true;
            it->second.recordsLoaded = 0;

            emit nextStage(document, file, recordCount);
        }
        else
        {
            LOG_INFO(QString("All files loaded for %1").arg(document->getSavePath()));
            done = true;
        }

        ++(it->second.file);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(QString("Error loading %1: %2").arg(document->getSavePath()).arg(e.what()));
        documents.erase(it);
        emit documentNotLoaded(document, e.what());
        return;
    }

    if (done)
    {
        LOG_INFO(QString("Document loaded successfully: %1").arg(document->getSavePath()));
        documents.erase(it);
        emit documentLoaded(document);
    }
}

void Loader::loadDocument(Document* document)
{
    documents.push_back(QPair<Document*, Stage>(document, Stage()));
}

void Loader::abortLoading(Document* document)
{
    for (auto it = documents.begin(); it != documents.end(); ++it)
    {
        if (it->first == document)
        {
            documents.erase(it);
            emit documentNotLoaded(document, "");
            break;
        }
    }
}