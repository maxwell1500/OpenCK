#ifndef WEAPONVALIDATOR_H
#define WEAPONVALIDATOR_H

#include "validator.hpp"

#include "../world/idcollection.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"

#include <QMap>

class WeaponValidator : public Validator
{
public:
    QString name() const override { return "Weapon Validation"; }

    void validate(const Data& data, Messages& messages) override
    {
        const auto& weaponCollection = data.getWeaponCollection();
        const auto& enchCollection = data.getEnchCollection();

        QMap<QString, int> editorIdMap;

        for (int i = 0; i < weaponCollection.size(); ++i)
        {
            const Record<WeaponRecord>& weapRec = weaponCollection.getRecord(i);
            const WeaponRecord& weap = weapRec.get();

            CkId ckId(CkId::Type_Weap_, weap.editorId);

            if (weap.editorId.isEmpty())
            {
                messages.append(ckId, "Weapon has an empty EditorID.", "", Message::Error);
            }
            else
            {
                auto it = editorIdMap.find(weap.editorId);
                if (it != editorIdMap.end())
                {
                    messages.append(ckId,
                        "Duplicate EditorID '" + weap.editorId + "' found.",
                        "This EditorID already exists at index " + QString::number(it.value()),
                        Message::Error);
                }
                else
                {
                    editorIdMap.insert(weap.editorId, i);
                }
            }

            float damage = weap.damage;
            if (damage < 0 || damage > 99999)
            {
                messages.append(ckId,
                    "Damage value " + QString::number(damage) + " is invalid. Must be 0-99999.",
                    "",
                    Message::Error);
            }

            float speed = weap.speed;
            if (speed < 0 || speed > 999)
            {
                messages.append(ckId,
                    "Speed value " + QString::number(speed) + " is invalid. Must be 0-999.",
                    "",
                    Message::Error);
            }

            float weight = weap.weight;
            if (weight < 0 || weight > 999)
            {
                messages.append(ckId,
                    "Weight value " + QString::number(weight) + " is invalid. Must be 0-999.",
                    "",
                    Message::Error);
            }

            if (weap.value > 9999999)
            {
                messages.append(ckId,
                    "Value " + QString::number(weap.value) + " is invalid. Must be 0-9999999.",
                    "",
                    Message::Error);
            }

            if (weap.enchantment != 0)
            {
                if (enchCollection.size() > 0)
                {
                    bool enchExists = false;
                    for (int ei = 0; ei < enchCollection.size(); ++ei)
                    {
                        const Record<EnchRecord>& ench = enchCollection.getRecord(ei);
                        if (ench.get().formId == weap.enchantment)
                        {
                            enchExists = true;
                            break;
                        }
                    }
                    if (!enchExists)
                    {
                        messages.append(ckId,
                            "Enchantment references formID 0x" + QString::number(weap.enchantment, 16) + " which does not exist.",
                            "",
                            Message::Warning);
                    }
                }
            }
        }
    }
};

#endif // WEAPONVALIDATOR_H
