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

// Volumes_Component::VLMS — Starfield's volume entries (the shipped
// representation behind reflection probes / probe grid volumes and trigger
// volumes). Layout from xEdit wbDefinitionsSF1 (Volumes_Component +
// wbVLMSTypeDecider) and validated against every VLMS in Starfield.esm
// (11,765 subrecords, all consuming exactly their own size):
//
//   uint32 count
//   repeat count times:
//     uint32 type             (1, 3 or 5 in shipped data)
//     float  matrix[16]       row-major transform
//     float  a, b, c
//     type-specific: type 1 -> 1 float, type 3 -> 2 floats, type 5 -> 3 floats
struct VolumeEntry
{
    quint32 type = 0;
    float matrix[16] = {};
    float a = 0.0f, b = 0.0f, c = 0.0f;
    QVector<float> extra;   // 1, 2 or 3 floats depending on type

    static constexpr int kMatrixFloats = 16;
};

// Parses a VLMS payload. Returns false (leaving `out` empty) when the buffer is
// malformed; shipped data never fails this.
bool parseVolumePayload(const QByteArray& data, QVector<VolumeEntry>& out);

#endif // BASEFORMCOMPONENTS_HPP
