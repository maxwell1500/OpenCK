#ifndef ShouRECORD_H
#define ShouRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

struct ShoutWord {
    quint32 wordFormId = 0;
    quint32 spellFormId = 0;
    float recoveryTime = 0.0f;

    inline bool operator==(const ShoutWord& o) const {
        return wordFormId == o.wordFormId && spellFormId == o.spellFormId
            && recoveryTime == o.recoveryTime;
    }
    inline bool operator!=(const ShoutWord& o) const { return !(*this == o); }
};

struct ShouRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString fullName;
    QVector<ShoutWord> words;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ShouRecord& l, const ShouRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.fullName == r.fullName && l.words == r.words
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ShouRecord& l, const ShouRecord& r)
{
    return !(l == r);
}
#endif
