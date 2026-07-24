#include "packrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <QDataStream>

void PackageRecord::load(ESMReader& esm, bool base)
{
    editorId = esm.readSubZString('EDID');
    formId = esm.readType<quint32>();
    flags = esm.readType<quint32>();

    while (esm.isRecLeft())
    {
        NAME subName = esm.readName();
        switch (subName)
        {
        case 'DESC':
            description = esm.readZString();
            break;
        case 'DATA':
        {
            esm.readSubHeader();
            data.type = esm.readType<quint8>();
            data.flags = esm.readType<quint8>();
            data.aiPackage = esm.readType<quint8>();
            data.aiClass = esm.readType<quint8>();
            break;
        }
        case 'SNAM':
            scriptName = esm.readZString();
            break;
        case 'TNAM':
            scriptData = esm.readType<quint32>();
            break;
        case 'CNAM':
            menuIcon = esm.readType<quint32>();
            break;
        case 'NNAM':
            menuBackground = esm.readType<quint32>();
            break;
        default:
            esm.skipSub();
            break;
        }
    }
}

void PackageRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeType<quint32>('PACK');
    esm.writeType<quint32>(formId);
    esm.writeType<quint32>(flags);

    if (!description.isEmpty())
    {
        esm.writeSubZString('DESC', description);
    }

    esm.startSubRecord('DATA');
    esm.writeType<quint8>(data.type);
    esm.writeType<quint8>(data.flags);
    esm.writeType<quint8>(data.aiPackage);
    esm.writeType<quint8>(data.aiClass);
    esm.endSubRecord();

    if (!scriptName.isEmpty())
    {
        esm.writeSubZString('SNAM', scriptName);
    }

    esm.writeType<quint32>('TNAM');
    esm.writeType<quint32>(scriptData);

    esm.writeType<quint32>('CNAM');
    esm.writeType<quint32>(menuIcon);

    esm.writeType<quint32>('NNAM');
    esm.writeType<quint32>(menuBackground);
}
