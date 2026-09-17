#ifndef BookRECORD_H
#define BookRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct BookRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 pageCount = 0;
    QString pages;
    QString iconPath;
    QString modelPath;
    QVector<RawSubRecord> rawSubRecords;
    QVector<NAME> loadOrder;
    bool hasFlags = false;
    NAME flagsSpelling = NAME('FNAM');
    quint8 flagsWidth = 4;
    quint32 dataValue = 0;
    float dataWeight = 0.0f;
    bool hasData = false;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const BookRecord& l, const BookRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.pageCount == r.pageCount && l.pages == r.pages
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const BookRecord& l, const BookRecord& r)
{
    return !(l == r);
}
#endif
