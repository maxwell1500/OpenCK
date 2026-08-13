#include "loader.hpp"

#include "document.hpp"
#include "documentstate.hpp"
#include "messages.hpp"
#include "../tools/reports.hpp"
#include "logger.hpp"

#include <iostream>
#include <stdexcept>
#include <QElapsedTimer>
#include <QThread>

Loader::Stage::Stage() :
    file(0), recordsLoaded(0), recordsLeft(false)
{
}

Loader::Loader()
{
    LOG_INFO("Loader initialized");
}

// Drives the load pipeline until either the queue is drained or ~15 ms of a
// batch have elapsed, then re-queues itself so queued progress signals reach
// the UI. Called from the mediator's tick timer and from the self-resume
// below; both land on the loader thread and are serialized there.
void Loader::load()
{
    if (documents.empty())
    {
        return;
    }

    QElapsedTimer budget;
    budget.start();

    while (!documents.empty())
    {
        auto it = documents.begin();
        Document* document = it->first;

        int size = static_cast<int>(document->getContentFiles().size());
        int editedIndex = size - 1;

        try
        {
            if (it->second.recordsLeft)
            {
                LOG_DEBUG(QString("Loading batch for %1, recordsLoaded=%2")
                    .arg(document->getSavePath()).arg(it->second.recordsLoaded));
                Messages messages(Message::Error);
                int batch = 0;

                while (true)
                {
                    if (document->getData().continueLoading(messages))
                    {
                        LOG_DEBUG(QString("Batch loading complete for %1")
                            .arg(document->getSavePath()));
                        it->second.recordsLeft = false;
                        break;
                    }
                    ++(it->second.recordsLoaded);
                    ++batch;
                    // Yield once the time budget is spent so the UI gets its
                    // progress signals; the remaining records continue on the
                    // next event-loop pass.
                    if (batch >= 64 && budget.elapsed() > 15)
                    {
                        break;
                    }
                }

                for (Messages::Iterator messageIt = messages.begin(); messageIt != messages.end(); ++messageIt)
                {
                    document->getReport()->add(*messageIt);
                    emit loadMessage(document, messageIt->message);
                }

                emit nextRecord(document, it->second.recordsLoaded);

                if (it->second.recordsLeft)
                {
                    QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
                    return;
                }

                ++(it->second.file);
                continue;
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
                continue;
            }

            LOG_INFO(QString("All files loaded for %1").arg(document->getSavePath()));
            documents.erase(it);
            emit documentLoaded(document);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(QString("Error loading %1: %2").arg(document->getSavePath()).arg(e.what()));
            documents.erase(it);
            emit documentNotLoaded(document, e.what());
        }
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
