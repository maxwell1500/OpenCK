#ifndef WTHRRECORD_HPP
#define WTHRRECORD_HPP

#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct WthrRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString sunTexture;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const WthrRecord& l, const WthrRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags && l.sunTexture == r.sunTexture
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const WthrRecord& l, const WthrRecord& r)
{
    return !(l == r);
}

#endif // WTHRRECORD_HPP
