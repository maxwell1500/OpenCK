#ifndef GLOB_H
#define GLOB_H

class ESMReader;
class ESMWriter;

#include "variant.hpp"

#include <QString>
#include <QVariant>

struct GlobalVariable
{
    enum Flag
    {
        None = 0,
        Constant = 0x40
    };

    QString editorId;
    Variant value;

    void load(ESMReader& esm, bool base = false);
    void save(ESMWriter& esm) const;
    void blank();

    bool constant;
};

inline bool operator==(const GlobalVariable& l, const GlobalVariable& r)
{
    return l.editorId == r.editorId && l.value == r.value && l.constant == r.constant;
}

inline bool operator!=(const GlobalVariable& l, const GlobalVariable& r)
{
    return !(l == r);
}

#endif // GLOB_H
