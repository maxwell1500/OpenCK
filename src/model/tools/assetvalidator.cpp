#include "assetvalidator.hpp"
#include "assetresolver.hpp"
#include <functional>
#include "logger.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDataStream>
#include <QSet>
#include <QMap>
#include <QPair>

#include "../../../libs/files/esm/tes4.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/sounrecord.hpp"
#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/lcrt.hpp"
#include "../../../libs/files/esm/Packagerecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/components/tier1_components.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "formcomponentsresolver.hpp"
#include "../../../libs/components/formcomponents.hpp"
#include "../../../libs/components/editorproperty.hpp"

// ============================================================================
// ValidationReport
// ============================================================================

int AssetValidator::ValidationReport::errors() const
{
    int count = 0;
    for (const auto& issue : issues)
        if (issue.severity == ValidationIssue::Error) count++;
    return count;
}

int AssetValidator::ValidationReport::warnings() const
{
    int count = 0;
    for (const auto& issue : issues)
        if (issue.severity == ValidationIssue::Warning) count++;
    return count;
}

int AssetValidator::ValidationReport::infos() const
{
    int count = 0;
    for (const auto& issue : issues)
        if (issue.severity == ValidationIssue::Info) count++;
    return count;
}

// ============================================================================
// Helpers
// ============================================================================

AssetValidator::ValidationReport AssetValidator::mergeReports(const QVector<ValidationReport>& reports)
{
    ValidationReport merged;
    for (const auto& report : reports)
    {
        for (const auto& issue : report.issues)
            merged.issues.append(issue);
    }
    return merged;
}

bool AssetValidator::isPowerOf2(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

bool AssetValidator::isSupportedDdsFormat(quint32 format)
{
    // Common supported DXT/BC formats and uncompressed BGRA
    switch (format)
    {
    case 0x31545844: // DXT1 ("DXTC" in little-endian = 0x44585443, then DDS header)
    case 0x33545844: // DXT3
    case 0x35545844: // DXT5
    case 0x00000000: // BGRA8 uncompressed (FourCC = 0)
        return true;
    default:
        return false;
    }
}

bool AssetValidator::isWavValidSampleRate(quint32 sampleRate)
{
    return sampleRate == 22050 || sampleRate == 44100 || sampleRate == 48000;
}

bool AssetValidator::isWavValidBitDepth(quint16 bitsPerSample)
{
    return bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32;
}

// ============================================================================
// validateNif
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateNif(const QString& nifPath)
{
    ValidationReport report;

    QFileInfo fileInfo(nifPath);
    if (!fileInfo.exists())
    {
        report.issues.append({ValidationIssue::Error, "NIF",
            QString("NIF file does not exist: %1").arg(nifPath), "", nifPath});
        return report;
    }

    QFile file(nifPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        report.issues.append({ValidationIssue::Error, "NIF",
            QString("Cannot open NIF file: %1").arg(file.errorString()), "", nifPath});
        return report;
    }

    // Check NiHeader magic bytes: "NetImmerse File Format" header
    // NIF files start with the string "NetImmerse File Format" or "Gamebryo File Format"
    char headerBytes[40] = {};
    qint64 bytesRead = file.read(headerBytes, sizeof(headerBytes) - 1);
    if (bytesRead < 20)
    {
        report.issues.append({ValidationIssue::Error, "NIF",
            "NIF file is too small to contain a valid header.", "", nifPath});
        return report;
    }

    QString headerStr = QString::fromLatin1(headerBytes);
    if (!headerStr.startsWith("NetImmerse File Format") && !headerStr.startsWith("Gamebryo File Format"))
    {
        report.issues.append({ValidationIssue::Error, "NIF",
            "NIF file does not start with expected NiHeader magic bytes.", "", nifPath});
        return report;
    }

    // Check file size warning (>10MB)
    if (fileInfo.size() > 10 * 1024 * 1024)
    {
        report.issues.append({ValidationIssue::Warning, "NIF",
            QString("NIF file is very large (%1 MB). This may indicate unoptimized geometry.")
                .arg(fileInfo.size() / (1024 * 1024)), "", nifPath});
    }

    return report;
}

// ============================================================================
// validateTexture
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateTexture(const QString& texPath)
{
    ValidationReport report;

    QFileInfo fileInfo(texPath);
    if (!fileInfo.exists())
    {
        report.issues.append({ValidationIssue::Error, "Texture",
            QString("Texture file does not exist: %1").arg(texPath), "", texPath});
        return report;
    }

    QFile file(texPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        report.issues.append({ValidationIssue::Error, "Texture",
            QString("Cannot open texture file: %1").arg(file.errorString()), "", texPath});
        return report;
    }

    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "dds")
    {
        // Check DDS magic number: "DDS " = 0x20534444
        quint32 magic = 0;
        if (file.read(reinterpret_cast<char*>(&magic), sizeof(magic)) != sizeof(magic))
        {
            report.issues.append({ValidationIssue::Error, "Texture",
                "Cannot read DDS header magic.", "", texPath});
            return report;
        }

        if (magic != 0x20534444)
        {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file has invalid magic number (expected 0x20534444).", "", texPath});
            return report;
        }

        // Read DDS header fields
        quint32 headerSize = 0;
        if (file.read(reinterpret_cast<char*>(&headerSize), sizeof(headerSize)) != sizeof(headerSize)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read header size.", "", texPath});
            return report;
        }

        quint32 flags = 0;
        if (file.read(reinterpret_cast<char*>(&flags), sizeof(flags)) != sizeof(flags)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read flags.", "", texPath});
            return report;
        }

        quint32 height = 0, width = 0;
        if (file.read(reinterpret_cast<char*>(&height), sizeof(height)) != sizeof(height) ||
            file.read(reinterpret_cast<char*>(&width), sizeof(width)) != sizeof(width)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read dimensions.", "", texPath});
            return report;
        }

        quint32 pitchOrLinearSize = 0;
        if (file.read(reinterpret_cast<char*>(&pitchOrLinearSize), sizeof(pitchOrLinearSize)) != sizeof(pitchOrLinearSize)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read pitch/linear size.", "", texPath});
            return report;
        }

        quint32 depth = 0;
        if (file.read(reinterpret_cast<char*>(&depth), sizeof(depth)) != sizeof(depth)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read depth.", "", texPath});
            return report;
        }

        quint32 mipCount = 0;
        if (file.read(reinterpret_cast<char*>(&mipCount), sizeof(mipCount)) != sizeof(mipCount)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read mip count.", "", texPath});
            return report;
        }

        file.seek(84); // FourCC offset in DDS header
        quint32 fourCC = 0;
        if (file.read(reinterpret_cast<char*>(&fourCC), sizeof(fourCC)) != sizeof(fourCC)) {
            report.issues.append({ValidationIssue::Error, "Texture",
                "DDS file truncated: could not read FourCC.", "", texPath});
            return report;
        }

        if (!isPowerOf2(static_cast<int>(height)) || !isPowerOf2(static_cast<int>(width)))
        {
            report.issues.append({ValidationIssue::Warning, "Texture",
                QString("DDS dimensions are not power-of-2 (%1x%2).")
                    .arg(width).arg(height), "", texPath});
        }

        if (!isSupportedDdsFormat(fourCC))
        {
            report.issues.append({ValidationIssue::Warning, "Texture",
                QString("DDS format FourCC 0x%1 may not be supported by the engine.")
                    .arg(fourCC, 0, 16), "", texPath});
        }
    }
    else if (suffix == "tga")
    {
        // Check TGA signature
        char sigBytes[18] = {};
        file.read(sigBytes, sizeof(sigBytes));
        quint8 idLength = static_cast<quint8>(sigBytes[0]);
        quint8 colorMapType = static_cast<quint8>(sigBytes[1]);
        quint8 imageType = static_cast<quint8>(sigBytes[2]);

        if (imageType < 1 || imageType > 11)
        {
            report.issues.append({ValidationIssue::Error, "Texture",
                QString("TGA file has unsupported image type: %1").arg(imageType), "", texPath});
        }

        if (colorMapType != 0 && colorMapType != 1)
        {
            report.issues.append({ValidationIssue::Warning, "Texture",
                QString("TGA file has unusual color map type: %1").arg(colorMapType), "", texPath});
        }
    }
    else
    {
        report.issues.append({ValidationIssue::Info, "Texture",
            QString("Unrecognized texture format: .%1").arg(suffix), "", texPath});
    }

    return report;
}

