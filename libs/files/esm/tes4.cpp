#include "tes4.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../log/logger.hpp"

#include <QDebug>

const float DEFAULT_VERSION = 0.94f;

Header::Header()
    : author(""),
      description("")
{
}

void Header::blank()
{
    recHeader = RecHeader();

    // HEDR
    version = DEFAULT_VERSION;
    numRecords = 0;
    nextObjectID = 0;

    // INTV
    internalVersion = 0;

    // INCC
    incc = 0;
}

void Header::load(ESMReader& esm)
{
    LOG_DEBUG(QString("Header::load: entered, filePos=%1").arg(esm.filePos()));
    recHeader = esm.readHeader();
    flags.val = recHeader.flags.val;
    LOG_DEBUG(QString("Header::load: readHeader done, size=%1 flags=0x%2 filePos=%3")
        .arg(recHeader.size)
        .arg(QString::number(flags.val, 16))
        .arg(esm.filePos()));

    int iter = 0;
    qint64 lastRecLeft = esm.recLeft() + 1;
    while (esm.isRecLeft())
    {
        if (esm.recLeft() < 0)
        {
            LOG_WARNING(QString("Header::load: recLeft went negative (%1), breaking").arg(esm.recLeft()));
            break;
        }
        if (esm.recLeft() >= lastRecLeft)
        {
            LOG_WARNING(QString("Header::load: recLeft not decreasing (%1 >= %2), breaking to prevent infinite loop")
                .arg(esm.recLeft()).arg(lastRecLeft));
            break;
        }
        lastRecLeft = esm.recLeft();
        qint64 recLeftBefore = esm.recLeft();
        qint64 filePosBefore = esm.filePos();
        NAME subName = esm.readNSubHeader();
        qint64 filePosAfter = esm.filePos();

        if (subName == 0)
        {
            LOG_DEBUG(QString("Header::load: readNSubHeader returned 0 at iter %1, filePos=%2, ending header").arg(iter).arg(filePosAfter));
            break;
        }

        // 'GRUP' is never a TES4 subrecord — it's a top-level record group.
        // Encountering it here means the TES4 record's declared size was
        // larger than its real body (a malformed header seen in some
        // Starfield plugins, where size is set to the whole remaining
        // file). Roll back so continueLoading reads the group as a real
        // top-level record instead of swallowing it into the header.
        if (subName == NAME('GRUP'))
        {
            esm.seekTo(filePosBefore);
            LOG_WARNING(QString("Header::load: top-level GRUP at 0x%1 before declared TES4 end (recLeft=%2); treating TES4 as ended here")
                .arg(filePosBefore).arg(esm.recLeft()));
            break;
        }

        LOG_DEBUG(QString("Header::load iter %1 subName=0x%2 filePos %3->%4 recLeft before=%5 after=%6 subLeft=%7")
            .arg(iter)
            .arg(QString::number(subName, 16))
            .arg(filePosBefore)
            .arg(filePosAfter)
            .arg(recLeftBefore)
            .arg(esm.recLeft())
            .arg(esm.subLeft()));
        switch (subName)
        {
        case 'HEDR':
        {
            version = esm.readType<float>();
            numRecords = esm.readType<qint32>();
            nextObjectID = esm.readType<quint32>();
            break;
        }
        case 'CNAM':
        {
            author = esm.readZString();
            break;
        }
        case 'SNAM':
        {
            description = esm.readZString();
            break;
        }
        case 'MAST':
        {
            MasterData mst;
            mst.name= esm.readZString();

            // A DATA subrecord (master file size / timestamp) usually follows
            // each MAST, but Starfield plugins sometimes list MASTs back-to-
            // back without DATA. Only consume DATA when it is actually next;
            // otherwise leave the stream positioned for the main loop to read
            // the following subrecord (which is typically another MAST).
            if (esm.isNextName(NAME('DATA')))
            {
                esm.readNSubHeader();
                mst.size = esm.readType<quint64>();
            }
            masters.push_back(mst);
            break;
        }
        case 'ONAM':
        {
            while (esm.isSubLeft())
            {
                overrides.push_back(esm.readType<FormID>());
            }
            break;
        }
        case 'INTV':
        {
            internalVersion = esm.readType<quint32>();
            break;
        }
        case 'INCC':
        {
            incc = esm.readType<quint32>();
            break;
        }
        default:
        {
            // Unknown subrecord: skip the data, but cap at remaining recLeft so we
            // can't underflow it. If subLeft is corrupt (bigger than recLeft), the
            // safest move is to drain the rest of the header and stop.
            qint64 sub = esm.subLeft();
            qint64 remaining = esm.recLeft();
            if (sub < 0 || sub > remaining)
            {
                LOG_WARNING(QString("Header::load: bogus subrecord size %1 (recLeft=%2) at iter %3, ending header")
                    .arg(sub).arg(remaining).arg(iter));
                if (remaining > 0)
                {
                    esm.skip(static_cast<int>(remaining));
                }
                break;
            }
            esm.skip(static_cast<int>(sub));
            break;
        }
        }
        ++iter;
        if (iter > 200)
        {
            LOG_WARNING(QString("Header::load: too many iterations (%1), breaking. recLeft=%2")
                .arg(iter).arg(esm.recLeft()));
            break;
        }
    }
}

void Header::save(ESMWriter& esm)
{
    esm.startSubRecord('HEDR');
    esm.writeType<float>(version);
    esm.writeType<qint32>(numRecords);
    esm.writeType<quint32>(nextObjectID);
    esm.endSubRecord();

    if (author.compare("") != 0)
    {
        esm.writeSubZString('CNAM', author);
    }

    if (description.compare("") != 0)
    {
        esm.writeSubZString('SNAM', description);
    }

    if (!masters.empty())
    {
        for (auto master: masters)
        {
            esm.writeSubZString('MAST', master.name);
            esm.writeSubData<quint64>('DATA', master.size);
        }
    }

    if (!overrides.empty())
    {
        esm.startSubRecord('ONAM');

        for (auto override: overrides)
        {
            esm.writeType<FormID>(override);
        }

        esm.endSubRecord();
    }

    esm.writeSubData<quint32>('INTV', internalVersion);

    if (incc != 0)
    {
        esm.writeSubData<quint32>('INCC', incc);
    }
}
