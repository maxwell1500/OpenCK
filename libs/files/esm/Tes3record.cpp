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

    // DATA occurrences: the typed component mirrors the first occurrence
    // with a proven layout (e.g. the CELL header, not a nested ref
    // transform); records without one keep last-wins like before.
    int dataTarget = -1;
    int dataSeen = -1;
    for (const auto& raw : rawSubRecords)
    {
        if (raw.name != NAME('DATA'))
            continue;
        ++dataSeen;
        if (dataTarget < 0 && tes3DataLayoutFor(code, raw.data.size()))
            dataTarget = dataSeen;
    }
    if (dataTarget < 0)
        dataTarget = dataSeen;

    // Parse subrecords into components for display/editing
    int dataOccurrence = -1;
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
            ++dataOccurrence;
            if (dataOccurrence != dataTarget)
                continue;
            data->data = raw.data;
            data->dataOccurrence = dataTarget;
            data->decode(code, raw.data);
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

    // Component-backed values for the subrecords parseComponents() extracts.
    // A stored raw is re-emitted verbatim when the component still matches
    // the parse of its last raw occurrence (parse is last-wins for
    // duplicates and lossy for non-UTF8 bytes, so byte comparison would flag
    // untouched records as edited); a genuine component edit is written
    // once, at the last occurrence's position, so untouched records stay
    // byte-identical while form-dialog edits survive the save.
    QHash<NAME, QString> editedStrings;
    if (const auto* full = static_cast<const tescomponents::TESFullName_Component*>(
            components.findByName(QStringLiteral("TESFullName"))))
        editedStrings.insert(NAME('FULL'), full->fullName);
    if (const auto* model = static_cast<const tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        editedStrings.insert(NAME('MODL'), model->modelPath);
        editedStrings.insert(NAME('MNAM'), model->lodModelPath);
    }
    if (const auto* tex = static_cast<const tescomponents::TESTexture_Component*>(
            components.findByName(QStringLiteral("TESTexture"))))
    {
        editedStrings.insert(NAME('ICON'), tex->iconPath);
        editedStrings.insert(NAME('ICO2'), tex->smallIconPath);
    }
    const auto* dataComp = static_cast<const tescomponents::Tes3Data_Component*>(
        components.findByName(QStringLiteral("Tes3Data")));

    // Replica of the parseComponents() string decode (NUL-strip + UTF-8).
    const auto parseString = [](const QByteArray& raw) {
        QString s = QString::fromUtf8(raw.constData(), raw.size());
        while (s.endsWith(QChar(0)))
            s.chop(1);
        return s;
    };

    QSet<NAME> dirty;
    const auto checkString = [&](NAME sub) {
        if (!editedStrings.contains(sub) || !rawByName.contains(sub))
            return;
        const QVector<int>& idx = rawByName[sub];
        if (parseString(rawSubRecords[idx.last()].data) != editedStrings.value(sub))
            dirty.insert(sub);
    };
    checkString(NAME('FULL'));
    checkString(NAME('MODL'));
    checkString(NAME('MNAM'));
    checkString(NAME('ICON'));
    checkString(NAME('ICO2'));

    // DATA substitutes at its mirrored occurrence (usually the last; the
    // CELL header for multi-DATA CELLs), not blindly at the last raw.
    int dataTargetPos = -1;
    bool dataDirty = false;
    if (dataComp && rawByName.contains(NAME('DATA')))
    {
        const QVector<int>& didx = rawByName[NAME('DATA')];
        dataTargetPos = (dataComp->dataOccurrence >= 0 && dataComp->dataOccurrence < didx.size())
            ? dataComp->dataOccurrence
            : didx.size() - 1;
        dataDirty = rawSubRecords[didx[dataTargetPos]].data != dataComp->data;
    }

    const auto matchesRaw = [](const QByteArray& raw, const QByteArray& payload) {
        if (raw == payload)
            return true;
        return raw.size() > payload.size() && raw.startsWith(payload)
            && std::all_of(raw.constBegin() + payload.size(), raw.constEnd(),
                            [](char c) { return c == '\0'; });
    };

    const auto writeEdited = [&esm, &editedStrings, dataComp](NAME sub) {
        if (sub == NAME('DATA'))
        {
            esm.startSubRecord(sub);
            const QByteArray payload = dataComp ? dataComp->data : QByteArray();
            esm.writeRawData(payload.constData(), payload.size());
            esm.endSubRecord();
            return;
        }
        esm.writeSubZString(sub, editedStrings.value(sub));
    };

    QSet<NAME> substituted;
    for (int i = 0; i < loadOrder.size(); ++i)
    {
        const NAME sub = loadOrder[i];
        if (i == nameIndex)
        {
            const QByteArray payload = editorId.toLatin1();
            if (matchesRaw(nameRaw, payload))
            {
                esm.startSubRecord(NAME('NAME'));
                esm.writeRawData(nameRaw.constData(), nameRaw.size());
                esm.endSubRecord();
            }
            else
            {
                esm.writeSubZString(NAME('NAME'), editorId);
            }
            continue;
        }
        const QVector<int>& idx = rawByName[sub];
        int& cur = rawCursor[sub];
        if (cur >= idx.size())
            continue;
        const int occurrencePos = cur;
        const RawSubRecord& raw = rawSubRecords[idx[cur++]];
        const bool isLastOccurrence = (occurrencePos == idx.size() - 1);
        bool substitute = false;
        if (sub == NAME('DATA'))
            substitute = dataDirty && occurrencePos == dataTargetPos;
        else
            substitute = dirty.contains(sub) && isLastOccurrence;
        if (substitute && !substituted.contains(sub))
        {
            substituted.insert(sub);
            writeEdited(sub);
        }
        else
        {
            esm.writeRawSubRecord(raw);
        }
    }

    if (nameIndex < 0 && !editorId.isEmpty())
        esm.writeSubZString(NAME('NAME'), editorId);

    // Component values for subrecords the source lacked are appended in a
    // fixed order; anything else replays verbatim as before.
    static const NAME kAppended[] = {
        NAME('FULL'), NAME('MODL'), NAME('MNAM'),
        NAME('ICON'), NAME('ICO2'), NAME('DATA')
    };
    for (NAME sub : kAppended)
    {
        if (rawByName.contains(sub))
            continue;
        const QByteArray payload = (sub == NAME('DATA') && dataComp)
            ? dataComp->data
            : editedStrings.value(sub).toUtf8();
        if (!payload.isEmpty())
            writeEdited(sub);
    }

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
