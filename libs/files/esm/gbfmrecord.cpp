#include "gbfmrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

QVector<GbfmComponent> GbfmRecord::parseComponents() const
{
    return splitBaseFormComponents(rawSubRecords);
}

void GbfmRecord::splitComponents(QVector<GbfmComponent>& outPieces,
                                 QVector<RawSubRecord>& outLeading,
                                 QVector<RawSubRecord>& outTrailing) const
{
    outPieces = splitBaseFormComponents(rawSubRecords, &outLeading, &outTrailing);
}

const GbfmComponent* GbfmRecord::findComponent(const QVector<GbfmComponent>& comps,
                                               const QString& typeName)
{
    return findBaseFormComponent(comps, typeName);
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

QStringList GbfmRecord::componentTypeNames() const
{
    QStringList names;
    const QVector<GbfmComponent> pieces = parseComponents();
    names.reserve(pieces.size());
    for (const GbfmComponent& c : pieces)
        names.append(c.typeName);
    return names;
}

quint32 GbfmRecord::fullNameStringId() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("TESFullName_Component"));
    return c ? c->firstU32(NAME('FULL')) : 0;
}

QVector<quint32> GbfmRecord::keywordFormIds() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSKeywordForm_Component"));
    return c ? c->u32List(NAME('KWDA')) : QVector<quint32>();
}

int GbfmRecord::itemCount() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSFormLinkData_Component"));
    return c ? static_cast<int>(c->firstU32(NAME('ITMC'))) : 0;
}

QVector<quint32> GbfmRecord::linkedFormIds() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("BGSFormLinkData_Component"));
    return c ? c->u32List(NAME('FLFM')) : QVector<quint32>();
}

QVector<quint32> GbfmRecord::linkedKeywordIds() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
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

QVector<ShipBlueprintItem> GbfmRecord::blueprintItems() const
{
    QVector<ShipBlueprintItem> items;

    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces, QStringLiteral("Blueprint_Component"));
    if (!c)
        return items;

    for (const RawSubRecord& raw : c->subrecords)
    {
        if (raw.name != NAME('BUO4'))
            continue;
        const int count = raw.data.size() / ShipBlueprintItem::kStride;
        for (int i = 0; i < count; ++i)
        {
            const int base = i * ShipBlueprintItem::kStride;
            ShipBlueprintItem item;
            item.baseItemFormId = baseFormU32(raw.data, base + 0);
            item.constructionFormId = baseFormU32(raw.data, base + 4);
            item.posX = baseFormF32(raw.data, base + 8);
            item.posY = baseFormF32(raw.data, base + 12);
            item.posZ = baseFormF32(raw.data, base + 16);
            item.rotX = baseFormF32(raw.data, base + 20);
            item.rotY = baseFormF32(raw.data, base + 24);
            item.rotZ = baseFormF32(raw.data, base + 28);
            item.partId = baseFormU32(raw.data, base + 32);
            items.append(item);
        }
    }
    return items;
}

float GbfmRecord::crowdDensity() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces,
        QStringLiteral("BGSCrowdComponent_Component"));
    return c ? c->firstFloat(NAME('CDND')) : 0.0f;
}

int GbfmRecord::crowdPopulationCount() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces,
        QStringLiteral("BGSCrowdComponent_Component"));
    return c ? static_cast<int>(c->firstU32(NAME('CDNS'))) : 0;
}

QVector<CrowdPopulation> GbfmRecord::crowdPopulations() const
{
    QVector<CrowdPopulation> populations;

    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces,
        QStringLiteral("BGSCrowdComponent_Component"));
    if (!c)
        return populations;

    // Each population ends with STRV (name) then FLTV (scale), in order.
    CrowdPopulation current;
    bool pending = false;
    for (const RawSubRecord& raw : c->subrecords)
    {
        if (raw.name == NAME('STRV'))
        {
            if (pending)
                populations.append(current);
            current = CrowdPopulation();
            current.name = QString::fromLatin1(raw.data);
            while (!current.name.isEmpty()
                   && current.name.at(current.name.size() - 1) < QChar(0x20))
                current.name.chop(1);
            pending = true;
        }
        else if (raw.name == NAME('FLTV') && pending)
        {
            current.scale = baseFormF32(raw.data, 0);
            populations.append(current);
            current = CrowdPopulation();
            pending = false;
        }
    }
    if (pending)
        populations.append(current);

    return populations;
}

ReflectionStream GbfmRecord::reflectionStream() const
{
    const QVector<GbfmComponent> pieces = parseComponents();
    const GbfmComponent* c = findComponent(pieces,
        QStringLiteral("ReflectionProbes_Component"));
    if (!c)
        return ReflectionStream();
    const RawSubRecord* r = c->findSubrecord(NAME('REFL'));
    return r ? parseReflectionStream(r->data) : ReflectionStream();
}
