#ifndef PACKRECORD_H
#define PACKRECORD_H

#include "records.hpp"
#include "variant.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct PackageRecord
{
    QString editorId;
    quint32 formId;
    quint32 flags;

    // DESC - Description
    QString description;

    // DATA - Package Data
    struct PackageData
    {
        quint8 type;
        quint8 flags;
        quint8 aiPackage;
        quint8 aiClass;

        PackageData() : type(0), flags(0), aiPackage(0), aiClass(0) {}
    } data;

    // SNAM - Script Name
    QString scriptName;

    // TNAM - Script Data
    quint32 scriptData;

    // CNAM - Menu Icon
    quint32 menuIcon;

    // NNAM - Menu Background
    quint32 menuBackground;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
};

#endif // PACKRECORD_H
