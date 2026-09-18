#ifndef MrhpRECORD_H
#define MrhpRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct MrhpRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    // Typed fields (surveyed from Starfield.esm: 998 records, 4 subrecord types)
    QString morphPath;        // TCMP — folder containing morph.dat
    quint32 mobcFlags = 0xFF; // MOBC — always 0xFF000000 in surveyed data
    QString templatePath;     // TMPP — optional template morph folder
    QVector<RawSubRecord> rawSubRecords;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<MrhpRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const MrhpRecord& l, const MrhpRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.morphPath == r.morphPath && l.mobcFlags == r.mobcFlags
        && l.templatePath == r.templatePath
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const MrhpRecord& l, const MrhpRecord& r)
{
    return !(l == r);
}
#endif
