#ifndef GMST_H
#define GMST_H

class ESMReader;
class ESMWriter;

#include "variant.hpp"

#include <QString>
#include <QVariant>

struct GameSetting
{
    QString editorId;
    quint32 formId = 0;
    Variant value;

    void load(ESMReader& esm, bool base = false);
    void save(ESMWriter& esm) const;
    void blank();
};

bool operator==(const GameSetting& l, const GameSetting& r);

#endif // GMST_H
