#include "Tes3record.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"
#include "../../components/tes3_components.hpp"

#include <QHash>

#include <algorithm>

namespace
{
// TES3 records have no on-disk form id. Each loaded record gets a unique
// synthetic one so the collection key (and pluginOrder) is stable even for
// records with no NAME subrecord (LAND, PGRD, ...).
quint32 nextSyntheticTes3FormId()
{
    static quint32 counter = 0x01000000;
    return counter++;
}
}

void Tes3Record::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::Tes3Data_Component>();
}

void Tes3Record::load(ESMReader& esm, bool)
{
    RecHeader recHeader = esm.readHeader();
    code = esm.currentRecordName();
    formId = nextSyntheticTes3FormId();
    flags = recHeader.flags.val;
    unknownHeader = 0;
    editorId.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    nameIndex = -1;
    nameRaw.clear();
    initComponents();

    while (esm.isRecLeft())
    {
        if (esm.recLeft() < 0)
            break;
        NAME sub = esm.readNSubHeader();
        if (sub == 0)
            break;
        loadOrder.append(sub);

        if (sub == NAME('NAME') && nameIndex < 0)
        {
            nameIndex = loadOrder.size() - 1;
            esm.readRawSubData(nameRaw);
            QByteArray trimmed = nameRaw;
            while (trimmed.endsWith('\0'))
                trimmed.chop(1);
            editorId = QString::fromLatin1(trimmed);
            continue;
        }

        // Try to dispatch to a component
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
        if (handled)
            continue;

        // Fallback: store as raw for lossless round-trip
        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }
}

void Tes3Record::save(ESMWriter& esm) const
{
    // Index raw subrecords by name for save-time lookup
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    // Track which subrecords we've written to avoid duplicates
    bool wroteName = false;
    bool wroteFull = false;
    bool wroteModl = false;
    bool wroteMnam = false;
    bool wroteIcon = false;
    bool wroteIco2 = false;
    bool wroteData = false;

    // Get component pointers
    auto* full = static_cast<tescomponents::TESFullName_Component*>(
        const_cast<Tes3Record*>(this)->components.findByName(QStringLiteral("TESFullName")));
    auto* model = static_cast<tescomponents::TESModel_Component*>(
        const_cast<Tes3Record*>(this)->components.findByName(QStringLiteral("TESModel")));
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(
        const_cast<Tes3Record*>(this)->components.findByName(QStringLiteral("TESTexture")));
    auto* data = static_cast<tescomponents::Tes3Data_Component*>(
        const_cast<Tes3Record*>(this)->components.findByName(QStringLiteral("Tes3Data")));

    // Walk the load order and write each subrecord in its original position
    for (int i = 0; i < loadOrder.size(); ++i)
    {
        const NAME sub = loadOrder[i];

        if (sub == NAME('NAME'))
        {
            if (!wroteName)
            {
                const QByteArray payload = editorId.toLatin1();
                const bool unchanged = nameRaw.size() >= payload.size()
                    && nameRaw.startsWith(payload)
                    && std::all_of(nameRaw.constBegin() + payload.size(), nameRaw.constEnd(),
                                   [](char c) { return c == '\0'; });
                esm.startSubRecord(NAME('NAME'));
                if (unchanged)
                {
                    esm.writeRawData(nameRaw.constData(), nameRaw.size());
                }
                else
                {
                    esm.writeRawData(payload.constData(), payload.size());
                    const char nul = '\0';
                    esm.writeRawData(&nul, 1);
                }
                esm.endSubRecord();
                wroteName = true;
            }
            continue;
        }

        // Dispatch to component save
        if (sub == NAME('FULL'))
        {
            if (!wroteFull && full && !full->fullName.isEmpty())
            {
                esm.startSubRecord(NAME('FULL'));
                esm.writeZString(full->fullName);
                esm.endSubRecord();
            }
            wroteFull = true;
            continue;
        }

        if (sub == NAME('MODL'))
        {
            if (!wroteModl && model && !model->modelPath.isEmpty())
            {
                esm.writeSubZString(NAME('MODL'), model->modelPath);
            }
            wroteModl = true;
            continue;
        }

        if (sub == NAME('MNAM'))
        {
            if (!wroteMnam && model && !model->lodModelPath.isEmpty())
            {
                esm.writeSubZString(NAME('MNAM'), model->lodModelPath);
            }
            wroteMnam = true;
            continue;
        }

        if (sub == NAME('ICON'))
        {
            if (!wroteIcon && tex && !tex->iconPath.isEmpty())
            {
                esm.writeSubZString(NAME('ICON'), tex->iconPath);
            }
            wroteIcon = true;
            continue;
        }

        if (sub == NAME('ICO2'))
        {
            if (!wroteIco2 && tex && !tex->smallIconPath.isEmpty())
            {
                esm.writeSubZString(NAME('ICO2'), tex->smallIconPath);
            }
            wroteIco2 = true;
            continue;
        }

        if (sub == NAME('DATA'))
        {
            if (!wroteData && data)
            {
                data->save(esm);
            }
            wroteData = true;
            continue;
        }

        // Fallback: write raw subrecord
        const QVector<int>& idx = rawByName[sub];
        if (!idx.isEmpty())
        {
            int& cur = rawCursor[sub];
            if (cur < idx.size())
                esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
        }
    }

    // Append any component subrecords that weren't in the load order
    if (!wroteName && !editorId.isEmpty())
        esm.writeSubZString(NAME('NAME'), editorId);
    if (!wroteFull && full && !full->fullName.isEmpty())
    {
        esm.startSubRecord(NAME('FULL'));
        esm.writeZString(full->fullName);
        esm.endSubRecord();
    }
    if (!wroteModl && model && !model->modelPath.isEmpty())
        esm.writeSubZString(NAME('MODL'), model->modelPath);
    if (!wroteMnam && model && !model->lodModelPath.isEmpty())
        esm.writeSubZString(NAME('MNAM'), model->lodModelPath);
    if (!wroteIcon && tex && !tex->iconPath.isEmpty())
        esm.writeSubZString(NAME('ICON'), tex->iconPath);
    if (!wroteIco2 && tex && !tex->smallIconPath.isEmpty())
        esm.writeSubZString(NAME('ICO2'), tex->smallIconPath);
    if (!wroteData && data)
        data->save(esm);

    // Append any remaining raw subrecords
    for (auto it = rawByName.constBegin(); it != rawByName.constEnd(); ++it)
    {
        const QVector<int>& idx = it.value();
        int& cur = rawCursor[it.key()];
        while (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    }
}

void Tes3Record::blank()
{
    code = 0;
    editorId.clear();
    formId = 0;
    flags = 0;
    unknownHeader = 0;
    nameIndex = -1;
    nameRaw.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    initComponents();
}
