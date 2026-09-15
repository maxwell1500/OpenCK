#ifndef GMST_H
#define GMST_H

class ESMReader;
class ESMWriter;

#include "records.hpp"
#include "variant.hpp"

#include <QString>
#include <QVariant>
#include <QVector>

struct GameSetting
{
    QString editorId;
    quint32 formId = 0;
    Variant value;
    QVector<RawSubRecord> rawSubRecords;
    // Set when a DATA value subrecord was present at load. Some shipped
    // settings carry other subrecords (e.g. XALG) in its place; those must
    // stay raw and no DATA may be invented on save.
    bool hasData = false;

    void load(ESMReader& esm, bool base = false);
    void save(ESMWriter& esm) const;
    void blank();
};

bool operator==(const GameSetting& l, const GameSetting& r);

#endif // GMST_H
