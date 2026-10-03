#ifndef FACEFXANIM_HPP
#define FACEFXANIM_HPP

#include <QString>
#include <QVector>
#include <QByteArray>

namespace FaceFx {

struct AnimRecord {
    float value = 0.0f;
    quint16 field0 = 0;
    quint16 field1 = 0;
    quint16 field2 = 0;
    quint16 field3 = 0;
};

struct AnimHeader {
    quint16 format = 0;
    quint32 size = 0;
    QByteArray id; // 20 bytes; last 8 observed constant per entry type
    quint32 count = 0;
};

// Parser for the Starfield FaceFX runtime blob (".ffxanim"). Strict: it
// refuses files whose size field does not match the count-implied layout,
// so a bad read trips here rather than leeching telemetry garbage into a
// future runtime. Layout confirmed against the FFX animations held by
// Starfield - FaceAnimation01.ba2:
//   u8[6] __ffx\0, u16 format, u32 size, u8[20] id, u32 count, then
//   "count" 12-byte records { f32 value, u16 field0..field3 }.
class FaceFxAnim {
public:
    bool load(const QString& path);
    bool load(const QByteArray& data);

    const AnimHeader& header() const { return m_header; }
    const QVector<AnimRecord>& records() const { return m_records; }
    QString errorString() const { return m_error; }

private:
    bool parse(const QByteArray& data);
    void setError(const QString& msg);

    AnimHeader m_header;
    QVector<AnimRecord> m_records;
    QString m_error;
};

} // namespace FaceFx

#endif
