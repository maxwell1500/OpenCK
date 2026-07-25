#ifndef TreeRECORD_H
#define TreeRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct TreeRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    QString modelPath;
    float leafCurvature = 0.0f;
    float leafAmplitude = 0.0f;
    QString lodModelPath;
    quint32 lodFlags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const TreeRecord& l, const TreeRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.leafCurvature == r.leafCurvature && l.leafAmplitude == r.leafAmplitude
        && l.lodModelPath == r.lodModelPath && l.lodFlags == r.lodFlags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const TreeRecord& l, const TreeRecord& r)
{
    return !(l == r);
}
#endif