// ============================================================================
// validateSound
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateSound(const QString& soundPath)
{
    ValidationReport report;

    QFileInfo fileInfo(soundPath);
    if (!fileInfo.exists())
    {
        report.issues.append({ValidationIssue::Error, "Sound",
            QString("Sound file does not exist: %1").arg(soundPath), "", soundPath});
        return report;
    }

    QFile file(soundPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        report.issues.append({ValidationIssue::Error, "Sound",
            QString("Cannot open sound file: %1").arg(file.errorString()), "", soundPath});
        return report;
    }

    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "wav")
    {
        // Check RIFF header
        char riffHeader[12] = {};
        if (file.read(riffHeader, sizeof(riffHeader)) != sizeof(riffHeader))
        {
            report.issues.append({ValidationIssue::Error, "Sound",
                "WAV file is too small to contain a RIFF header.", "", soundPath});
            return report;
        }

        if (QByteArray(riffHeader, 4) != "RIFF")
        {
            report.issues.append({ValidationIssue::Error, "Sound",
                "WAV file does not start with RIFF header.", "", soundPath});
            return report;
        }

        if (QByteArray(riffHeader + 8, 4) != "WAVE")
        {
            report.issues.append({ValidationIssue::Error, "Sound",
                "WAV file is not in WAVE format.", "", soundPath});
            return report;
        }

        // Read fmt chunk to get sample rate and bit depth
        char chunkHeader[8] = {};
        while (file.read(chunkHeader, 8) == 8)
        {
            QByteArray chunkId(chunkHeader, 4);
            quint32 chunkSize = 0;
            memcpy(&chunkSize, chunkHeader + 4, 4);

            if (chunkId == "fmt ")
            {
                quint16 audioFormat = 0;
                quint16 numChannels = 0;
                quint32 sampleRate = 0;
                quint32 byteRate = 0;
                quint16 blockAlign = 0;
                quint16 bitsPerSample = 0;

                file.read(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));
                file.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));
                file.read(reinterpret_cast<char*>(&sampleRate), sizeof(sampleRate));
                file.read(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));
                file.read(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));
                file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));

                if (!isWavValidSampleRate(sampleRate))
                {
                    report.issues.append({ValidationIssue::Warning, "Sound",
                        QString("WAV sample rate %1 is not standard (expected 22050, 44100, or 48000).")
                            .arg(sampleRate), "", soundPath});
                }

                if (!isWavValidBitDepth(bitsPerSample))
                {
                    report.issues.append({ValidationIssue::Warning, "Sound",
                        QString("WAV bit depth %1 is not standard (expected 16, 24, or 32).")
                            .arg(bitsPerSample), "", soundPath});
                }

                if (numChannels == 0 || numChannels > 8)
                {
                    report.issues.append({ValidationIssue::Warning, "Sound",
                        QString("WAV channel count %1 is unusual.").arg(numChannels), "", soundPath});
                }

                break;
            }
            else
            {
                // Skip this chunk
                file.seek(file.pos() + chunkSize);
                if (chunkSize % 2 != 0)
                    file.seek(file.pos() + 1); // Pad byte
            }
        }
    }
    else if (suffix == "ogg" || suffix == "opus")
    {
        // OGG/Opus files: just check magic bytes
        char oggMagic[4] = {};
        if (file.read(oggMagic, sizeof(oggMagic)) == sizeof(oggMagic))
        {
            if (QByteArray(oggMagic, 4) != "OggS")
            {
                report.issues.append({ValidationIssue::Warning, "Sound",
                    "OGG file does not start with expected OggS magic bytes.", "", soundPath});
            }
        }
    }

    return report;
}

