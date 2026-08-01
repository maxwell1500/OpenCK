#ifndef COVERAGEVALIDATOR_H
#define COVERAGEVALIDATOR_H

#include "validator.hpp"

#include "../world/data.hpp"
#include "../world/irecordcollection.hpp"

#include <QHash>

// Scans every collection in the document for records with an empty
// Editor ID or a duplicate Editor ID. Unlike the per-type validators
// (NPC, WEAP, QUST) this covers all record types generically through
// Data::allCollectionsWithTypes.
class CoverageValidator : public Validator
{
public:
    QString name() const override { return "Coverage Validation"; }

    void validate(const Data& data, Messages& messages) override
    {
        const QVector<Data::TypedCollection> collections = data.allCollectionsWithTypes();

        for (const auto& entry : collections)
        {
            const IRecordCollection* collection = entry.collection;
            if (!collection)
                continue;

            QHash<QString, int> editorIdMap;

            for (int i = 0; i < collection->count(); ++i)
            {
                const QString editorId = collection->getEditorId(i);
                CkId ckId(entry.type, editorId);

                if (editorId.isEmpty())
                {
                    messages.append(ckId, "Record has an empty Editor ID.",
                        "Give the record a unique Editor ID.", Message::Warning);
                    continue;
                }

                auto it = editorIdMap.constFind(editorId);
                if (it != editorIdMap.constEnd())
                {
                    messages.append(ckId,
                        "Duplicate Editor ID '" + editorId + "' found.",
                        "This Editor ID already exists at index " + QString::number(it.value()) + ".",
                        Message::Warning);
                }
                else
                {
                    editorIdMap.insert(editorId, i);
                }
            }
        }
    }
};

#endif // COVERAGEVALIDATOR_H
