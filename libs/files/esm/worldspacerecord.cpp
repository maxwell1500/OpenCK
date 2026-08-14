#include "worldspacerecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

#include <cstring>

namespace {

bool isTypedName(NAME sub)
{
    switch (sub)
    {
    case 'EDID': case 'FULL': case 'CNAM': case 'ZNAM':
    case 'XNAM': case 'NAM2': case 'NAM3': case 'MNAM':
    case 'NAMA': case 'ONAM': case 'DATA': case 'DNAM':
        return true;
    default:
        return false;
    }
}

QString trimmedZString(const QString& s)
{
    QString t = s;
    while (!t.isEmpty() && t.at(t.size() - 1) < QChar(0x20))
        t.chop(1);
    return t;
}

float floatAt(const QByteArray& data)
{
    if (data.size() < static_cast<int>(sizeof(float))) return 1.0f;
    float v = 1.0f;
    std::memcpy(&v, data.constData(), sizeof(float));
    return v;
}

} // namespace

void WorldspaceRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void WorldspaceRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        mOrder.append(sub);
        if (isTypedName(sub))
        {
            switch (sub)
            {
            case 'EDID': editorId = trimmedZString(esm.readZString()); break;
            case 'FULL': name = trimmedZString(esm.readZString()); break;
            case 'CNAM': climateId = esm.readType<quint32>(); break;
            case 'ZNAM': lightingId = esm.readType<quint32>(); break;
            case 'XNAM': waterType = esm.readType<quint32>(); break;
            case 'NAM2': mapWidth = esm.readType<quint32>(); break;
            case 'NAM3': mapHeight = esm.readType<quint32>(); break;
            case 'MNAM':
            {
                mapNwX = esm.readType<qint32>();
                mapNwY = esm.readType<qint32>();
                mapSeX = esm.readType<qint32>();
                mapSeY = esm.readType<qint32>();
                break;
            }
            case 'NAMA': mapLodBias = esm.readType<float>(); break;
            case 'ONAM': esm.readRawSubData(onamData); break;
            case 'DATA': esm.readRawSubData(dataFlags); break;
            case 'DNAM': esm.readRawSubData(dnamData); break;
            }
        }
        else
        {
            RawSubRecord raw;
            raw.name = sub;
            esm.readRawSubData(raw.data);
            rawSubRecords.push_back(raw);
        }
    }

    mapSize = mapWidth;
    dataMinX = mapNwX;
    dataMinY = mapNwY;
}

void WorldspaceRecord::save(ESMWriter& esm) const
{
    int rawIdx = 0;
    for (quint32 sub : mOrder)
    {
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FULL': esm.writeSubZString('FULL', name); break;
        case 'CNAM': esm.writeSubData<quint32>('CNAM', climateId); break;
        case 'ZNAM': esm.writeSubData<quint32>('ZNAM', lightingId); break;
        case 'XNAM': esm.writeSubData<quint32>('XNAM', waterType); break;
        case 'NAM2': esm.writeSubData<quint32>('NAM2', mapWidth); break;
        case 'NAM3': esm.writeSubData<quint32>('NAM3', mapHeight); break;
        case 'MNAM':
        {
            esm.startSubRecord('MNAM');
            esm.writeType<qint32>(mapNwX);
            esm.writeType<qint32>(mapNwY);
            esm.writeType<qint32>(mapSeX);
            esm.writeType<qint32>(mapSeY);
            esm.endSubRecord();
            break;
        }
        case 'NAMA': esm.writeSubData<float>('NAMA', mapLodBias); break;
        case 'ONAM':
        {
            esm.startSubRecord('ONAM');
            esm.writeRawData(onamData.constData(), static_cast<qint32>(onamData.size()));
            esm.endSubRecord();
            break;
        }
        case 'DATA':
        {
            esm.startSubRecord('DATA');
            esm.writeRawData(dataFlags.constData(), static_cast<qint32>(dataFlags.size()));
            esm.endSubRecord();
            break;
        }
        case 'DNAM':
        {
            esm.startSubRecord('DNAM');
            esm.writeRawData(dnamData.constData(), static_cast<qint32>(dnamData.size()));
            esm.endSubRecord();
            break;
        }
        default:
            if (rawIdx < rawSubRecords.size())
            {
                const RawSubRecord& raw = rawSubRecords[rawIdx++];
                esm.writeRawSubRecord(raw);
            }
            break;
        }
    }
}

void WorldspaceRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    name = "";
    iconPath = "";
    waterType = 0;
    climateId = 0;
    lightingId = 0;
    mapWidth = 0;
    mapHeight = 0;
    mapNwX = 0;
    mapNwY = 0;
    mapSeX = 0;
    mapSeY = 0;
    mapLodBias = 1.0f;
    onamData.clear();
    dataFlags.clear();
    dnamData.clear();
    templ = 0;
    terrain = 0;
    mapImage = "";
    lodNoise = "";
    billboardTexture = "";
    music = 0;
    dnam = 0;
    dataMinX = 0;
    dataMinY = 0;
    mapSize = 0;
    cellIds.clear();
    navPointIds.clear();
    rawSubRecords.clear();
    mOrder.clear();
    initComponents();
}

float WorldspaceRecord::mapScale() const
{
    return floatAt(onamData);
}

void WorldspaceRecord::setMapScale(float scale)
{
    if (onamData.size() < static_cast<int>(sizeof(float)))
        onamData.resize(sizeof(float), '\0');
    std::memcpy(onamData.data(), &scale, sizeof(float));
}