// ============================================================================
// validateMasters
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateMasters(const Data& data)
{
    ValidationReport report;

    const auto& metaDataCollection = data.getMetaData();
    const QStringList contentFiles = data.getContentFiles();

    if (contentFiles.isEmpty())
    {
        report.issues.append({ValidationIssue::Info, "Master",
            "No content files loaded.", "", ""});
    }

    const Header& header = data.getReaderHeader();
    const QDir dataDirectory(data.getPaths().dataDir.absolutePath());
    for (const MasterData& master : header.masters)
    {
        if (!master.name.isEmpty() && !QFileInfo(dataDirectory, master.name).exists())
        {
            report.issues.append({ValidationIssue::Error, "Master",
                QString("Declared master is missing: %1").arg(master.name), "", master.name});
        }
    }
    // Check FormID master index range:
    // If declared masters exist, the high byte of a FormID must not exceed the declared master count.
    const int masterCount = header.masters.size();
    if (masterCount > 0)
    {
        for (const auto& typed : data.allCollectionsWithTypes())
        {
            if (!typed.collection) continue;
            for (int i = 0; i < typed.collection->count(); ++i)
            {
                quint32 formId = typed.collection->getFormId(i);
                if (formId == 0) continue;
                quint32 modIndex = (formId >> 24) & 0xFF;
                if (modIndex == 0xFE || modIndex == 0xFF) continue;
                if (modIndex > static_cast<quint32>(masterCount))
                {
                    report.issues.append({ValidationIssue::Error, "Master",
                        QString("Record '%1' (0x%2) references master index 0x%3, but only %4 master(s) are declared.")
                            .arg(typed.collection->getEditorId(i))
                            .arg(QString::number(formId, 16))
                            .arg(QString::number(modIndex, 16))
                            .arg(masterCount),
                        typed.collection->getEditorId(i), ""});
                }
            }
        }
    }

    if (contentFiles.isEmpty())
        return report;

    // Build set of available master file names (lowercase for case-insensitive comparison)
    QSet<QString> availableMasters;
    for (const auto& meta : metaDataCollection.getRecords())
    {
        availableMasters.insert(meta.get().editorId.toLower());
    }

    // Check if declared masters exist in load order
    for (int i = 0; i < contentFiles.size(); i++)
    {
        const QString& pluginName = contentFiles.at(i);

        // Try to get the TES4 header for this plugin
        int metaIdx = metaDataCollection.searchId(pluginName);
        if (metaIdx < 0)
        {
            report.issues.append({ValidationIssue::Warning, "Master",
                QString("Plugin '%1' not found in metadata collection.").arg(pluginName),
                "", pluginName});
            continue;
        }

        const MetaData& meta = metaDataCollection.getRecord(metaIdx).get();
        Q_UNUSED(meta);

        // Check for ESM/ESP ordering: ESM files should come before ESP files
        // Note: The header flags indicating ESM are in the plugin file itself,
        // but for load order validation we check the filename convention
        QString lowerName = pluginName.toLower();
        bool isEspFile = lowerName.endsWith(".esp");
        bool isEsmFile = lowerName.endsWith(".esm");

        if (isEspFile)
        {
            // Check that no ESM files come after this ESP
            for (int j = i + 1; j < contentFiles.size(); j++)
            {
                QString laterName = contentFiles.at(j).toLower();
                if (laterName.endsWith(".esm"))
                {
                    report.issues.append({ValidationIssue::Warning, "Master",
                        QString("ESM file '%1' loaded after ESP file '%2'. "
                            "ESM files should be loaded before ESP files.")
                            .arg(contentFiles.at(j), pluginName),
                        "", contentFiles.at(j)});
                }
            }
        }
    }

    // Check for circular master dependencies
    // (simplified: if any plugin lists itself as a master)
    for (const auto& pluginName : contentFiles)
    {
        QString lowerPlugin = pluginName.toLower();
        if (lowerPlugin.endsWith(".esm") && !availableMasters.contains(lowerPlugin))
        {
            report.issues.append({ValidationIssue::Error, "Master",
                QString("ESM file '%1' is not declared in any loaded plugin's master list.")
                    .arg(pluginName),
                "", pluginName});
        }
    }

    return report;
}

// ============================================================================
// validateFormIds
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateFormIds(const Data& data)
{
    ValidationReport report;

    // Map: formId -> (editorId, typeName)
    QMap<quint32, QPair<QString, QString>> formIdMap;

    auto checkCollection = [&](const auto& collection, const QString& typeName) {
        for (int i = 0; i < collection.size(); i++)
        {
            const auto& rec = collection.getRecord(i).get();
            quint32 formId = rec.formId;
            QString editorId = rec.editorId;

            if (formId == 0)
            {
                report.issues.append({ValidationIssue::Warning, "FormID",
                    QString("Record has no FormID: %1 (%2)").arg(editorId, typeName),
                    editorId, ""});
                continue;
            }

            auto it = formIdMap.find(formId);
            if (it != formIdMap.end())
            {
                report.issues.append({ValidationIssue::Error, "FormID",
                    QString("FormID conflict: 0x%1 used by both '%2' (%3) and '%4' (%5).")
                        .arg(formId, 0, 16)
                        .arg(editorId, typeName)
                        .arg(it.value().first, it.value().second),
                    editorId, ""});
            }
            else
            {
                formIdMap.insert(formId, qMakePair(editorId, typeName));
            }
        }
    };

    checkCollection(data.getNpcCollection(), "NPC");
    checkCollection(data.getWeaponCollection(), "Weapon");
    checkCollection(data.getArmorCollection(), "Armor");
    checkCollection(data.getSpellCollection(), "Spell");
    checkCollection(data.getMagicCollection(), "Magic");
    checkCollection(data.getQuestCollection(), "Quest");
    checkCollection(data.getDialCollection(), "Dialogue");
    checkCollection(data.getInfoCollection(), "Info");
    checkCollection(data.getPackCollection(), "Package");
    checkCollection(data.getTreeCollection(), "Tree");
    checkCollection(data.getAlchCollection(), "Alchemy");
    checkCollection(data.getIngrCollection(), "Ingredient");
    checkCollection(data.getContCollection(), "Container");
    checkCollection(data.getEnchCollection(), "Enchantment");
    checkCollection(data.getBookCollection(), "Book");
    checkCollection(data.getMiscCollection(), "Misc");
    checkCollection(data.getActiCollection(), "Activator");
    checkCollection(data.getStatCollection(), "Static");
    checkCollection(data.getRaceCollection(), "Race");
    checkCollection(data.getClassCollection(), "Class");
    checkCollection(data.getFactCollection(), "Faction");
    checkCollection(data.getPerkCollection(), "Perk");
    checkCollection(data.getCellCollection(), "Cell");
    checkCollection(data.getWorldspaceCollection(), "Worldspace");
    checkCollection(data.getLocationCollection(), "Location");
    checkCollection(data.getRefrCollection(), "Reference");
    checkCollection(data.getMaterialCollection(), "Material");

    return report;
}

