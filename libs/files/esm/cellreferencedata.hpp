#ifndef CELLREFERENCEDATA_HPP
#define CELLREFERENCEDATA_HPP

#include <QtGlobal>
#include <QString>

struct CellRefEntry
{
    quint32 formId;
    quint32 baseObject;
    float posX;
    float posY;
    float posZ;
    float rotX;
    float rotY;
    float rotZ;
    float scale;
    quint32 flags;

    CellRefEntry()
        : formId(0)
        , baseObject(0)
        , posX(0.0f)
        , posY(0.0f)
        , posZ(0.0f)
        , rotX(0.0f)
        , rotY(0.0f)
        , rotZ(0.0f)
        , scale(1.0f)
        , flags(0)
    {
    }

    bool isDisabled() const { return (flags & 0x01) != 0; }
    bool isHidden() const { return (flags & 0x02) != 0; }
    void setDisabled(bool v) { v ? (flags |= 0x01) : (flags &= ~0x01); }
    void setHidden(bool v) { v ? (flags |= 0x02) : (flags &= ~0x02); }
};

#endif // CELLREFERENCEDATA_HPP
