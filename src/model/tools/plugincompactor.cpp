#include "plugincompactor.hpp"

#include <algorithm>

#include "libs/files/esm/refrecord.hpp"
#include "libs/files/esm/cellrecord.hpp"
#include "src/model/world/irecordcollection.hpp"
#include "libs/files/log/logger.hpp"

QVector<quint32> PluginCompactor::collectFormIds(
    const QVector<const IRecordCollection*>& collections)
{
    QVector<quint32> ids;
    for (const IRecordCollection* collection : collections)
    {
        if (!collection)
            continue;
        const int count = collection->count();
        for (int i = 0; i < count; ++i)
        {
            const quint32 id = collection->getFormId(i);
            if (id != 0 && !ids.contains(id))
                ids.append(id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

PluginCompactor::RenumberMap PluginCompactor::buildMap(
    const QVector<quint32>& formIds)
{
    RenumberMap map;
    QVector<quint32> sorted = formIds;
    std::sort(sorted.begin(), sorted.end());
    for (const quint32 oldId : sorted)
    {
        if (oldId == 0)
            continue;
        // Keep the master-index byte (top 8 bits), renumber the local part
        // (bottom 24 bits) densely.
        const quint32 master = oldId & 0xFF000000u;
        const quint32 newLocal = static_cast<quint32>(map.renumbered + 1);
        const quint32 newId = master | (newLocal & 0x00FFFFFFu);
        map.oldToNew.insert(oldId, newId);
        ++map.renumbered;
    }
    return map;
}

quint32 PluginCompactor::remap(const RenumberMap& map, quint32 oldFormId)
{
    const auto it = map.oldToNew.constFind(oldFormId);
    return it != map.oldToNew.constEnd() ? it.value() : oldFormId;
}

quint32 PluginCompactor::renumberId(const RenumberMap& map, quint32 oldFormId,
                                    Result& result)
{
    const quint32 newId = remap(map, oldFormId);
    if (newId != oldFormId)
        ++result.renumbered;
    return newId;
}

void PluginCompactor::repointRefr(RefrRecord& record, const RenumberMap& map,
                                  Result& result)
{
    const quint32 newBase = renumberId(map, record.baseId, result);
    if (newBase != record.baseId)
        ++result.repointedReferences;
    record.baseId = newBase;

    const quint32 newOwner = renumberId(map, record.owner, result);
    if (newOwner != record.owner)
        ++result.repointedReferences;
    record.owner = newOwner;

    for (quint32& scriptId : record.scriptIds)
    {
        const quint32 newScriptId = renumberId(map, scriptId, result);
        if (newScriptId != scriptId)
            ++result.repointedReferences;
        scriptId = newScriptId;
    }
}

void PluginCompactor::repointCell(CellRecord& record, const RenumberMap& map,
                                  Result& result)
{
    const quint32 newOwner = renumberId(map, record.owner, result);
    if (newOwner != record.owner)
        ++result.repointedReferences;
    record.owner = newOwner;
}