AssetValidator::ValidationReport AssetValidator::validateReferences(const Data& data)
{
    ValidationReport report;
    QSet<quint32> known;
    for (const auto& typed : data.allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            const quint32 id = typed.collection->getFormId(i);
            if (id != 0) known.insert(id);
        }
    }

    auto check = [&known, &report](quint32 id, const QString& editorId,
        const QString& typeName, const QString& field) {
        if (id != 0 && !known.contains(id))
        {
            report.issues.append({ValidationIssue::Error, "Reference",
                QString("%1 references missing FormID 0x%2 in %3")
                    .arg(typeName, QString::number(id, 16), field),
                editorId, ""});
        }
    };

    const auto& npcs = data.getNpcCollection();
    for (int i = 0; i < npcs.size(); ++i)
    {
        const auto& record = npcs.getRecord(i).get();
        check(record.race, record.editorId, QStringLiteral("NPC"), QStringLiteral("Race"));
        check(record.class_, record.editorId, QStringLiteral("NPC"), QStringLiteral("Class"));
        check(record.faction, record.editorId, QStringLiteral("NPC"), QStringLiteral("Faction"));
        for (quint32 id : record.spells) check(id, record.editorId, QStringLiteral("NPC"), QStringLiteral("Spells"));
        for (quint32 id : record.inventoryItems) check(id, record.editorId, QStringLiteral("NPC"), QStringLiteral("Inventory"));
    }

    const auto& weapons = data.getWeaponCollection();
    for (int i = 0; i < weapons.size(); ++i)
    {
        const auto& record = weapons.getRecord(i).get();
        check(record.enchantment, record.editorId, QStringLiteral("Weapon"), QStringLiteral("Enchantment"));
    }
    const auto& spells = data.getSpellCollection();
    for (int i = 0; i < spells.size(); ++i)
    {
        const auto& record = spells.getRecord(i).get();
        for (quint32 id : record.effects) check(id, record.editorId, QStringLiteral("Spell"), QStringLiteral("Effects"));
    }
    const auto& cells = data.getCellCollection();
    for (int i = 0; i < cells.size(); ++i)
    {
        const auto& record = cells.getRecord(i).get();
        check(record.owner, record.editorId, QStringLiteral("Cell"), QStringLiteral("Owner"));
    }
    const auto& refs = data.getRefrCollection();
    for (int i = 0; i < refs.size(); ++i)
    {
        const auto& record = refs.getRecord(i).get();
        check(record.baseId, record.editorId, QStringLiteral("Reference"), QStringLiteral("Base Object"));
        check(record.owner, record.editorId, QStringLiteral("Reference"), QStringLiteral("Owner"));
    }
    const auto& dials = data.getDialCollection();
    for (int i = 0; i < dials.size(); ++i)
    {
        const auto& record = dials.getRecord(i).get();
        for (quint32 id : record.responseIds)
            check(id, record.editorId, QStringLiteral("Dialogue"), QStringLiteral("Responses"));
    }
    const auto& infos = data.getInfoCollection();
    for (int i = 0; i < infos.size(); ++i)
    {
        const auto& record = infos.getRecord(i).get();
        check(record.targetId, record.editorId, QStringLiteral("Info"), QStringLiteral("Target"));
        for (quint32 id : record.scriptIds)
            check(id, record.editorId, QStringLiteral("Info"), QStringLiteral("Scripts"));
    }

    const auto& facts = data.getFactCollection();
    for (int i = 0; i < facts.size(); ++i)
    {
        const auto& record = facts.getRecord(i).get();
        for (quint32 id : record.relations)
            check(id, record.editorId, QStringLiteral("Faction"), QStringLiteral("Relation"));
    }
    const auto& locts = data.getLocationCollection();
    for (int i = 0; i < locts.size(); ++i)
    {
        const auto& record = locts.getRecord(i).get();
        if (record.parentId != 0)
            check(record.parentId, record.editorId, QStringLiteral("Location"), QStringLiteral("Parent"));
    }
    const auto& enchs = data.getEnchCollection();
    for (int i = 0; i < enchs.size(); ++i)
    {
        const auto& record = enchs.getRecord(i).get();
        check(record.soulGem, record.editorId, QStringLiteral("Enchantment"), QStringLiteral("Soul Gem"));
    }
    const auto& packs = data.getPackCollection();
    for (int i = 0; i < packs.size(); ++i)
    {
        const auto& record = packs.getRecord(i).get();
        for (quint32 id : record.targetIds)
            check(id, record.editorId, QStringLiteral("Package"), QStringLiteral("Target"));
    }

    // Generic inspection of FormID references exposed via FormComponents
    for (const auto& typed : data.allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        BaseCollection* coll = const_cast<Data&>(data).getCollectionByType(typed.type);
        if (!coll) continue;
        const QString typeName = CkId(typed.type).getTypeName();
        for (int i = 0; i < coll->count(); ++i)
        {
            openck::FormComponents* comps = nullptr;
            void* recPtr = nullptr;
            if (resolveComponents(coll, i, comps, recPtr) && comps)
            {
                const QString editorId = coll->getEditorId(i);
                for (const auto& comp : comps->all())
                {
                    if (!comp) continue;
                    auto props = comp->createEditorProperties();
                    for (const auto& prop : props)
                    {
                        if (auto* formProp = dynamic_cast<FormEditorProperty*>(prop.get()))
                        {
                            quint32 fid = formProp->value().toUInt();
                            check(fid, editorId, typeName, QString("%1 -> %2").arg(comp->name(), formProp->name()));
                        }
                        else if (auto* arrayProp = dynamic_cast<FormArrayEditorProperty*>(prop.get()))
                        {
                            QVector<quint32> fids = arrayProp->value().value<QVector<quint32>>();
                            for (quint32 fid : fids)
                            {
                                check(fid, editorId, typeName, QString("%1 -> %2").arg(comp->name(), arrayProp->name()));
                            }
                        }
                    }
                }
            }
        }
    }

    return report;
}

