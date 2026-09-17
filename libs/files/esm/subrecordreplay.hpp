#ifndef SUBRECORDREPLAY_H
#define SUBRECORDREPLAY_H

#include "records.hpp"
#include "esmwriter.hpp"

#include <QHash>
#include <QVector>

// Re-emits raw subrecords at the positions they occupied on disk. Typed
// fields are written by the caller when the load-order walk reaches their
// name; everything else goes through `write` so an untouched record keeps
// its exact subrecord order and duplicates.
struct SubrecordReplay
{
    void init(const QVector<RawSubRecord>& recordSubs)
    {
        raws = &recordSubs;
        byName.clear();
        cursor.clear();
        for (int i = 0; i < recordSubs.size(); ++i)
            byName[recordSubs[i].name].append(i);
    }

    bool write(NAME name, ESMWriter& esm)
    {
        if (!raws)
            return false;
        const QVector<int>& idx = byName[name];
        int& cur = cursor[name];
        if (cur < idx.size())
        {
            esm.writeRawSubRecord((*raws)[idx[cur++]]);
            return true;
        }
        return false;
    }

    void writeLeftover(ESMWriter& esm)
    {
        if (!raws)
            return;
        for (auto it = byName.constBegin(); it != byName.constEnd(); ++it)
        {
            const QVector<int>& idx = it.value();
            int& cur = cursor[it.key()];
            while (cur < idx.size())
                esm.writeRawSubRecord((*raws)[idx[cur++]]);
        }
    }

private:
    QHash<NAME, QVector<int>> byName;
    QHash<NAME, int> cursor;
    const QVector<RawSubRecord>* raws = nullptr;
};

#endif // SUBRECORDREPLAY_H
