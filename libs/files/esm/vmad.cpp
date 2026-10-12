#include "vmad.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <QDataStream>
#include <QIODevice>
#include <QtMath>

bool ScriptProperty::operator==(const ScriptProperty& o) const
{
    if (name != o.name || type != o.type || status != o.status)
        return false;
    switch (type)
    {
    case Object:
        return objectVal == o.objectVal;
    case String:
        return stringVal == o.stringVal;
    case Int:
        return intVal == o.intVal;
    case Float:
        return qFuzzyCompare(floatVal, o.floatVal);
    case Bool:
        return boolVal == o.boolVal;
    case ArrayObject:
        return arrayObject == o.arrayObject;
    case ArrayString:
        return arrayString == o.arrayString;
    case ArrayInt:
        return arrayInt == o.arrayInt;
    case ArrayFloat:
        if (arrayFloat.size() != o.arrayFloat.size()) return false;
        for (int i = 0; i < arrayFloat.size(); ++i)
            if (!qFuzzyCompare(arrayFloat[i], o.arrayFloat[i])) return false;
        return true;
    case ArrayBool:
        return arrayBool == o.arrayBool;
    default:
        return true;
    }
}

bool AttachedScript::operator==(const AttachedScript& o) const
{
    return scriptName == o.scriptName && status == o.status && properties == o.properties;
}

bool VmadData::operator==(const VmadData& o) const
{
    return version == o.version && objectFormat == o.objectFormat &&
           scripts == o.scripts && rawTail == o.rawTail;
}

void VmadData::clear()
{
    version = 5;
    objectFormat = 2;
    scripts.clear();
    rawTail.clear();
}

bool VmadData::unpack(const QByteArray& data, VmadData& out)
{
    out.clear();
    if (data.size() < 6)
        return false;

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    stream >> out.version;
    stream >> out.objectFormat;
    quint16 scriptCount = 0;
    stream >> scriptCount;

    auto readString = [&stream](QString& str) -> bool {
        quint16 len = 0;
        stream >> len;
        if (len == 0) {
            str.clear();
            return true;
        }
        QByteArray bytes(len, '\0');
        if (stream.readRawData(bytes.data(), len) != len)
            return false;
        str = QString::fromUtf8(bytes);
        return true;
    };

    for (quint16 s = 0; s < scriptCount; ++s)
    {
        AttachedScript script;
        if (!readString(script.scriptName))
            return false;

        stream >> script.status;
        quint16 propCount = 0;
        stream >> propCount;

        for (quint16 p = 0; p < propCount; ++p)
        {
            ScriptProperty prop;
            if (!readString(prop.name))
                return false;

            quint8 rawType = 0;
            stream >> rawType;
            prop.type = static_cast<ScriptProperty::Type>(rawType);
            stream >> prop.status;

            switch (prop.type)
            {
            case ScriptProperty::Object:
                stream >> prop.objectVal.alias;
                stream >> prop.objectVal.unused;
                stream >> prop.objectVal.formId;
                break;
            case ScriptProperty::String:
                if (!readString(prop.stringVal))
                    return false;
                break;
            case ScriptProperty::Int:
                stream >> prop.intVal;
                break;
            case ScriptProperty::Float:
                stream >> prop.floatVal;
                break;
            case ScriptProperty::Bool:
            {
                quint8 b = 0;
                stream >> b;
                prop.boolVal = (b != 0);
                break;
            }
            case ScriptProperty::ArrayObject:
            {
                quint32 count = 0;
                stream >> count;
                prop.arrayObject.resize(count);
                for (quint32 i = 0; i < count; ++i)
                {
                    stream >> prop.arrayObject[i].alias;
                    stream >> prop.arrayObject[i].unused;
                    stream >> prop.arrayObject[i].formId;
                }
                break;
            }
            case ScriptProperty::ArrayString:
            {
                quint32 count = 0;
                stream >> count;
                prop.arrayString.resize(count);
                for (quint32 i = 0; i < count; ++i)
                {
                    if (!readString(prop.arrayString[i]))
                        return false;
                }
                break;
            }
            case ScriptProperty::ArrayInt:
            {
                quint32 count = 0;
                stream >> count;
                prop.arrayInt.resize(count);
                for (quint32 i = 0; i < count; ++i)
                    stream >> prop.arrayInt[i];
                break;
            }
            case ScriptProperty::ArrayFloat:
            {
                quint32 count = 0;
                stream >> count;
                prop.arrayFloat.resize(count);
                for (quint32 i = 0; i < count; ++i)
                    stream >> prop.arrayFloat[i];
                break;
            }
            case ScriptProperty::ArrayBool:
            {
                quint32 count = 0;
                stream >> count;
                prop.arrayBool.resize(count);
                for (quint32 i = 0; i < count; ++i)
                {
                    quint8 b = 0;
                    stream >> b;
                    prop.arrayBool[i] = (b != 0);
                }
                break;
            }
            default:
                // Unknown property type: cannot safely continue parsing scripts
                return false;
            }

            if (stream.status() != QDataStream::Ok)
                return false;

            script.properties.append(prop);
        }

        out.scripts.append(script);
    }

    const int readPos = stream.device() ? static_cast<int>(stream.device()->pos()) : 0;
    if (readPos < data.size())
    {
        out.rawTail = data.mid(readPos);
    }

    return true;
}