// ============================================================================
// validateEditorIds
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateEditorIds(const Data& data)
{
    ValidationReport report;
    struct ExistingId {
        quint32 formId;
        QString typeName;
    };
    QMap<QString, ExistingId> editorIdMap;

    for (const auto& typed : data.allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        const QString typeName = CkId(typed.type).getTypeName();
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            const QString editorId = typed.collection->getEditorId(i);
            const quint32 formId = typed.collection->getFormId(i);

            if (editorId.trimmed().isEmpty())
                continue;

            const QString lower = editorId.toLower();
            auto it = editorIdMap.find(lower);
            if (it != editorIdMap.end())
            {
                if (it.value().formId != formId)
                {
                    report.issues.append({ValidationIssue::Error, "EditorID",
                        QString("Duplicate EditorID '%1' used by FormID 0x%2 (%3) and 0x%4 (%5)")
                            .arg(editorId,
                                 QString::number(formId, 16),
                                 typeName,
                                 QString::number(it.value().formId, 16),
                                 it.value().typeName),
                        editorId, ""});
                }
            }
            else
            {
                editorIdMap.insert(lower, {formId, typeName});
            }
        }
    }
    return report;
}

// ============================================================================
// validateRequiredComponents
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateRequiredComponents(const Data& data)
{
    ValidationReport report;

    // Placed references must have a valid base object
    const auto& refs = data.getRefrCollection();
    for (int i = 0; i < refs.size(); ++i)
    {
        const auto& rec = refs.getRecord(i).get();
        if (rec.baseId == 0)
        {
            report.issues.append({ValidationIssue::Error, "Component",
                QString("Placed reference '%1' (0x%2) is missing required base object FormID.")
                    .arg(rec.editorId, QString::number(rec.formId, 16)),
                rec.editorId, ""});
        }
    }

    // Actors (NPC) must have a race
    const auto& npcs = data.getNpcCollection();
    for (int i = 0; i < npcs.size(); ++i)
    {
        const auto& rec = npcs.getRecord(i).get();
        if (rec.race == 0)
        {
            report.issues.append({ValidationIssue::Error, "Component",
                QString("Actor '%1' (0x%2) is missing required race FormID.")
                    .arg(rec.editorId, QString::number(rec.formId, 16)),
                rec.editorId, ""});
        }
    }

    // Statics must have a 3D model
    const auto& stats = data.getStatCollection();
    for (int i = 0; i < stats.size(); ++i)
    {
        const auto& rec = stats.getRecord(i).get();
        bool hasModel = !rec.modelPath.isEmpty();
        if (!hasModel)
        {
            if (const auto* comp = static_cast<const tescomponents::TESModel_Component*>(
                    rec.components.findByName(QStringLiteral("TESModel"))))
            {
                hasModel = !comp->modelPath.isEmpty();
            }
        }
        if (!hasModel)
        {
            report.issues.append({ValidationIssue::Warning, "Component",
                QString("Static '%1' (0x%2) has no 3D model path specified.")
                    .arg(rec.editorId, QString::number(rec.formId, 16)),
                rec.editorId, ""});
        }
    }

    // Quests must have non-empty EditorID
    const auto& quests = data.getQuestCollection();
    for (int i = 0; i < quests.size(); ++i)
    {
        const auto& rec = quests.getRecord(i).get();
        if (rec.editorId.trimmed().isEmpty())
        {
            report.issues.append({ValidationIssue::Error, "Component",
                QString("Quest (0x%1) is missing a required EditorID.")
                    .arg(QString::number(rec.formId, 16)),
                "", ""});
        }
    }

    return report;
}

// ============================================================================
// validateScripts
// ============================================================================

AssetValidator::ValidationReport AssetValidator::validateScripts(const Data& data)
{
    ValidationReport report;

    QSet<quint32> knownScripts;
    const auto& scriptColl = data.getScptCollection();
    for (int i = 0; i < scriptColl.size(); ++i)
    {
        quint32 fid = scriptColl.getRecord(i).get().formId;
        if (fid != 0) knownScripts.insert(fid);
    }
    for (const auto& typed : data.allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            quint32 fid = typed.collection->getFormId(i);
            if (fid != 0) knownScripts.insert(fid);
        }
    }

    auto checkScript = [&](quint32 scriptId, const QString& editorId, const QString& typeName) {
        if (scriptId != 0 && !knownScripts.contains(scriptId))
        {
            report.issues.append({ValidationIssue::Error, "Script",
                QString("%1 '%2' references missing script FormID 0x%3")
                    .arg(typeName, editorId, QString::number(scriptId, 16)),
                editorId, ""});
        }
    };

    const auto& quests = data.getQuestCollection();
    for (int i = 0; i < quests.size(); ++i)
    {
        const auto& rec = quests.getRecord(i).get();
        for (quint32 id : rec.scriptIds)
            checkScript(id, rec.editorId, QStringLiteral("Quest"));
    }

    const auto& infos = data.getInfoCollection();
    for (int i = 0; i < infos.size(); ++i)
    {
        const auto& rec = infos.getRecord(i).get();
        for (quint32 id : rec.scriptIds)
            checkScript(id, rec.editorId, QStringLiteral("Dialogue response"));
    }


    return report;
}


