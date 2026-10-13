#ifndef MATDATABASE_HPP
#define MATDATABASE_HPP

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>

// Reader for Starfield's compiled material database, materialsbeta.cdb. The
// base game keeps its material objects there instead of shipping loose .mat
// files, so a material referenced by a NIF has to be resolved from the binary.
//
// The file is a stream of DIFF records:
//
//     "DIFF" + u32 totalLen + [u32 fieldType + payload(totalLen - 4)]
//
// The texture-set region alternates object-name records (fieldType 0x1F4) with
// the texture-file records they own (fieldType 0x38A), so ownership is the
// record that follows an "_TextureSet<n>" name until the next name record.
//
// Field payload encoding for both record kinds:
//
//     u16 0 + u16 length + text + 00 FF FF
//
// and the length is one greater than the text, so records are read by
// position rather than by that count.
class MatDatabase
{
public:
    // Opens the database and indexes every texture set it contains. Returns
    // false if the file could not be read or holds no texture records.
    explicit MatDatabase(const QString& filePath);

    bool isValid() const { return m_valid; }

    // The .mat JSON a texture set serializes to, or an empty string when the
    // database holds no texture set with that name. Emitting the same JSON a
    // loose file uses lets MatFile::textureSlots() read it unchanged.
    QString matDocument(const QString& textureSetName) const;

private:
    struct Slot
    {
        int index;
        QString path;
    };

    QHash<QString, QList<Slot>> m_sets;
    bool m_valid = false;
};

#endif // MATDATABASE_HPP
