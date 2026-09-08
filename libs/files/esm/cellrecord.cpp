#include "cellrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../log/logger.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

#include <QHash>

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
    loadOrder.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        LOG_DEBUG(QString("CellRecord::load iter %1 sub=0x%2 recLeft=%3 subLeft=%4")
            .arg(iter).arg(QString::number(sub, 16)).arg(esm.recLeft()).arg(esm.subLeft()));
        // readNSubHeader returns 0 when the record is drained or its tail
        // is a run of zero subrecords (some Starfield mods pad records with
        // zeros). Stop here and drain whatever remains so the reader stays
        // aligned on the next record.
        if (sub == 0)
        {
            esm.skipRemainingRecord();
            break;
        }
        loadOrder.append(sub);
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
            hasFull = true;
            continue;
        }
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'DATA':
        {
            // Starfield writes DATA as 4 bytes, older games as 1, and some
            // variants carry more. Decode the leading bytes little-endian
            // into flags and keep any tail verbatim so the save path
            // re-emits the identical payload.
            qint64 n = esm.subLeft();
            if (n <= 0)
            {
                dataWidth = 0;
                flags = 0;
                dataExtra.clear();
            }
            else
            {
                dataWidth = static_cast<quint8>(qMin<qint64>(n, 255));
                const qint64 head = qMin<qint64>(n, 4);
                quint32 v = 0;
                for (qint64 i = 0; i < head; ++i)
                    v |= quint32(esm.readType<quint8>()) << (8 * i);
                flags = v;
                dataExtra.clear();
                for (qint64 i = head; i < n; ++i)
                    dataExtra.append(static_cast<char>(esm.readType<quint8>()));
            }
            hasData = true;
            break;
        }
        case 'XCLC':
        {
            cellX = esm.readType<qint32>();
            cellY = esm.readType<qint32>();
            // Exterior XCLC is 12 bytes (grid + extra dword); keep the tail.
            xclcExtra.clear();
            qint64 tail = esm.subLeft();
            for (qint64 i = 0; i < tail; ++i)
                xclcExtra.append(static_cast<char>(esm.readType<quint8>()));
            hasXclc = true;
            break;
        }
        case 'XOWN': owner = esm.readType<quint32>(); hasOwner = true; break;
        case 'XLOC': lockLevel = esm.readType<quint32>(); hasLock = true; break;
        case 'XCLW':
            // Starfield writes XCLW as an empty subrecord (size 0) when a
            // cell has no water height. Only read the float when it exists;
            // an unconditional read walked 4 bytes past the record end and
            // desynced every following record.
            hasXclw = true;
            if (esm.subLeft() >= 4)
            {
                hasWaterHeight = true;
                waterHeight = esm.readType<float>();
            }
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

    auto writeData = [&]() {
        esm.startSubRecord(NAME('DATA'));
        const int head = qMin<int>(dataWidth, 4);
        for (int i = 0; i < head; ++i)
            esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
        if (!dataExtra.isEmpty())
            esm.writeRawData(dataExtra.constData(), dataExtra.size());
        esm.endSubRecord();
    };
    auto writeXclc = [&]() {
        esm.startSubRecord(NAME('XCLC'));
        esm.writeType<qint32>(static_cast<qint32>(cellX));
        esm.writeType<qint32>(static_cast<qint32>(cellY));
        if (!xclcExtra.isEmpty())
            esm.writeRawData(xclcExtra.constData(), xclcExtra.size());
        esm.endSubRecord();
    };
    auto writeXclw = [&]() {
        if (hasWaterHeight)
            esm.writeSubData<float>('XCLW', waterHeight);
        else
        {
            esm.startSubRecord(NAME('XCLW'));
            esm.endSubRecord();
        }
    };

    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    bool wroteEdid = false, wroteData = false, wroteFull = false,
        wroteXclc = false, wroteOwner = false, wroteLock = false,
        wroteXclw = false;

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
        case 'FULL':
            if (!wroteFull && (hasFull || !cellName.isEmpty()))
                esm.writeSubZString('FULL', cellName);
            wroteFull = true;
            break;
        case 'DATA':
            if (!wroteData)
            {
                writeData();
                wroteData = true;
            }
            break;
        case 'XCLC':
            if (!wroteXclc && hasXclc)
                writeXclc();
            wroteXclc = true;
            break;
        case 'XOWN':
            if (!wroteOwner && (hasOwner || owner != 0))
                esm.writeSubData<quint32>('XOWN', owner);
            wroteOwner = true;
            break;
        case 'XLOC':
            if (!wroteLock && (hasLock || lockLevel != 0))
                esm.writeSubData<quint32>('XLOC', lockLevel);
            wroteLock = true;
            break;
        case 'XCLW':
            if (!wroteXclw && hasXclw)
                writeXclw();
            wroteXclw = true;
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
    if (!wroteFull && !cellName.isEmpty())
        esm.writeSubZString('FULL', cellName);
    if (!wroteData)
        writeData();
    if (!wroteXclc && (cellX != 0 || cellY != 0))
        writeXclc();
    if (!wroteOwner && owner != 0)
        esm.writeSubData<quint32>('XOWN', owner);
    if (!wroteLock && lockLevel != 0)
        esm.writeSubData<quint32>('XLOC', lockLevel);
    if (!wroteXclw && hasWaterHeight)
        esm.writeSubData<float>('XCLW', waterHeight);

    for (auto it = rawByName.constBegin(); it != rawByName.constEnd(); ++it)
    {
        const QVector<int>& idx = it.value();
        int& cur = rawCursor[it.key()];
        while (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    }
}

void CellRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    dataWidth = 1;
    dataExtra.clear();
    cellX = 0;
    cellY = 0;
    xclcExtra.clear();
    owner = 0;
    lockLevel = 0;
    cellName = "";
    hasWaterHeight = false;
    waterHeight = 0.0f;
    rawSubRecords.clear();
    loadOrder.clear();
    hasData = false;
    hasFull = false;
    hasXclc = false;
    hasOwner = false;
    hasLock = false;
    hasXclw = false;
    initComponents();
}