AssetValidator::ValidationReport AssetValidator::validateOrphanedRecords(const Data& data)
{
    ValidationReport report;

    // Record types that are commonly orphaned (skip NPC, CELL, WRLD which are always referenced)
    // We focus on records that reference other records and check for dangling references

    // Collect all referenced formIds from records
    QSet<quint32> referencedFormIds;

    auto collectRefs = [&](const auto& collection) {
        for (int i = 0; i < collection.size(); i++)
        {
            const auto& rec = collection.getRecord(i).get();
            // This is a simplified check - each record type has different reference fields
            // For now we collect all formIds as potential references
        }
    };

    // For a more complete orphan detection, we scan common reference fields
    // Check spell references in NPCs
    const auto& npcCollection = data.getNpcCollection();
    for (int i = 0; i < npcCollection.size(); i++)
    {
        const auto& npc = npcCollection.getRecord(i).get();
        for (quint32 spellId : npc.spells)
            referencedFormIds.insert(spellId);
        for (quint32 itemId : npc.inventoryItems)
            referencedFormIds.insert(itemId);
        if (npc.race != 0) referencedFormIds.insert(npc.race);
        if (npc.faction != 0) referencedFormIds.insert(npc.faction);
    }

    // Check weapon references
    const auto& weapCollection = data.getWeaponCollection();
    for (int i = 0; i < weapCollection.size(); i++)
    {
        const auto& weapon = weapCollection.getRecord(i).get();
        if (weapon.enchantment != 0) referencedFormIds.insert(weapon.enchantment);
    }

    // Check armor references
    const auto& armorCollection = data.getArmorCollection();
    for (int i = 0; i < armorCollection.size(); i++)
    {
        const auto& armor = armorCollection.getRecord(i).get();
    }

    // Check spell references
    const auto& spellCollection = data.getSpellCollection();
    for (int i = 0; i < spellCollection.size(); i++)
    {
        const auto& spell = spellCollection.getRecord(i).get();
        for (quint32 effectId : spell.effects)
            referencedFormIds.insert(effectId);
    }

    // Check alchemy references
    const auto& alchCollection = data.getAlchCollection();
    for (int i = 0; i < alchCollection.size(); i++)
    {
        const auto& alch = alchCollection.getRecord(i).get();
    }

    // Now report records that have zero references (potentially orphaned)
    auto checkOrphan = [&](const auto& collection, CkId::Type type, const QString& typeName) {
        for (int i = 0; i < collection.size(); i++)
        {
            const auto& rec = collection.getRecord(i).get();
            quint32 formId = rec.formId;
            QString editorId = rec.editorId;

            if (formId == 0)
                continue;

            if (!referencedFormIds.contains(formId))
            {
                report.issues.append({ValidationIssue::Info, "Record",
                    QString("%1 '%2' may be orphaned (no references found).")
                        .arg(typeName, editorId),
                    editorId, ""});
            }
        }
    };

    checkOrphan(data.getSpellCollection(), CkId::Type_Spel_, "Spell");
    checkOrphan(data.getMagicCollection(), CkId::Type_Magic_, "Magic");
    checkOrphan(data.getQuestCollection(), CkId::Type_Quest_, "Quest");
    checkOrphan(data.getDialCollection(), CkId::Type_Dial_, "Dialogue");
    checkOrphan(data.getInfoCollection(), CkId::Type_Info_, "Info");
    checkOrphan(data.getPackCollection(), CkId::Type_Pack_, "Package");
    checkOrphan(data.getTreeCollection(), CkId::Type_Tree_, "Tree");
    checkOrphan(data.getIngrCollection(), CkId::Type_Ingr_, "Ingredient");
    checkOrphan(data.getContCollection(), CkId::Type_Cont_, "Container");
    checkOrphan(data.getEnchCollection(), CkId::Type_Ench_, "Enchantment");
    checkOrphan(data.getBookCollection(), CkId::Type_Book_, "Book");
    checkOrphan(data.getMiscCollection(), CkId::Type_Misc_, "Misc");
    checkOrphan(data.getActiCollection(), CkId::Type_Acti_, "Activator");
    checkOrphan(data.getStatCollection(), CkId::Type_Stat_, "Static");
    checkOrphan(data.getRaceCollection(), CkId::Type_Race_, "Race");
    checkOrphan(data.getClassCollection(), CkId::Type_Class_, "Class");
    checkOrphan(data.getFactCollection(), CkId::Type_Fact_, "Faction");
    checkOrphan(data.getPerkCollection(), CkId::Type_PerK_, "Perk");
    checkOrphan(data.getLocationCollection(), CkId::Type_LOCT_, "Location");
    checkOrphan(data.getMaterialCollection(), CkId::Type_Material_, "Material");

    return report;
}

// ============================================================================
// validateAll
// ============================================================================

namespace {

// A record path must stay inside the data tree. Checked textually, without
// touching the filesystem, because the asset may legitimately live inside an
// archive rather than loose.
bool isEscapingAssetPath(const QString& path)
{
    if (path.isEmpty())
        return false;
    QString normalised = path;
    normalised.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // Absolute, drive-qualified or UNC.
    if (normalised.startsWith(QLatin1Char('/')))
        return true;
    if (normalised.size() >= 2 && normalised.at(1) == QLatin1Char(':'))
        return true;
    // Any parent-directory hop, including one that only escapes mid-path.
    const QStringList parts = normalised.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    return parts.contains(QLatin1String(".."));
}

} // namespace

bool AssetValidator::isEscapingPath(const QString& path)
{
    return isEscapingAssetPath(path);
}

bool AssetValidator::shouldBlockSave(const ValidationReport& report)
{
    return report.errors() > 0;
}

bool AssetValidator::shouldBlockSave(const ValidationReport& report, int allowedErrors)
{
    return report.errors() > allowedErrors;
}

