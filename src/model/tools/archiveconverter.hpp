#ifndef ARCHIVECONVERTER_HPP
#define ARCHIVECONVERTER_HPP

#include "../../esm/gameformat.hpp"

#include <QString>

// Cross-game archive conversion. Morrowind (MWSA BSA), Oblivion/Skyrim/
// Fallout (classic 'BSA\0' family at 0x67/0x68/0x69) and Starfield (BA2) all
// use different containers, so moving an asset tree to another game's format
// means re-creating it entirely: extract every payload, rebuild the container
// in the target game's on-disk layout, verify.
//
// The source bytes never pass through a format guess here: BsaArchive and
// Ba2Archive read their own containers, and the writer that runs is the one
// that game's targetsForGame()/create() prescribes. A conversion that cannot
// be verified by re-reading the result reports failure instead of shipping it.
struct ArchiveConversionReport
{
    int fileCount = 0;
    qint64 bytesIn = 0;
    qint64 bytesOut = 0;
    // Entries that failed verification, as "<path>: reason".
    QStringList failures;

    bool ok() const { return failures.isEmpty(); }
    QString summary() const;
};

class ArchiveConverter
{
public:
    // Rebuild a classic BSA (any version 0x67/0x68/0x69) in the target
    // game's accepted format, compressing payloads with that version's codec.
    static ArchiveConversionReport convertBsa(const QString& sourcePath,
                                              GameFormat::Game targetGame,
                                              const QString& outputPath,
                                              bool compress = true);

    // Repackage a classic BSA as a Starfield/skyrim BA2 (GNRL general or
    // DX10 texture container).
    static ArchiveConversionReport convertBsaToBa2(const QString& sourcePath,
                                                   const QString& outputPath,
                                                   bool compress = true,
                                                   const QString& archiveType = QStringLiteral("GNRL"));

    // Repackage a BA2 back into a classic BSA for the target game.
    static ArchiveConversionReport convertBa2ToBsa(const QString& sourcePath,
                                                   GameFormat::Game targetGame,
                                                   const QString& outputPath,
                                                   bool compress = true);

    // Re-read a written archive and compare every payload against the
    // source bytes. Returns a report whose failures list mismatches.
    static ArchiveConversionReport verifyBsa(const QString& writtenPath,
                                             const QString& sourcePath);
};

#endif // ARCHIVECONVERTER_HPP
