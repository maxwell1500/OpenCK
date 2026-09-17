#include "Actirecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

#include <QHash>

void ActiRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void ActiRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG':
                flags = esm.readSubU32(&flagsWidth);
                hasFlags = true;
                flagsSpelling = sub;
                break;
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
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void ActiRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<ActiRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<ActiRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    const auto writeFlags = [&]()
    {
        esm.startSubRecord(flagsSpelling);
        const quint8 w = flagsWidth == 0 ? 1 : flagsWidth;
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false, wroteModel = false,
        wroteLodModel = false, wroteIcon = false, wroteSmallIcon = false;

    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFlags && (hasFlags || flags != 0))
                {
                    writeFlags();
                    wroteFlags = true;
                }
                break;
            case 'MODL':
                if (!wroteModel && model && !model->modelPath.isEmpty())
                    esm.writeSubZString(NAME('MODL'), model->modelPath);
                wroteModel = true;
                break;
            case 'MNAM':
                if (!wroteLodModel && model && !model->lodModelPath.isEmpty())
                    esm.writeSubZString(NAME('MNAM'), model->lodModelPath);
                wroteLodModel = true;
                break;
            case 'ICON':
                if (!wroteIcon && tex && !tex->iconPath.isEmpty())
                    esm.writeSubZString(NAME('ICON'), tex->iconPath);
                wroteIcon = true;
                break;
            case 'ICO2':
                if (!wroteSmallIcon && tex && !tex->smallIconPath.isEmpty())
                    esm.writeSubZString(NAME('ICO2'), tex->smallIconPath);
                wroteSmallIcon = true;
                break;
            default:
            {
                const QVector<int>& idx = rawByName[sub];
                int& cur = rawCursor[sub];
                if (cur < idx.size())
                    esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
                break;
            }
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && (hasFlags || flags != 0))
        writeFlags();
    if (!wroteModel && model && !model->modelPath.isEmpty())
        esm.writeSubZString(NAME('MODL'), model->modelPath);
    if (!wroteLodModel && model && !model->lodModelPath.isEmpty())
        esm.writeSubZString(NAME('MNAM'), model->lodModelPath);
    if (!wroteIcon && tex && !tex->iconPath.isEmpty())
        esm.writeSubZString(NAME('ICON'), tex->iconPath);
    if (!wroteSmallIcon && tex && !tex->smallIconPath.isEmpty())
        esm.writeSubZString(NAME('ICO2'), tex->smallIconPath);

    for (auto it = rawByName.constBegin(); it != rawByName.constEnd(); ++it)
    {
        const QVector<int>& idx = it.value();
        int& cur = rawCursor[it.key()];
        while (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    }
}

void ActiRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    initComponents();
}
