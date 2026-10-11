#include "blankrecordfactory.hpp"

#include "../world/basecollection.hpp"

bool BlankRecordFactory::supports(BaseCollection* collection)
{
    return collection && collection->supportsBlankRecord();
}

std::unique_ptr<BaseRecord> BlankRecordFactory::create(BaseCollection* collection,
    const QString& editorId, quint32 formId)
{
    return collection ? collection->createBlankRecord(editorId, formId) : nullptr;
}
