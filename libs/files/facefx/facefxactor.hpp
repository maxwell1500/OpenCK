#ifndef FACEFXACTOR_HPP
#define FACEFXACTOR_HPP

#include <QString>
#include <QVector>
#include <QByteArray>

namespace FaceFx {

struct TypeEntry {
    quint32 id = 0;
    QString name;
};

struct NameEntry {
    quint32 id = 0;
    QString name;
    quint32 payload[3] = { 0, 0, 0 };
};

struct ActorHeader {
    quint32 version = 0;
    quint32 opaque = 0;
    QString publisher;
    QString project;
    quint32 constant1000 = 0;
    quint16 constant2 = 0;
    quint32 typeCount = 0;
    quint32 fieldB = 0;
    quint32 fieldC = 0;
    quint32 fieldD = 0;
};

class FaceFxActor {
public:
    bool load(const QString& path);
    bool load(const QByteArray& data);

    const ActorHeader& header() const { return m_header; }
    const QVector<TypeEntry>& types() const { return m_types; }
    const QVector<NameEntry>& names() const { return m_names; }
    QString errorString() const { return m_error; }

private:
    bool parse(const QByteArray& data);
    void setError(const QString& msg);

    ActorHeader m_header;
    QVector<TypeEntry> m_types;
    QVector<NameEntry> m_names;
    QString m_error;
};

} // namespace FaceFx

#endif
