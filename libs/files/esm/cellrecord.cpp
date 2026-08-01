#include "cellrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../log/logger.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void CellRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void CellRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    qint64 filePos = esm.filePos();
    qint64 recLeft = esm.recLeft();
    quint64 peek = esm.peekType<quint64>();
    LOG_DEBUG(QString("CellRecord::load: formId=0x%1 recLeft=%2 filePos=0x%3 peek=0x%4")
        .arg(formId, 8, 16, QChar('0'))
        .arg(recLeft)
        .arg(filePos, 0, 16)
        .arg(peek, 16, 16, QChar('0')));
    int iter = 0;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        LOG_DEBUG(QString("CellRecord::load iter %1 sub=0x%2 recLeft=%3 subLeft=%4")
            .arg(iter).arg(QString::number(sub, 16)).arg(esm.recLeft()).arg(esm.subLeft()));
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        if (sub == 'FULL')
        {
            auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
            if (fn) fn->fullName = esm.readZString();
            continue;
        }
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'DATA': flags = esm.readType<quint8>(); break;
        case 'XCLC':
        {
            cellX = esm.readType<qint32>();
            cellY = esm.readType<qint32>();
            break;
        }
        case 'XOWN': owner = esm.readType<quint32>(); break;
        case 'XLOC': lockLevel = esm.readType<quint32>(); break;
        case 'XCLW':
            hasWaterHeight = true;
            waterHeight = esm.readType<float>();
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
        ++iter;
        if (iter > 500)
        {
            LOG_WARNING(QString("CellRecord::load: too many iters (%1), breaking. recLeft=%2").arg(iter).arg(esm.recLeft()));
            break;
        }
    }
    auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (fn) cellName = fn->fullName;
    LOG_DEBUG(QString("CellRecord::load complete, recLeft=%1").arg(esm.recLeft()));
}

void CellRecord::save(ESMWriter& esm) const
{
    auto* fn = const_cast<CellRecord*>(this)->components.findByName(QStringLiteral("TESFullName"));
    if (fn) static_cast<tescomponents::TESFullName_Component*>(fn)->fullName = cellName;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint8>('DATA', flags);
    esm.startSubRecord('XCLC');
    esm.writeType<qint32>(cellX);
    esm.writeType<qint32>(cellY);
    esm.endSubRecord();
    esm.writeSubData<quint32>('XOWN', owner);
    esm.writeSubData<quint32>('XLOC', lockLevel);
    if (hasWaterHeight) {
        esm.writeSubData<float>('XCLW', waterHeight);
    }
    components.saveAll(esm);
    esm.writeSubZString('FULL', cellName);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void CellRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    cellX = 0;
    cellY = 0;
    owner = 0;
    lockLevel = 0;
    cellName = "";
    hasWaterHeight = false;
    waterHeight = 0.0f;
    rawSubRecords.clear();
    initComponents();
}
