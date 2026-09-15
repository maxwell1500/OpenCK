#include "shippartcodec.hpp"

namespace {

quint32 leU32(const QByteArray& data, int offset)
{
    if (offset < 0 || offset + 4 > data.size())
        return 0;
    const uchar* p = reinterpret_cast<const uchar*>(data.constData()) + offset;
    return static_cast<quint32>(p[0])
        | (static_cast<quint32>(p[1]) << 8)
        | (static_cast<quint32>(p[2]) << 16)
        | (static_cast<quint32>(p[3]) << 24);
}

void storeU32(QByteArray& data, int offset, quint32 value)
{
    if (offset < 0 || offset + 4 > data.size())
        return;
    uchar* p = reinterpret_cast<uchar*>(data.data()) + offset;
    p[0] = static_cast<uchar>(value & 0xFF);
    p[1] = static_cast<uchar>((value >> 8) & 0xFF);
    p[2] = static_cast<uchar>((value >> 16) & 0xFF);
    p[3] = static_cast<uchar>((value >> 24) & 0xFF);
}

QString trimmedZString(const QByteArray& data)
{
    QString t = QString::fromLatin1(data);
    while (!t.isEmpty() && t.at(t.size() - 1) < QChar(0x20))
        t.chop(1);
    return t;
}

// Walks rawSubRecords tracking the current BFCB component, invoking fn for
// each subrecord with its index and the component name it belongs to.
template<typename Fn>
void forEachSubrecord(const QVector<RawSubRecord>& subs, Fn fn)
{
    QString component;
    bool inComponent = false;
    for (int i = 0; i < subs.size(); ++i)
    {
        const RawSubRecord& raw = subs[i];
        if (raw.name == NAME('BFCB'))
        {
            component = trimmedZString(raw.data);
            inComponent = true;
            continue;
        }
        if (raw.name == NAME('BFCE'))
        {
            inComponent = false;
            component.clear();
            continue;
        }
        fn(i, inComponent ? component : QString(), raw);
    }
}

} // namespace

namespace ShipPartCodec {

ShipPartDefinition fromGbfm(const GbfmRecord& rec)
{
    ShipPartDefinition def;
    def.editorId = rec.editorId;
    def.formId = rec.formId;
    def.nameStringId = rec.fullNameStringId();
    def.keywordFormIds = rec.keywordFormIds();
    def.itemCount = rec.itemCount();
    def.linkedFormIds = rec.linkedFormIds();
    def.linkedKeywordIds = rec.linkedKeywordIds();
    def.componentTypes = rec.componentTypeNames();
    return def;
}

void applyToGbfm(const ShipPartDefinition& def, GbfmRecord& rec)
{
    rec.editorId = def.editorId;

    // FLKW/FLFM repeat once per entry; each occurrence carries a single u32.
    // KWDA is one subrecord holding all N ids, so it advances by offset.
    // Track each independently so neither shape is assumed.
    int kwdaIndex = 0;
    int flkwSeen = 0;
    int flfmSeen = 0;

    forEachSubrecord(rec.rawSubRecords,
        [&](int index, const QString& component, const RawSubRecord& raw)
        {
            RawSubRecord& target = rec.rawSubRecords[index];

            if (component == QStringLiteral("TESFullName_Component")
                && raw.name == NAME('FULL'))
            {
                storeU32(target.data, 0, def.nameStringId);
            }
            else if (component == QStringLiteral("BGSKeywordForm_Component")
                     && raw.name == NAME('KWDA'))
            {
                // Never resize: the KSIZ count and the record's declared size
                // must stay coherent.
                const int capacity = raw.data.size() / 4;
                for (int k = 0; k < capacity && kwdaIndex < def.keywordFormIds.size(); ++k)
                    storeU32(target.data, k * 4, def.keywordFormIds[kwdaIndex++]);
            }
            else if (component == QStringLiteral("BGSFormLinkData_Component")
                     && raw.name == NAME('ITMC'))
            {
                storeU32(target.data, 0, static_cast<quint32>(def.itemCount));
            }
            else if (component == QStringLiteral("BGSFormLinkData_Component")
                     && raw.name == NAME('FLFM'))
            {
                if (flfmSeen < def.linkedFormIds.size())
                    storeU32(target.data, 0, def.linkedFormIds[flfmSeen]);
                ++flfmSeen;
            }
            else if (component == QStringLiteral("BGSFormLinkData_Component")
                     && raw.name == NAME('FLKW'))
            {
                if (flkwSeen < def.linkedKeywordIds.size())
                    storeU32(target.data, 0, def.linkedKeywordIds[flkwSeen]);
                ++flkwSeen;
            }
        });
}

} // namespace ShipPartCodec

namespace {

// Collections are keyed case-sensitively while some callers (and the game)
// compare editor ids case-insensitively, so scan rather than rely on
// searchId's lowercasing. Returns the index or -1.
template<typename ESXRecord>
int findIndexByEditorId(const IdCollection<ESXRecord>& collection,
                        const QString& editorId)
{
    const QVector<Record<ESXRecord>>& records = collection.getRecords();
    for (int i = 0; i < records.size(); ++i)
    {
        if (QString::compare(records[i].get().editorId, editorId,
                             Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

} // namespace

namespace ShipCompositeResolver {

quint32 findEditorIdByFormId(const IdCollection<GbfmRecord>& gbfms,
                             quint32 formId, QString* outEditorId)
{
    const QVector<Record<GbfmRecord>>& records = gbfms.getRecords();
    for (const Record<GbfmRecord>& rec : records)
    {
        if (rec.get().formId == formId)
        {
            if (outEditorId)
                *outEditorId = rec.get().editorId;
            return formId;
        }
    }
    return 0;
}

const GbfmRecord* findGbfmByEditorId(const IdCollection<GbfmRecord>& gbfms,
                                     const QString& editorId)
{
    const int index = findIndexByEditorId(gbfms, editorId);
    return index >= 0 ? &gbfms.getRecord(index).get() : nullptr;
}

bool resolve(const IdCollection<CobjRecord>& cobjs,
             const IdCollection<FormListRecord>& flsts,
             const IdCollection<GbfmRecord>& gbfms,
             const QString& cobjEditorId,
             ShipComposite& out)
{
    out = ShipComposite();

    const int cobjIndex = findIndexByEditorId(cobjs, cobjEditorId);
    if (cobjIndex < 0)
        return false;
    const CobjRecord& cobj = cobjs.getRecord(cobjIndex).get();
    out.cobjFormId = cobj.formId;
    out.cobjEditorId = cobj.editorId;

    const quint32 flstFormId = cobj.createdObjectId();
    if (flstFormId == 0)
        return true;   // unlinked recipe: resolves with an empty variant list

    // Find the form list by FormID (collections are editor-id keyed).
    const FormListRecord* flst = nullptr;
    for (const Record<FormListRecord>& rec : flsts.getRecords())
    {
        if (rec.get().formId == flstFormId)
        {
            flst = &rec.get();
            break;
        }
    }
    if (!flst)
        return true;   // dangling CNAM: keep what we have

    out.formListFormId = flst->formId;
    out.formListEditorId = flst->editorId;
    out.variantFormIds = flst->formIds;

    out.variantEditorIds.reserve(flst->formIds.size());
    for (quint32 variantFormId : flst->formIds)
    {
        QString editorId;
        findEditorIdByFormId(gbfms, variantFormId, &editorId);
        out.variantEditorIds.append(editorId);
    }
    return true;
}

} // namespace ShipCompositeResolver
