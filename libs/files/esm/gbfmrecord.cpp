#include "gbfmrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

#include <cstring>

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

QString trimmedZString(const QString& s)
{
    QString t = s;
    while (!t.isEmpty() && t.at(t.size() - 1) < QChar(0x20))
        t.chop(1);
    return t;
}

const GbfmComponent* componentByName(const QVector<GbfmComponent>& comps,
                                     const QString& typeName)
{
    for (const GbfmComponent& c : comps)
        if (c.typeName == typeName)
            return &c;
    return nullptr;
}

} // namespace

bool GbfmComponent::hasSubrecord(NAME name) const
{
    return findSubrecord(name) != nullptr;
}

const RawSubRecord* GbfmComponent::findSubrecord(NAME name) const
{
    for (const RawSubRecord& r : subrecords)
        if (r.name == name)
            return &r;
    return nullptr;
}

quint32 GbfmComponent::firstU32(NAME name) const
{
    const RawSubRecord* r = findSubrecord(name);
    return r ? leU32(r->data, 0) : 0;
}

QVector<quint32> GbfmComponent::u32List(NAME name) const
{
    // Collect across every occurrence: KWDA is one subrecord holding N ids,
    // while FLKW/FLFM appear once per entry. Both flatten to the same list.
    QVector<quint32> out;
    for (const RawSubRecord& r : subrecords)
    {
        if (r.name != name)
            continue;
        for (int off = 0; off + 4 <= r.data.size(); off += 4)
            out.append(leU32(r.data, off));
    }
    return out;
}

void GbfmRecord::initComponents()
{
    components.clear();
}

void GbfmRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
        }
    }
}

void GbfmRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void GbfmRecord::blank()
{
    editorId.clear();
    formId = 0;
    rawSubRecords.clear();
    initComponents();
}

void GbfmRecord::splitComponents(QVector<GbfmComponent>& outPieces,
                                 QVector<RawSubRecord>& outLeading,
                                 QVector<RawSubRecord>& outTrailing) const
{
    outPieces.clear();
    outLeading.clear();
    outTrailing.clear();

    // BFCB opens a component; BFCE (when present) closes it; a new BFCB also
    // implicitly closes the open one. Everything before the first BFCB and
    // after the last close is reported as leading/trailing.
    GbfmComponent current;
    bool inComponent = false;

    for (const RawSubRecord& raw : rawSubRecords)
    {
        if (raw.name == NAME('BFCB'))
        {
            if (inComponent)
                outPieces.append(current);
            current = GbfmComponent();
            current.typeName = trimmedZString(QString::fromLatin1(raw.data));
            inComponent = true;
        }
        else if (raw.name == NAME('BFCE'))
        {
            if (inComponent)
            {
                outPieces.append(current);
                current = GbfmComponent();
                inComponent = false;
            }
        }
        else if (inComponent)
        {
            current.subrecords.append(raw);
        }
        else if (outPieces.isEmpty())
        {
            outLeading.append(raw);
        }
        else
        {
            outTrailing.append(raw);
        }
    }

    if (inComponent)
        outPieces.append(current);
}

QVector<GbfmComponent> GbfmRecord::parseComponents() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    return pieces;
}

const GbfmComponent* GbfmRecord::findComponent(const QVector<GbfmComponent>& comps,
                                               const QString& typeName)
{
    return componentByName(comps, typeName);
}

QStringList GbfmRecord::componentTypeNames() const
{
    QStringList names;
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    names.reserve(pieces.size());
    for (const GbfmComponent& c : pieces)
        names.append(c.typeName);
    return names;
}

quint32 GbfmRecord::fullNameStringId() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("TESFullName_Component"));
    return c ? c->firstU32(NAME('FULL')) : 0;
}

QVector<quint32> GbfmRecord::keywordFormIds() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSKeywordForm_Component"));
    return c ? c->u32List(NAME('KWDA')) : QVector<quint32>();
}

int GbfmRecord::itemCount() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSFormLinkData_Component"));
    return c ? static_cast<int>(c->firstU32(NAME('ITMC'))) : 0;
}

QVector<quint32> GbfmRecord::linkedFormIds() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSFormLinkData_Component"));
    return c ? c->u32List(NAME('FLFM')) : QVector<quint32>();
}

QVector<quint32> GbfmRecord::linkedKeywordIds() const
{
    QVector<GbfmComponent> pieces;
    QVector<RawSubRecord> leading;
    QVector<RawSubRecord> trailing;
    splitComponents(pieces, leading, trailing);
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSFormLinkData_Component"));
    return c ? c->u32List(NAME('FLKW')) : QVector<quint32>();
}

int GbfmRecord::rawSubrecordIndex(NAME name) const
{
    for (int i = 0; i < rawSubRecords.size(); ++i)
        if (rawSubRecords[i].name == name)
            return i;
    return -1;
}
