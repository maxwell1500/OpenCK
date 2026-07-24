#ifndef QUESTVALIDATOR_H
#define QUESTVALIDATOR_H

#include "validator.hpp"

#include "../world/idcollection.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"
#include "../../../libs/files/esm/questrecord.hpp"

#include <QMap>

class QuestValidator : public Validator
{
public:
    QString name() const override { return "Quest Validation"; }

    void validate(const Data& data, Messages& messages) override
    {
        const auto& questCollection = data.getQuestCollection();

        QMap<QString, int> editorIdMap;

        for (int i = 0; i < questCollection.size(); ++i)
        {
            const Record<QuestRecord>& questRec = questCollection.getRecord(i);
            const QuestRecord& quest = questRec.get();

            CkId ckId(CkId::Type_Quest_, quest.editorId);

            if (quest.editorId.isEmpty())
            {
                messages.append(ckId, "Quest has an empty EditorID.", "", Message::Error);
            }
            else
            {
                auto it = editorIdMap.find(quest.editorId);
                if (it != editorIdMap.end())
                {
                    messages.append(ckId,
                        "Duplicate EditorID '" + quest.editorId + "' found.",
                        "This EditorID already exists at index " + QString::number(it.value()),
                        Message::Error);
                }
                else
                {
                    editorIdMap.insert(quest.editorId, i);
                }
            }

            if (quest.stageIds.isEmpty())
            {
                messages.append(ckId,
                    "Quest has no stages defined.",
                    "Add at least one stage to the quest.",
                    Message::Error);
            }

            for (int si = 0; si < quest.stageIds.size(); ++si)
            {
                quint32 stageId = quest.stageIds[si];
                if (stageId == 0)
                {
                    messages.append(ckId,
                        "Stage at index " + QString::number(si) + " has an invalid ID (0).",
                        "Set a valid stage ID.",
                        Message::Error);
                }
            }

            if (quest.objectiveIds.size() > 0)
            {
                for (int oi = 0; oi < quest.objectiveIds.size(); ++oi)
                {
                    quint32 objId = quest.objectiveIds[oi];
                    bool valid = false;
                    for (int si = 0; si < quest.stageIds.size(); ++si)
                    {
                        if (quest.stageIds[si] == objId)
                        {
                            valid = true;
                            break;
                        }
                    }
                    if (!valid)
                    {
                        messages.append(ckId,
                            "Objective at index " + QString::number(oi) + " references stage ID 0x" + QString::number(objId, 16) + " which does not exist in this quest's stages.",
                            "Add this stage ID or remove the objective.",
                            Message::Warning);
                    }
                }
            }
        }
    }
};

#endif // QUESTVALIDATOR_H
