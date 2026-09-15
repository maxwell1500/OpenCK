#ifndef SHIPPARTCODEC_HPP
#define SHIPPARTCODEC_HPP

#include <QString>
#include <QStringList>
#include <QVector>

#include "../../../libs/files/esm/gbfmrecord.hpp"
#include "../../../libs/files/esm/constructibleobjectrecord.hpp"
#include "../../../libs/files/esm/formlistrecord.hpp"
#include "../world/idcollection.hpp"

// Starfield ship composite (REMAINING.md §3.8). There is no single SHIP
// record: a ship part is a GBFM built from BFCB components, and a part recipe
// is a COBJ (co_SMS_*) whose CNAM points at a FLST variant list, whose LNAM
// entries are the GBFM variants. Surveyed against Starfield.esm 2026-09-14:
// 3,141 GBFM / 3,009 COBJ / 1,142 FLST.
//
// The GBFM byte layout is preserved verbatim by GbfmRecord; this codec exposes
// the editor-relevant fields (identity, display-name string id, keywords, and
// the linked-form table that names the modules) without decoding the opaque
// components (Blueprint_Component's BUO4, the NVNM navmesh blob, etc.), which
// ride along untouched.
struct ShipPartDefinition
{
    QString editorId;
    quint32 formId = 0;
    quint32 nameStringId = 0;            // TESFullName_Component::FULL
    QVector<quint32> keywordFormIds;     // BGSKeywordForm_Component::KWDA
    int itemCount = 0;                   // BGSFormLinkData_Component::ITMC
    QVector<quint32> linkedFormIds;      // BGSFormLinkData_Component::FLFM
    QVector<quint32> linkedKeywordIds;   // BGSFormLinkData_Component::FLKW
    QStringList componentTypes;          // BFCB component type names present
};

namespace ShipPartCodec
{
    ShipPartDefinition fromGbfm(const GbfmRecord& rec);

    // Writes the definition's identity back onto a record: EDID and the
    // typed fields of TESFullName_Component / BGSKeywordForm_Component /
    // BGSFormLinkData_Component / BGSFormLinkData_Component. Every other
    // raw subrecord (opaque components included) is left untouched, so an
    // edited record still round-trips byte-exactly apart from the mapped
    // fields. Fields the record does not already carry are not created.
    void applyToGbfm(const ShipPartDefinition& def, GbfmRecord& rec);
}

// A resolved COBJ -> FLST -> GBFM chain.
struct ShipComposite
{
    quint32 cobjFormId = 0;
    QString cobjEditorId;
    quint32 formListFormId = 0;
    QString formListEditorId;
    QVector<quint32> variantFormIds;     // FLST LNAM entries
    QStringList variantEditorIds;        // resolved GBFM EDIDs ("" if absent)
};

namespace ShipCompositeResolver
{
    // Resolves a COBJ editor id (e.g. "co_SMS_Struct_Deimos_Hull_A") through
    // its CNAM form list to the variant GBFMs. Returns false when the COBJ is
    // not found; an unlinked COBJ still resolves (empty variant list).
    bool resolve(const IdCollection<CobjRecord>& cobjs,
                 const IdCollection<FormListRecord>& flsts,
                 const IdCollection<GbfmRecord>& gbfms,
                 const QString& cobjEditorId,
                 ShipComposite& out);

    // FormID lookup helper shared by the resolvers (linear scan; collections
    // are editor-id keyed). Returns 0 and leaves outEditorId empty if absent.
    quint32 findEditorIdByFormId(const IdCollection<GbfmRecord>& gbfms,
                                 quint32 formId, QString* outEditorId);

    // Case-insensitive editor-id lookup (the collections' own indexes are
    // case-sensitive while the game compares ids case-insensitively).
    // Returns nullptr when absent.
    const GbfmRecord* findGbfmByEditorId(const IdCollection<GbfmRecord>& gbfms,
                                         const QString& editorId);
}

#endif // SHIPPARTCODEC_HPP
