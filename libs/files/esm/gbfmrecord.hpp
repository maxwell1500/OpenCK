#ifndef GbfmRECORD_H
#define GbfmRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QStringList>
#include <QVector>
class ESMReader;
class ESMWriter;

// A parsed BFCB component: the marker's component-type name (the BFCB
// payload, a NUL-terminated string such as "TESFullName_Component") plus the
// subrecords that follow it until the next BFCB/BFCE. This is a *derived
// view*: storage stays in GbfmRecord::rawSubRecords in on-disk order, so the
// record still round-trips byte-for-byte; the view exists so callers can read
// the typed fields (full name, keywords, form links) without decoding the
// whole component catalogue.
struct GbfmComponent
{
    QString typeName;
    QVector<RawSubRecord> subrecords;

    bool hasSubrecord(NAME name) const;
    const RawSubRecord* findSubrecord(NAME name) const;
    // First little-endian uint32 of the named subrecord, or 0 when absent.
    quint32 firstU32(NAME name) const;
    // Every little-endian uint32 in the named subrecord (e.g. KWDA/FLFM lists).
    QVector<quint32> u32List(NAME name) const;
};

struct GbfmRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();

    // --- Derived BFCB component view (Starfield §3.8 ship composite) ---
    // Splits rawSubRecords at BFCB markers. Subrecords before the first BFCB
    // (EDID/OBND/ODTY/OPDS) and after a BFCE are reported through
    // leadingSubrecords()/trailingSubrecords() rather than as components.
    QVector<GbfmComponent> parseComponents() const;
    void splitComponents(QVector<GbfmComponent>& outPieces,
                         QVector<RawSubRecord>& outLeading,
                         QVector<RawSubRecord>& outTrailing) const;

    static const GbfmComponent* findComponent(const QVector<GbfmComponent>& comps,
                                              const QString& typeName);
    // Component type names present, in on-disk order.
    QStringList componentTypeNames() const;

    // --- Typed extraction from the standard components ---
    // TESFullName_Component::FULL — a string-table id (not a literal).
    quint32 fullNameStringId() const;
    // BGSKeywordForm_Component::KWDA (KSIZ gives the declared count).
    QVector<quint32> keywordFormIds() const;
    // BGSFormLinkData_Component::ITMC — number of linked-form pairs.
    int itemCount() const;
    // BGSFormLinkData_Component::FLFM — linked form ids, in order.
    QVector<quint32> linkedFormIds() const;
    // BGSFormLinkData_Component::FLKW — the keyword each linked form carries.
    QVector<quint32> linkedKeywordIds() const;

    // --- On-disk order helpers (BFCB is preserved as a raw subrecord; the
    // editor view above is derived, not stored) ---
    int rawSubrecordIndex(NAME name) const;
};

inline bool operator==(const GbfmRecord& l, const GbfmRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const GbfmRecord& l, const GbfmRecord& r)
{
    return !(l == r);
}
#endif
