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

void Tes3Record::parseComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::Tes3Data_Component>();

    // Parse subrecords into components for display/editing
    for (const auto& raw : rawSubRecords)
    {
        if (raw.name == NAME('NAME'))
            continue; // editor id is handled separately

        auto* full = static_cast<tescomponents::TESFullName_Component*>(
            components.findByName(QStringLiteral("TESFullName")));
        if (raw.name == NAME('FULL') && full)
        {
            QString s = QString::fromUtf8(raw.data.constData(), raw.data.size());
            while (s.endsWith('\0'))
                s.chop(1);
            full->fullName = s;
            continue;
        }

        auto* model = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel")));
        if (raw.name == NAME('MODL') && model)
        {
            model->modelPath = QString::fromUtf8(raw.data.constData(), raw.data.size());
            while (model->modelPath.endsWith('\0'))
                model->modelPath.chop(1);
            continue;
        }
        if (raw.name == NAME('MNAM') && model)
        {
            model->lodModelPath = QString::fromUtf8(raw.data.constData(), raw.data.size());
            while (model->lodModelPath.endsWith('\0'))
                model->lodModelPath.chop(1);
            continue;
        }

        auto* tex = static_cast<tescomponents::TESTexture_Component*>(
            components.findByName(QStringLiteral("TESTexture")));
        if (raw.name == NAME('ICON') && tex)
        {
            tex->iconPath = QString::fromUtf8(raw.data.constData(), raw.data.size());
            while (tex->iconPath.endsWith('\0'))
                tex->iconPath.chop(1);
            continue;
        }
        if (raw.name == NAME('ICO2') && tex)
        {
            tex->smallIconPath = QString::fromUtf8(raw.data.constData(), raw.data.size());
            while (tex->smallIconPath.endsWith('\0'))
                tex->smallIconPath.chop(1);
            continue;
        }

        auto* data = static_cast<tescomponents::Tes3Data_Component*>(
            components.findByName(QStringLiteral("Tes3Data")));
        if (raw.name == NAME('DATA') && data)
        {
            data->data = raw.data;
            continue;
        }
    }
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
        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }

    parseComponents();
}

void Tes3Record::save(ESMWriter& esm) const
{
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    for (int i = 0; i < loadOrder.size(); ++i)
    {
        const NAME sub = loadOrder[i];
        if (i == nameIndex)
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
            continue;
        }
        const QVector<int>& idx = rawByName[sub];
        int& cur = rawCursor[sub];
        if (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    }

    if (nameIndex < 0 && !editorId.isEmpty())
        esm.writeSubZString(NAME('NAME'), editorId);

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
    components.clear();
}
