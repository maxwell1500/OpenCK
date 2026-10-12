#ifndef VMAD_HPP
#define VMAD_HPP

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QtGlobal>

class ESMReader;
class ESMWriter;

struct ScriptProperty
{
    enum Type : quint8
    {
        Null = 0,
        Object = 1,
        String = 2,
        Int = 3,
        Float = 4,
        Bool = 5,
        ArrayObject = 11,
        ArrayString = 12,
        ArrayInt = 13,
        ArrayFloat = 14,
        ArrayBool = 15
    };

    struct ObjectValue
    {
        quint16 alias = 0;
        quint16 unused = 0xFFFF;
        quint32 formId = 0;

        bool operator==(const ObjectValue& o) const
        {
            return alias == o.alias && formId == o.formId;
        }
    };

    QString name;
    Type type = Null;
    quint8 status = 1;

    ObjectValue objectVal;
    QString stringVal;
    qint32 intVal = 0;
    float floatVal = 0.0f;
    bool boolVal = false;

    QVector<ObjectValue> arrayObject;
    QVector<QString> arrayString;
    QVector<qint32> arrayInt;
    QVector<float> arrayFloat;
    QVector<bool> arrayBool;

    bool operator==(const ScriptProperty& o) const;
};

struct AttachedScript
{
    QString scriptName;
    qint8 status = 0;
    QVector<ScriptProperty> properties;

    bool operator==(const AttachedScript& o) const;
};

struct VmadData
{
    qint16 version = 5;       // 5 for Skyrim/SSE, 6 for Starfield
    qint16 objectFormat = 2;  // 2 for Skyrim 1.5+, FO4, Starfield
    QVector<AttachedScript> scripts;
    QByteArray rawTail;       // Retain trailing bytes (e.g. quest/dialogue fragments or unparsed structures)

    bool isEmpty() const { return scripts.isEmpty() && rawTail.isEmpty(); }
    void clear();

    static bool unpack(const QByteArray& data, VmadData& out);
    static QByteArray pack(const VmadData& in);

    bool load(ESMReader& esm);
    void save(ESMWriter& esm) const;

    bool operator==(const VmadData& o) const;
};

#endif // VMAD_HPP