QByteArray VmadData::pack(const VmadData& in)
{
    if (in.isEmpty())
        return QByteArray();

    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    stream << in.version;
    stream << in.objectFormat;
    stream << static_cast<quint16>(in.scripts.size());

    auto writeString = [&stream](const QString& str) {
        const QByteArray utf8 = str.toUtf8();
        stream << static_cast<quint16>(utf8.size());
        if (!utf8.isEmpty())
            stream.writeRawData(utf8.constData(), utf8.size());
    };

    for (const auto& script : in.scripts)
    {
        writeString(script.scriptName);
        stream << script.status;
        stream << static_cast<quint16>(script.properties.size());

        for (const auto& prop : script.properties)
        {
            writeString(prop.name);
            stream << static_cast<quint8>(prop.type);
            stream << prop.status;

            switch (prop.type)
            {
            case ScriptProperty::Object:
                stream << prop.objectVal.alias;
                stream << prop.objectVal.unused;
                stream << prop.objectVal.formId;
                break;
            case ScriptProperty::String:
                writeString(prop.stringVal);
                break;
            case ScriptProperty::Int:
                stream << prop.intVal;
                break;
            case ScriptProperty::Float:
                stream << prop.floatVal;
                break;
            case ScriptProperty::Bool:
                stream << static_cast<quint8>(prop.boolVal ? 1 : 0);
                break;
            case ScriptProperty::ArrayObject:
                stream << static_cast<quint32>(prop.arrayObject.size());
                for (const auto& obj : prop.arrayObject)
                {
                    stream << obj.alias;
                    stream << obj.unused;
                    stream << obj.formId;
                }
                break;
            case ScriptProperty::ArrayString:
                stream << static_cast<quint32>(prop.arrayString.size());
                for (const auto& s : prop.arrayString)
                    writeString(s);
                break;
            case ScriptProperty::ArrayInt:
                stream << static_cast<quint32>(prop.arrayInt.size());
                for (auto i : prop.arrayInt)
                    stream << i;
                break;
            case ScriptProperty::ArrayFloat:
                stream << static_cast<quint32>(prop.arrayFloat.size());
                for (auto f : prop.arrayFloat)
                    stream << f;
                break;
            case ScriptProperty::ArrayBool:
                stream << static_cast<quint32>(prop.arrayBool.size());
                for (bool b : prop.arrayBool)
                    stream << static_cast<quint8>(b ? 1 : 0);
                break;
            default:
                break;
            }
        }
    }

    if (!in.rawTail.isEmpty())
    {
        stream.writeRawData(in.rawTail.constData(), in.rawTail.size());
    }

    return bytes;
}

bool VmadData::load(ESMReader& esm)
{
    QByteArray data;
    esm.readRawSubData(data);
    return unpack(data, *this);
}

void VmadData::save(ESMWriter& esm) const
{
    const QByteArray bytes = pack(*this);
    if (!bytes.isEmpty())
    {
        esm.startSubRecord('VMAD');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }
}
