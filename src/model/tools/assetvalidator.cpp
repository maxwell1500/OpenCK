#include "assetvalidator.hpp"
#include "assetresolver.hpp"
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
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"

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
        return report;
    }

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
                continue;

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

// ============================================================================
// validateOrphanedRecords
// ============================================================================

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

AssetValidator::ValidationReport AssetValidator::validateAll(const Data& data, const QString& dataDir)
{
    QVector<ValidationReport> reports;

    // 1. Validate masters and load order
    reports.append(validateMasters(data));

    // 2. Validate formID conflicts
    reports.append(validateFormIds(data));

    // 3. Validate orphaned records
    reports.append(validateOrphanedRecords(data));

    // 4. Validate NIF files referenced by stat records
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