AssetValidator::ValidationReport AssetValidator::validateRelationships(const Data& data)
{
    ValidationReport report;

    QSet<quint32> known;
    for (const auto& typed : data.allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            const quint32 id = typed.collection->getFormId(i);
            if (id != 0) known.insert(id);
        }
    }

    auto missing = [&known, &report](quint32 id, const QString& editorId,
        const QString& typeName, const QString& what) {
        if (id != 0 && !known.contains(id))
        {
            report.issues.append({ValidationIssue::Error, "Relationship",
                QString("%1 '%2' %3 points at missing FormID 0x%4")
                    .arg(typeName, editorId, what, QString::number(id, 16)),
                editorId, ""});
        }
    };

    // A placed reference must live in a cell that exists, and its base object
    // must exist. The parent comes from the load-time index, not the record, so
    // a reference that was placed in a cell which was later removed is caught
    // here rather than silently vanishing on save.
    const auto& refs = data.getRefrCollection();
    for (int i = 0; i < refs.size(); ++i)
    {
        const auto& record = refs.getRecord(i).get();
        const quint32 parentCell = data.parentCellOfRefr(record.formId);
        if (parentCell == 0)
        {
            report.issues.append({ValidationIssue::Warning, "Relationship",
                QStringLiteral("Placed reference has no parent cell"),
                record.editorId, ""});
        }
        else
        {
            missing(parentCell, record.editorId, QStringLiteral("Placed reference"),
                QStringLiteral("parent cell"));
        }
        missing(record.baseId, record.editorId, QStringLiteral("Placed reference"),
            QStringLiteral("base object"));
    }

    const auto& cells = data.getCellCollection();
    for (int i = 0; i < cells.size(); ++i)
    {
        const auto& record = cells.getRecord(i).get();
        missing(record.owner, record.editorId, QStringLiteral("Cell"), QStringLiteral("owner"));
    }

    // Quest and dialogue links. A quest stage or objective pointing at nothing
    // produces a quest that silently never completes.
    const auto& quests = data.getQuestCollection();
    for (int i = 0; i < quests.size(); ++i)
    {
        const auto& record = quests.getRecord(i).get();
        for (quint32 id : record.stageIds)
            missing(id, record.editorId, QStringLiteral("Quest"), QStringLiteral("stage"));
        for (quint32 id : record.objectiveIds)
            missing(id, record.editorId, QStringLiteral("Quest"), QStringLiteral("objective"));
        for (quint32 id : record.aliasIds)
            missing(id, record.editorId, QStringLiteral("Quest"), QStringLiteral("alias"));
        for (quint32 id : record.scriptIds)
            missing(id, record.editorId, QStringLiteral("Quest"), QStringLiteral("script"));
    }

    const auto& dials = data.getDialCollection();
    for (int i = 0; i < dials.size(); ++i)
    {
        const auto& record = dials.getRecord(i).get();
        for (quint32 id : record.responseIds)
            missing(id, record.editorId, QStringLiteral("Dialogue topic"), QStringLiteral("response"));
    }

    const auto& infos = data.getInfoCollection();
    for (int i = 0; i < infos.size(); ++i)
    {
        const auto& record = infos.getRecord(i).get();
        missing(record.targetId, record.editorId, QStringLiteral("Dialogue response"),
            QStringLiteral("target topic"));
    }

    // Cell grid coordinates. Interior cells must not carry a grid position
    // (they are positioned by their own XCLC-less DATA), and exterior grid
    // coordinates outside the engine's +-32 cell range make the cell
    // unreachable by the renderer.
    const auto& cellColl = data.getCellCollection();
    for (int i = 0; i < cellColl.size(); ++i)
    {
        const auto& cell = cellColl.getRecord(i).get();
        const bool interior = (cell.flags & 1) != 0;
        if (interior)
        {
            if (cell.hasXclc)
            {
                report.issues.append({ValidationIssue::Warning, "Cell",
                    QStringLiteral("Interior cell carries an exterior grid "
                                   "position (XCLC); the position is ignored"),
                    cell.editorId, ""});
            }
            continue;
        }
        // XCLC stores signed coordinates; check against the engine range.
        const int gridX = static_cast<int>(static_cast<qint32>(cell.cellX));
        const int gridY = static_cast<int>(static_cast<qint32>(cell.cellY));
        if (!cell.hasXclc)
        {
            report.issues.append({ValidationIssue::Warning, "Cell",
                QStringLiteral("Exterior cell has no grid position (XCLC)"),
                cell.editorId, ""});
        }
        else if (gridX < -32 || gridX > 32 || gridY < -32 || gridY > 32)
        {
            report.issues.append({ValidationIssue::Error, "Cell",
                QString("Exterior cell grid position (%1, %2) is outside the "
                        "valid -32..32 range")
                    .arg(gridX).arg(gridY),
                cell.editorId, ""});
        }
    }

    // Circular dialogue links: topic -> response -> another topic -> ...
    // A cycle here deadlocks the conversation (the same exchange repeats
    // forever) and the Creation Kit happily saves it. Depth-first search
    // over the DIAL/INFO graph with three-colour marking finds the cycle
    // rather than looping forever on the plugin's own logic.
    {
        QHash<quint32, QVector<quint32>> edges;   // DIAL formId -> INFO ids
        const auto& dialColl = data.getDialCollection();
        for (int i = 0; i < dialColl.size(); ++i)
        {
            const auto& dial = dialColl.getRecord(i).get();
            for (quint32 infoId : dial.responseIds)
                edges[dial.formId].append(infoId);
        }
        const auto& infoColl = data.getInfoCollection();
        QHash<quint32, QVector<quint32>> infoToTopics; // INFO formId -> topic ids
        for (int i = 0; i < infoColl.size(); ++i)
        {
            const auto& info = infoColl.getRecord(i).get();
            if (info.targetId != 0)
                infoToTopics[info.formId].append(info.targetId);
        }

        // topic ids reachable from a topic: through its responses' targets.
        QHash<quint32, QVector<quint32>> topicEdges;
        for (auto it = edges.constBegin(); it != edges.constEnd(); ++it)
        {
            QVector<quint32> targets;
            for (quint32 infoId : it.value())
            {
                for (quint32 topicId : infoToTopics.value(infoId))
                    targets.append(topicId);
            }
            topicEdges[it.key()] = targets;
        }

        enum Color { White, Grey, Black };
        QHash<quint32, Color> color;
        QVector<quint32> stack;
        QVector<quint32> cyclePath;

        std::function<bool(quint32)> visit = [&](quint32 topic) -> bool {
            color[topic] = Grey;
            stack.append(topic);
            for (quint32 next : topicEdges.value(topic))
            {
                if (color.value(next, White) == Grey)
                {
                    // Closed a cycle: stack[next..] is the path.
                    const int at = stack.indexOf(next);
                    cyclePath = stack.mid(at);
                    cyclePath.append(next);
                    return true;
                }
                if (color.value(next, White) == White && visit(next))
                    return true;
            }
            stack.removeLast();
            color[topic] = Black;
            return false;
        };

        for (auto it = topicEdges.constBegin(); it != topicEdges.constEnd(); ++it)
        {
            const quint32 topic = it.key();
            if (color.value(topic, White) != White)
                continue;
            stack.clear();
            if (visit(topic))
            {
                QString path;
                for (quint32 id : cyclePath)
                    path += QString("0x%1 -> ").arg(id, 0, 16);
                path.chop(4);   // trailing arrow
                report.issues.append({ValidationIssue::Error, "Dialogue",
                    QStringLiteral("Circular dialogue link starting at %1")
                        .arg(path),
                    QString(), ""});
                break;   // one cycle is enough to fail the save gate
            }
        }
    }

    // A worldspace that claims a cell which does not exist will not render.
    const auto& worldspaces = data.getWorldspaceCollection();
    for (int i = 0; i < worldspaces.size(); ++i)
    {
        const auto& record = worldspaces.getRecord(i).get();
        for (quint32 cellId : record.cellIds)
        {
            missing(cellId, record.editorId, QStringLiteral("Worldspace"), QStringLiteral("cell"));
        }
    }

    return report;
}

