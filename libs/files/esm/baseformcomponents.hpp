#ifndef BASEFORMCOMPONENTS_HPP
#define BASEFORMCOMPONENTS_HPP

#include "records.hpp"

#include <QString>
#include <QVector>

// Starfield base-form component parsing (shared). Many record types — GBFM,
// STDT (stars), REFR, ACTI, … — carry a sequence of components, each opened by
// a BFCB subrecord whose payload is the component-type name (e.g.
// "BGSStarDataComponent_Component") and closed by BFCE, or implicitly by the
// next BFCB. The components and their subrecords are stored in the record's
// rawSubRecords order-preservingly; this view makes them readable without
// changing how they round-trip.
struct BaseFormComponent
{
    QString typeName;
    QVector<RawSubRecord> subrecords;

    bool hasSubrecord(NAME name) const;
    const RawSubRecord* findSubrecord(NAME name) const;
    // First little-endian uint32 of the named subrecord, or 0 when absent.
    quint32 firstU32(NAME name) const;
    // Every little-endian uint32 in the named subrecord (flattens both the
    // one-subrecord/many-value shape and the one-value-per-occurrence shape).
    QVector<quint32> u32List(NAME name) const;
    // First float of the named subrecord, or 0 when absent.
    float firstFloat(NAME name) const;
};

// Splits a record's subrecords at BFCB/BFCE. Subrecords before the first BFCB
// and after the last close are returned through leading/trailing when given.
QVector<BaseFormComponent> splitBaseFormComponents(
    const QVector<RawSubRecord>& subrecords,
    QVector<RawSubRecord>* leading = nullptr,
    QVector<RawSubRecord>* trailing = nullptr);

const BaseFormComponent* findBaseFormComponent(
    const QVector<BaseFormComponent>& components, const QString& typeName);

// Reads a little-endian uint32 / float at a byte offset (0 when out of range).
quint32 baseFormU32(const QByteArray& data, int offset);
float baseFormF32(const QByteArray& data, int offset);

#endif // BASEFORMCOMPONENTS_HPP
