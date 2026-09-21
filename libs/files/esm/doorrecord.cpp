#include "doorrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <QSet>

void DoorRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();

    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlags = false;
    hasSnam = false;
    flagsWidth = 4;
    flagsExtra.clear();
    flagsName = NAME('FNAM');
    snamWidth = 4;
    snamExtra.clear();
    auto readScalar = [&](quint32& out, quint8& width, QByteArray& extra) {
        extra.clear();
        const qint64 left = esm.subLeft();
        if (left >= 4)
        {
            out = esm.readType<quint32>();
            width = 4;
        }
        else
        {
            out = 0;
            width = static_cast<quint8>(qMax<qint64>(left, 0));
            for (qint64 i = 0; i < left; ++i)
                out |= quint32(esm.readType<quint8>()) << (8 * i);
        }
        if (esm.subLeft() > 0)
            esm.readRawSubData(extra);
    };
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; handled = true; break;
            case 'FNAM': case 'FLAG':
                readScalar(flags, flagsWidth, flagsExtra); flagsName = sub;
                hasFlags = true; handled = true; break;
            case 'SNAM': readScalar(sound, snamWidth, snamExtra); hasSnam = true; handled = true; break;
            default: break;
        }
        if (handled) { loadIsRaw.append(0); continue; }

        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) { loadIsRaw.append(0); continue; }

        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
        loadIsRaw.append(1);
    }

    if (auto* n = static_cast<tescomponents::TESFullName_Component*>(
            components.findByName(QStringLiteral("TESFullName"))))
    {
        fullName = n->fullName;
    }
    if (auto* m = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        modelPath = m->modelPath;
    }
}

void DoorRecord::save(ESMWriter& esm) const
{
    const auto writeScalar = [&](NAME name, quint32 value, quint8 width, const QByteArray& extra) {
        quint8 w = width;
        if (w == 0 && value != 0)
            w = 4;
        esm.startSubRecord(name);
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((value >> (8 * i)) & 0xFF));
        if (!extra.isEmpty())
            esm.writeRawData(extra.constData(), extra.size());
        esm.endSubRecord();
    };
    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        if (hasFlags || flags != 0)
            writeScalar(flagsName, flags, flagsWidth, flagsExtra);
        if (hasSnam || sound != 0)
            writeScalar(NAME('SNAM'), sound, snamWidth, snamExtra);
        components.saveAll(esm);
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const NAME sub = loadOrder[p];
        seen.insert(sub);
        if (p >= loadIsRaw.size() || loadIsRaw[p] != 0)
        {
            if (rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            continue;
        }
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FNAM': case 'FLAG': writeScalar(sub, flags, flagsWidth, flagsExtra); break;
        case 'SNAM': writeScalar(NAME('SNAM'), sound, snamWidth, snamExtra); break;
        default:
        {
            bool done = false;
            for (auto& c : components.all())
                if (c->canHandle(sub)) { done = c->writeSubrecord(sub, esm); break; }
            if (!done && rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        }
        }
    }

    if (!seen.contains(NAME('FNAM')) && !seen.contains(NAME('FLAG')) && (hasFlags || flags != 0))
        writeScalar(flagsName, flags, flagsWidth, flagsExtra);
    if (!seen.contains(NAME('SNAM')) && (hasSnam || sound != 0))
        writeScalar(NAME('SNAM'), sound, snamWidth, snamExtra);
    const auto leftover = [&](const QString& cls, const QVector<NAME>& names) {
        const Component* comp = components.findByName(cls);
        if (!comp) return;
        for (NAME name : names)
            if (!seen.contains(name))
                comp->writeSubrecord(name, esm);
    };
    leftover(QStringLiteral("TESModel"), { NAME('MODL'), NAME('MNAM') });
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void DoorRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    modelPath = "";
    sound = 0;
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlags = false;
    hasSnam = false;
    flagsWidth = 4;
    flagsExtra.clear();
    flagsName = NAME('FNAM');
    snamWidth = 4;
    snamExtra.clear();
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    components.clear();
}