AssetValidator::ValidationReport AssetValidator::validateAssetPaths(const Data& data)
{
    ValidationReport report;

    auto check = [&report](const QString& path, const QString& editorId,
        const QString& typeName, const QString& field) {
        if (isEscapingAssetPath(path))
        {
            report.issues.append({ValidationIssue::Error, "AssetPath",
                QString("%1 '%2' %3 escapes the data directory: %4")
                    .arg(typeName, editorId, field, path),
                editorId, path});
        }
    };

    const auto& stats = data.getStatCollection();
    for (int i = 0; i < stats.size(); ++i)
    {
        const auto& record = stats.getRecord(i).get();
        check(record.modelPath, record.editorId, QStringLiteral("Static"), QStringLiteral("model"));
    }

    const auto& npcs = data.getNpcCollection();
    for (int i = 0; i < npcs.size(); ++i)
    {
        const auto& record = npcs.getRecord(i).get();
        // An actor's model and icon live in components, not the record struct,
        // so they have to be read through the component set.
        if (const auto* model = static_cast<const tescomponents::TESModel_Component*>(
                record.components.findByName(QStringLiteral("TESModel"))))
        {
            check(model->modelPath, record.editorId, QStringLiteral("Actor"),
                QStringLiteral("model"));
        }
        if (const auto* texture = static_cast<const tescomponents::TESTexture_Component*>(
                record.components.findByName(QStringLiteral("TESTexture"))))
        {
            check(texture->iconPath, record.editorId, QStringLiteral("Actor"),
                QStringLiteral("icon"));
        }
    }

    const auto& weapons = data.getWeaponCollection();
    for (int i = 0; i < weapons.size(); ++i)
    {
        const auto& record = weapons.getRecord(i).get();
        check(record.modelPath, record.editorId, QStringLiteral("Weapon"), QStringLiteral("model"));
        check(record.iconPath, record.editorId, QStringLiteral("Weapon"), QStringLiteral("icon"));
    }

    const auto& sounds = data.getSounCollection();
    for (int i = 0; i < sounds.size(); ++i)
    {
        const auto& record = sounds.getRecord(i).get();
        check(record.soundFile, record.editorId, QStringLiteral("Sound"), QStringLiteral("file"));
    }

    const auto& weathers = data.getWthrCollection();
    for (int i = 0; i < weathers.size(); ++i)
    {
        const auto& record = weathers.getRecord(i).get();
        check(record.sunTexture, record.editorId, QStringLiteral("Weather"), QStringLiteral("sun texture"));
    }

    const auto& lights = data.getLighCollection();
    for (int i = 0; i < lights.size(); ++i)
    {
        const auto& record = lights.getRecord(i).get();
        check(record.modelPath, record.editorId, QStringLiteral("Light"), QStringLiteral("model"));
        check(record.iconPath, record.editorId, QStringLiteral("Light"), QStringLiteral("icon"));
    }

    return report;
}

AssetValidator::ValidationReport AssetValidator::validateAll(const Data& data, const QString& dataDir)
{
    QVector<ValidationReport> reports;

    // 1. Validate masters and load order (includes master index range)
    reports.append(validateMasters(data));

    // 2. Validate formID conflicts
    reports.append(validateFormIds(data));

    // 3. Validate duplicate EditorIDs
    reports.append(validateEditorIds(data));

    // 4. Validate references (all collections + component properties)
    reports.append(validateReferences(data));

    // 5. Validate orphaned records
    reports.append(validateOrphanedRecords(data));

    // 6. Structural relationships the per-field reference check cannot see
    reports.append(validateRelationships(data));

    // 7. Required engine components
    reports.append(validateRequiredComponents(data));

    // 8. Script references
    reports.append(validateScripts(data));

    // 9. Asset paths that escape the data tree
    reports.append(validateAssetPaths(data));

    // The remaining rules resolve assets on disk. Without a usable data
    // directory AssetResolver scans from an invalid root, which walks an
    // enormous tree for minutes and then overruns the stack, so stop here and
    // report the data-side rules only.
    if (dataDir.isEmpty() || !QFileInfo(dataDir).isDir())
    {
        ValidationReport r;
        r.issues.append({ValidationIssue::Warning, "Validation",
            QStringLiteral("No data directory available; asset existence and "
                "content checks were skipped"),
            QString(), dataDir});
        reports.append(r);
        return mergeReports(reports);
    }

    // 7. Validate NIF files referenced by stat records
    AssetResolver resolver(dataDir);
    const auto& statCollection = data.getStatCollection();
    for (int i = 0; i < statCollection.size(); i++)
    {
        const auto& stat = statCollection.getRecord(i).get();
        if (stat.modelPath.isEmpty())
            continue;

        // The asset may live loose or inside a BSA/BA2 archive.
        if (!resolver.contains(stat.modelPath))
        {
            ValidationReport r;
            r.issues.append({ValidationIssue::Error, "NIF",
                QString("Referenced NIF does not exist (loose or in any archive): %1")
                    .arg(stat.modelPath),
                stat.editorId, stat.modelPath});
            reports.append(r);
            continue;
        }

        // Content validation needs a real file; only possible for loose assets.
        const QString loose = resolver.absoluteLoosePath(stat.modelPath, dataDir);
        if (!loose.isEmpty())
            reports.append(validateNif(loose));
    }

    // 5. Validate textures referenced by stat records (simplified - check for texture paths)
    const auto& alchCollection = data.getAlchCollection();
    Q_UNUSED(alchCollection);

    // Note: Full texture/sound validation requires scanning data directories
    // for all referenced assets. This is a targeted validation based on
    // records currently loaded.

    return mergeReports(reports);
}
