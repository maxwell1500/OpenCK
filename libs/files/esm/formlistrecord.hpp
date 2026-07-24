#ifndef FlstRECORD_H
#define FlstRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

struct FormListRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<quint32> formIds;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const FormListRecord& l, const FormListRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.formIds == r.formIds
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const FormListRecord& l, const FormListRecord& r) { return !(l == r); }
#endif
