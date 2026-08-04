#ifndef NavmRECORD_H
#define NavmRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
#include <QtGui/QVector3D>
class ESMReader;
class ESMWriter;

struct NavmTriangle {
    qint16 v0, v1, v2;
    qint16 edge0 = -1, edge1 = -1, edge2 = -1;
    quint16 flags = 0;

    inline bool operator==(const NavmTriangle& o) const {
        return v0 == o.v0 && v1 == o.v1 && v2 == o.v2 &&
               edge0 == o.edge0 && edge1 == o.edge1 && edge2 == o.edge2 &&
               flags == o.flags;
    }
    inline bool operator!=(const NavmTriangle& o) const { return !(*this == o); }
};

struct NavmRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 cellFormId = 0;
    QVector<QVector3D> vertices;
    QVector<NavmTriangle> triangles;
    QVector<quint8> coverData; // NVCV: per-vertex cover flags
    QVector<quint32> externalConnections;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const NavmRecord& l, const NavmRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.cellFormId == r.cellFormId && l.vertices == r.vertices
        && l.triangles == r.triangles && l.coverData == r.coverData
        && l.externalConnections == r.externalConnections
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const NavmRecord& l, const NavmRecord& r) { return !(l == r); }
#endif
