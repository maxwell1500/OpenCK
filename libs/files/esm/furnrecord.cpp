#include "furnrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"

void FurnRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();

    loadOrder.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': case 'FLAG':
                flags = esm.readSubU32(&flagsWidth);
                hasFlags = true;
                flagsSpelling = sub;
                handled = true;
                break;
            default: break;
        }
        if (handled) continue;

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

        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
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

void FurnRecord::save(ESMWriter& esm) const
{
    SubrecordReplay replay;
    replay.init(rawSubRecords);

    const auto writeFlags = [&]()
    {
        esm.startSubRecord(flagsSpelling);
        const quint8 w = flagsWidth == 0 ? 1 : flagsWidth;
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false;
    auto* modelComp = static_cast<tescomponents::TESModel_Component*>(
        const_cast<FurnRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    int mnamCur = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFlags && (hasFlags || flags != 0)) { writeFlags(); wroteFlags = true; }
                break;
            case 'MNAM':
                // MNAM repeats (FURN data + LOD); non-last occurrences replay
                // verbatim from the occurrence list.
                if (modelComp && mnamCur >= 0 && mnamCur < modelComp->mnamRaws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ sub, modelComp->mnamRaws[mnamCur] });
                else if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                ++mnamCur;
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && (hasFlags || flags != 0))
        writeFlags();

    replay.writeLeftover(esm);
}

void FurnRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    modelPath = "";
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    components.clear();
}
