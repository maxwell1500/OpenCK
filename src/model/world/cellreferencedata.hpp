#ifndef CELLREFERENCEDATA_H
#define CELLREFERENCEDATA_H

#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/common.hpp"

#include <QDataStream>
#include <QVector>
#include <QVector3D>
#include <limits>

struct CellReference
{
    quint32 formId = 0;
    quint32 baseObjectFormId = 0;
    float posX = 0, posY = 0, posZ = 0;
    float rotX = 0, rotY = 0, rotZ = 0;
    float scale = 1.0f;
    quint32 flags = 0;

    bool isDisabled() const { return (flags & 0x01) != 0; }
    bool isHidden() const { return (flags & 0x02) != 0; }
};

class CellReferenceData
{
public:
    CellReferenceData() = default;

    bool loadFromCellRecord(const CellRecord& cell);
    QVector<CellReference> getReferences() const { return m_references; }
    void addReference(const CellReference& ref);
    void removeReference(quint32 formId);
    void updateReference(const CellReference& ref);

    QVector3D getBoundingBoxMin() const;
    QVector3D getBoundingBoxMax() const;
    int referenceCount() const { return m_references.size(); }

private:
    QVector<CellReference> m_references;
};

inline bool CellReferenceData::loadFromCellRecord(const CellRecord& cell)
{
    m_references.clear();

    for (const auto& sub : cell.rawSubRecords)
    {
        if (sub.name != 'REFR')
            continue;

        QDataStream stream(sub.data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        CellReference ref;
        stream >> ref.formId;
        stream >> ref.baseObjectFormId;
        stream >> ref.posX >> ref.posY >> ref.posZ;
        stream >> ref.rotX >> ref.rotY >> ref.rotZ;
        stream >> ref.scale;
        stream >> ref.flags;

        if (stream.status() != QDataStream::Ok)
            continue;

        m_references.append(ref);
    }

    return !m_references.isEmpty();
}

inline void CellReferenceData::addReference(const CellReference& ref)
{
    m_references.append(ref);
}

inline void CellReferenceData::removeReference(quint32 formId)
{
    for (int i = m_references.size() - 1; i >= 0; --i)
    {
        if (m_references[i].formId == formId)
        {
            m_references.remove(i);
            return;
        }
    }
}

inline void CellReferenceData::updateReference(const CellReference& ref)
{
    for (int i = 0; i < m_references.size(); ++i)
    {
        if (m_references[i].formId == ref.formId)
        {
            m_references[i] = ref;
            return;
        }
    }
    m_references.append(ref);
}

inline QVector3D CellReferenceData::getBoundingBoxMin() const
{
    if (m_references.isEmpty())
        return QVector3D(0, 0, 0);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    for (const auto& ref : m_references)
    {
        if (ref.posX < minX) minX = ref.posX;
        if (ref.posY < minY) minY = ref.posY;
        if (ref.posZ < minZ) minZ = ref.posZ;
    }

    return QVector3D(minX, minY, minZ);
}

inline QVector3D CellReferenceData::getBoundingBoxMax() const
{
    if (m_references.isEmpty())
        return QVector3D(0, 0, 0);

    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& ref : m_references)
    {
        if (ref.posX > maxX) maxX = ref.posX;
        if (ref.posY > maxY) maxY = ref.posY;
        if (ref.posZ > maxZ) maxZ = ref.posZ;
    }

    return QVector3D(maxX, maxY, maxZ);
}

#endif // CELLREFERENCEDATA_H
