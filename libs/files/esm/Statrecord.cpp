#include "Statrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

#include <QHash>

void StatRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void StatRecord::load(ESMReader& esm, bool)
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
            case 'FNAM': case 'FLAG':
                flags = esm.readType<quint32>();
                hasFlags = true;
                flagsSpelling = sub;
                break;
            case 'RNAM': lodFlags = esm.readType<quint32>(); break;
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
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) {
        modelPath = model->modelPath;
        lodModelPath = model->lodModelPath;
    }
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) {
        iconPath = tex->iconPath;
        smallIconPath = tex->smallIconPath;
    }
}

void StatRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<StatRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) {
        model->modelPath = modelPath;
        model->lodModelPath = lodModelPath;
    }
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<StatRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) {
        tex->iconPath = iconPath;
        tex->smallIconPath = smallIconPath;
    }

    // Index raw subrecords by name so the load order walk below re-emits
    // them at their original positions (the snapshot gate compares
    // positionally, so an untouched save must preserve subrecord order).
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    bool wroteEdid = false, wroteFlags = false, wroteModel = false,
        wroteLodModel = false, wroteIcon = false, wroteSmallIcon = false,
        wroteLodFlags = false;

    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid)
                {
                    esm.writeSubZString('EDID', editorId);
                    wroteEdid = true;
                }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFlags && (hasFlags || flags != 0))
                {
                    esm.writeSubData<quint32>(flagsSpelling, flags);
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
            case 'RNAM':
                if (!wroteLodFlags && lodFlags != 0)
                    esm.writeSubData<quint32>('RNAM', lodFlags);
                wroteLodFlags = true;
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

    // Values introduced after load (or absent from it) are appended.
    if (!wroteEdid)
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && (hasFlags || flags != 0))
        esm.writeSubData<quint32>(flagsSpelling, flags);
    if (!wroteModel && model && !model->modelPath.isEmpty())
        esm.writeSubZString(NAME('MODL'), model->modelPath);
    if (!wroteLodModel && model && !model->lodModelPath.isEmpty())
        esm.writeSubZString(NAME('MNAM'), model->lodModelPath);
    if (!wroteIcon && tex && !tex->iconPath.isEmpty())
        esm.writeSubZString(NAME('ICON'), tex->iconPath);
    if (!wroteSmallIcon && tex && !tex->smallIconPath.isEmpty())
        esm.writeSubZString(NAME('ICO2'), tex->smallIconPath);
    if (!wroteLodFlags && lodFlags != 0)
        esm.writeSubData<quint32>('RNAM', lodFlags);

    for (auto it = rawByName.constBegin(); it != rawByName.constEnd(); ++it)
    {
        const QVector<int>& idx = it.value();
        int& cur = rawCursor[it.key()];
        while (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    }
}

void StatRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    iconPath.clear();
    smallIconPath.clear();
    modelPath.clear();
    lodModelPath.clear();
    lodFlags = 0;
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    components.clear();
    initComponents();
}
