#ifndef ScptRECORD_H
#define ScptRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
#include <QByteArray>
class ESMReader;
class ESMWriter;

struct ScriptRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString scriptText;
    QByteArray compiledData;
    quint16 scriptType = 0;
    QVector<quint32> refObjects;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ScriptRecord& l, const ScriptRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.scriptText == r.scriptText && l.compiledData == r.compiledData
        && l.scriptType == r.scriptType && l.refObjects == r.refObjects
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const ScriptRecord& l, const ScriptRecord& r) { return !(l == r); }
#endif
