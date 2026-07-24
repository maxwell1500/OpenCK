#include "assetconverter.hpp"

#include "logger.hpp"
#include "nifparser.hpp"
#include "ddsencoder.hpp"
#include "tgaencoder.hpp"
#include "oggencoder.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QImage>
#include <QDataStream>
#include <cstring>

// ============================================================================
// nifToObj
// ============================================================================

AssetConverter::ConversionResult AssetConverter::nifToObj(const QString& nifPath,
                                                         const QString& objPath)
{
    ConversionResult result;

    Nif::NifParser parser;
    if (!parser.load(nifPath))
    {
        result.error = QString("Failed to load NIF file: %1").arg(nifPath);
        LOG_ERROR(result.error);
        return result;
    }

    Nif::Node* root = parser.getRoot();
    if (!root)
    {
        result.error = "NIF file contains no root node";
        LOG_ERROR(result.error);
        return result;
    }

    QFile file(objPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        result.error = QString("Cannot open output file: %1").arg(objPath);
        LOG_ERROR(result.error);
        return result;
    }

    QTextStream out(&file);
    out << "# OpenCK OBJ Export\n";
    out << "mtllib material.mtl\n\n";

    // Global vertex/UV/normal counters across all shapes
    int globalV = 0;
    int globalVN = 0;
    int globalVT = 0;

    // Collect all shapes from root and children
    QVector<const Nif::TriShape*> allShapes;
    for (const auto& shape : root->shapes)
        allShapes.append(&shape);

    for (const auto* child : root->children)
    {
        for (const auto& shape : child->shapes)
            allShapes.append(&shape);
    }

    for (const auto* shape : allShapes)
    {
        out << "o " << (shape->name.isEmpty() ? "Mesh" : shape->name) << "\n";

        // Write vertices
        for (const auto& v : shape->vertices)
        {
            out << QString("v %1 %2 %3\n").arg(v.x, 0, 'f', 6)
                                           .arg(v.y, 0, 'f', 6)
                                           .arg(v.z, 0, 'f', 6);
        }

        // Write normals
        for (const auto& n : shape->normals)
        {
            out << QString("vn %1 %2 %3\n").arg(n.x, 0, 'f', 6)
                                            .arg(n.y, 0, 'f', 6)
                                            .arg(n.z, 0, 'f', 6);
        }

        // Write UVs
        for (const auto& uv : shape->uvs)
        {
            out << QString("vt %1 %2\n").arg(uv.u, 0, 'f', 6)
                                         .arg(uv.v, 0, 'f', 6);
        }

        // Write faces (triangulated, indices are 1-based in OBJ)
        for (int i = 0; i + 2 < shape->indices.size(); i += 3)
        {
            const int i0 = static_cast<int>(shape->indices[i]) + 1 + globalV;
            const int i1 = static_cast<int>(shape->indices[i + 1]) + 1 + globalV;
            const int i2 = static_cast<int>(shape->indices[i + 2]) + 1 + globalV;

            const int n0 = static_cast<int>(shape->indices[i]) + 1 + globalVN;
            const int n1 = static_cast<int>(shape->indices[i + 1]) + 1 + globalVN;
            const int n2 = static_cast<int>(shape->indices[i + 2]) + 1 + globalVN;

            const int t0 = static_cast<int>(shape->indices[i]) + 1 + globalVT;
            const int t1 = static_cast<int>(shape->indices[i + 1]) + 1 + globalVT;
            const int t2 = static_cast<int>(shape->indices[i + 2]) + 1 + globalVT;

            out << QString("f %1/%2/%3 %4/%5/%6 %7/%8/%9\n")
                       .arg(i0).arg(t0).arg(n0)
                       .arg(i1).arg(t1).arg(n1)
                       .arg(i2).arg(t2).arg(n2);
        }

        out << "\n";

        globalV  += shape->vertices.size();
        globalVN += shape->normals.size();
        globalVT += shape->uvs.size();
    }

    file.close();

    result.success = true;
    result.filesConverted = 1;
    LOG_INFO(QString("Exported NIF to OBJ: %1 (%2 shapes)")
                 .arg(objPath).arg(allShapes.size()));
    return result;
}

// ============================================================================
// objToNif
// ============================================================================

AssetConverter::ConversionResult AssetConverter::objToNif(const QString& objPath,
                                                         const QString& nifPath)
{
    ConversionResult result;

    QFile file(objPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.error = QString("Cannot open OBJ file: %1").arg(objPath);
        LOG_ERROR(result.error);
        return result;
    }

    QVector<Nif::Vector3> positions;
    QVector<Nif::Vector3> normals;
    QVector<Nif::Vector2> texcoords;
    QVector<unsigned int> indices;

    // For OBJ, indices are shared across v/vt/vn but we need per-vertex data for NIF.
    // We'll build a flat list of unique (pos, uv, normal) combinations.
    struct ObjVertex { int pi; int ti; int ni; };
    QVector<ObjVertex> faceVertices;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        if (parts[0] == "v" && parts.size() >= 4)
        {
            Nif::Vector3 v;
            v.x = parts[1].toFloat();
            v.y = parts[2].toFloat();
            v.z = parts[3].toFloat();
            positions.append(v);
        }
        else if (parts[0] == "vn" && parts.size() >= 4)
        {
            Nif::Vector3 n;
            n.x = parts[1].toFloat();
            n.y = parts[2].toFloat();
            n.z = parts[3].toFloat();
            normals.append(n);
        }
        else if (parts[0] == "vt" && parts.size() >= 3)
        {
            Nif::Vector2 uv;
            uv.u = parts[1].toFloat();
            uv.v = parts[2].toFloat();
            texcoords.append(uv);
        }
        else if (parts[0] == "f")
        {
            // Parse face indices (v/vt/vn format)
            QVector<ObjVertex> faceVerts;
            for (int i = 1; i < parts.size(); ++i)
            {
                QStringList idx = parts[i].split('/');
                ObjVertex fv = {0, 0, 0};
                if (idx.size() >= 1) fv.pi = idx[0].toInt() - 1;
                if (idx.size() >= 2 && !idx[1].isEmpty()) fv.ti = idx[1].toInt() - 1;
                if (idx.size() >= 3) fv.ni = idx[2].toInt() - 1;
                faceVerts.append(fv);
            }

            // Triangulate face (fan triangulation for n-gons)
            for (int i = 1; i + 1 < faceVerts.size(); ++i)
            {
                faceVertices.append(faceVerts[0]);
                faceVertices.append(faceVerts[i]);
                faceVertices.append(faceVerts[i + 1]);
            }
        }
    }
    file.close();

    if (positions.isEmpty())
    {
        result.error = "OBJ file contains no vertices";
        LOG_ERROR(result.error);
        return result;
    }

    // Build flat NIF arrays from unique face vertices
    QMap<quint64, int> vertRemap;
    QVector<Nif::Vector3> nifVerts;
    QVector<Nif::Vector2> nifUVs;
    QVector<Nif::Vector3> nifNorms;

    for (const auto& fv : faceVertices)
    {
        quint64 key = (static_cast<quint64>(fv.pi) << 32) |
                      (static_cast<quint64>(fv.ti) << 16) |
                      static_cast<quint64>(fv.ni);
        auto it = vertRemap.find(key);
        if (it == vertRemap.end())
        {
            int idx = nifVerts.size();
            vertRemap[key] = idx;

            if (fv.pi >= 0 && fv.pi < positions.size())
                nifVerts.append(positions[fv.pi]);
            else
                nifVerts.append({0, 0, 0});

            if (fv.ti >= 0 && fv.ti < texcoords.size())
                nifUVs.append(texcoords[fv.ti]);
            else
                nifUVs.append({0, 0});

            if (fv.ni >= 0 && fv.ni < normals.size())
                nifNorms.append(normals[fv.ni]);
            else
                nifNorms.append({0, 1, 0});

            indices.append(static_cast<unsigned int>(idx));
        }
        else
        {
            indices.append(static_cast<unsigned int>(it.value()));
        }
    }

    // Build NIF via NifParser
    Nif::NifParser parser;
    Nif::Node* root = new Nif::Node();
    root->name = QFileInfo(objPath).baseName();

    Nif::TriShape shape;
    shape.name = root->name;
    shape.vertices = nifVerts;
    shape.uvs = nifUVs;
    shape.normals = nifNorms;
    shape.indices = indices;
    shape.colors.resize(nifVerts.size(), Nif::Color4{1.0f, 1.0f, 1.0f, 1.0f});

    root->shapes.append(shape);
    parser.setRoot(root);

    if (!parser.save(nifPath))
    {
        result.error = QString("Failed to save NIF file: %1").arg(nifPath);
        LOG_ERROR(result.error);
        delete root;
        return result;
    }

    result.success = true;
    result.filesConverted = 1;
    LOG_INFO(QString("Imported OBJ to NIF: %1 (%2 vertices, %3 faces)")
                 .arg(nifPath).arg(nifVerts.size()).arg(indices.size() / 3));
    return result;
}

// ============================================================================
// convertTextures
// ============================================================================

static bool convertSingleTexture(const QString& inputPath, const QString& outputPath,
                                 const QString& targetFormat)
{
    QFileInfo inInfo(inputPath);
    QString srcSuffix = inInfo.suffix().toLower();

    // Load source image
    QImage image;
    if (srcSuffix == "dds")
    {
        // DDS needs to be loaded via QImage (Qt6 has native DDS support)
        if (!image.load(inputPath, "DDS"))
        {
            LOG_ERROR(QString("Failed to load DDS texture: %1").arg(inputPath));
            return false;
        }
    }
    else if (srcSuffix == "tga")
    {
        if (!image.load(inputPath, "TGA"))
        {
            LOG_ERROR(QString("Failed to load TGA texture: %1").arg(inputPath));
            return false;
        }
    }
    else if (srcSuffix == "png")
    {
        if (!image.load(inputPath, "PNG"))
        {
            LOG_ERROR(QString("Failed to load PNG texture: %1").arg(inputPath));
            return false;
        }
    }
    else
    {
        LOG_ERROR(QString("Unsupported source texture format: .%1").arg(srcSuffix));
        return false;
    }

    if (image.isNull())
    {
        LOG_ERROR(QString("Loaded image is null: %1").arg(inputPath));
        return false;
    }

    // Convert to target format
    if (targetFormat == "png")
    {
        if (!image.save(outputPath, "PNG"))
        {
            LOG_ERROR(QString("Failed to save PNG: %1").arg(outputPath));
            return false;
        }
    }
    else if (targetFormat == "tga")
    {
        // Use TgaEncoder for TGA output
        if (!TgaEncoder::encode(image, outputPath))
        {
            LOG_ERROR(QString("Failed to save TGA: %1").arg(outputPath));
            return false;
        }
    }
    else if (targetFormat == "dds")
    {
        // Use DdsEncoder for DDS output (DXT5 by default)
        if (!DdsEncoder::encode(image, outputPath, 3))
        {
            LOG_ERROR(QString("Failed to save DDS: %1").arg(outputPath));
            return false;
        }
    }
    else
    {
        LOG_ERROR(QString("Unsupported target texture format: %1").arg(targetFormat));
        return false;
    }

    return true;
}

AssetConverter::ConversionResult AssetConverter::convertTextures(const QStringList& inputPaths,
                                                               const QString& outputDir,
                                                               const QString& targetFormat)
{
    ConversionResult result;

    if (targetFormat != "dds" && targetFormat != "tga" && targetFormat != "png")
    {
        result.error = QString("Unsupported target format: %1 (use dds, tga, or png)").arg(targetFormat);
        LOG_ERROR(result.error);
        return result;
    }

    QDir outDir(outputDir);
    if (!outDir.exists())
    {
        outDir.mkpath(".");
    }

    for (const auto& inputPath : inputPaths)
    {
        QFileInfo info(inputPath);
        if (!info.exists())
        {
            LOG_WARNING(QString("Texture file does not exist, skipping: %1").arg(inputPath));
            continue;
        }

        QString outName = info.completeBaseName() + "." + targetFormat;
        QString outputPath = outDir.absoluteFilePath(outName);

        if (convertSingleTexture(inputPath, outputPath, targetFormat))
        {
            result.filesConverted++;
        }
    }

    result.success = (result.filesConverted > 0);
    LOG_INFO(QString("Texture conversion: %1 of %2 files converted to %3")
                 .arg(result.filesConverted).arg(inputPaths.size()).arg(targetFormat));
    return result;
}

// ============================================================================
// convertSounds (WAV -> OGG)
// ============================================================================

AssetConverter::ConversionResult AssetConverter::convertSounds(const QStringList& inputPaths,
                                                             const QString& outputDir)
{
    ConversionResult result;

    QDir outDir(outputDir);
    if (!outDir.exists())
    {
        outDir.mkpath(".");
    }

    for (const auto& inputPath : inputPaths)
    {
        QFileInfo info(inputPath);
        if (!info.exists())
        {
            LOG_WARNING(QString("Sound file does not exist, skipping: %1").arg(inputPath));
            continue;
        }

        QString suffix = info.suffix().toLower();
        if (suffix != "wav")
        {
            LOG_WARNING(QString("Skipping non-WAV file: %1").arg(inputPath));
            continue;
        }

        // Read WAV header to verify format
        QFile wavFile(inputPath);
        if (!wavFile.open(QIODevice::ReadOnly))
        {
            LOG_ERROR(QString("Cannot open WAV file: %1").arg(inputPath));
            continue;
        }

        char riffHeader[12] = {};
        if (wavFile.read(riffHeader, sizeof(riffHeader)) != sizeof(riffHeader))
        {
            LOG_ERROR(QString("WAV file too small: %1").arg(inputPath));
            continue;
        }

        if (QByteArray(riffHeader, 4) != "RIFF" || QByteArray(riffHeader + 8, 4) != "WAVE")
        {
            LOG_ERROR(QString("Invalid WAV file (not RIFF/WAVE): %1").arg(inputPath));
            continue;
        }

        // Read fmt chunk for format info
        quint16 audioFormat = 0;
        quint16 numChannels = 0;
        quint32 sampleRate = 0;
        quint16 bitsPerSample = 0;

        char chunkHeader[8] = {};
        while (wavFile.read(chunkHeader, 8) == 8)
        {
            QByteArray chunkId(chunkHeader, 4);
            quint32 chunkSize = 0;
            memcpy(&chunkSize, chunkHeader + 4, 4);

            if (chunkId == "fmt ")
            {
                if (chunkSize < 16) {
                    qWarning() << "WAV fmt chunk too small:" << chunkSize;
                    break;
                }
                wavFile.read(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));
                wavFile.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));
                wavFile.read(reinterpret_cast<char*>(&sampleRate), sizeof(sampleRate));
                quint32 byteRate = 0;
                wavFile.read(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));
                quint16 blockAlign = 0;
                wavFile.read(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));
                wavFile.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));
                break;
            }
            else
            {
                wavFile.seek(wavFile.pos() + chunkSize);
                if (chunkSize % 2 != 0)
                    wavFile.seek(wavFile.pos() + 1);
            }
        }
        wavFile.close();

        LOG_INFO(QString("WAV format: %1 Hz, %2 bit, %3 ch, format=%4")
                     .arg(sampleRate).arg(bitsPerSample).arg(numChannels).arg(audioFormat));

        QString outName = info.completeBaseName() + ".ogg";
        QString outputPath = outDir.absoluteFilePath(outName);

        // Read audio data chunk
        wavFile.seek(0);
        QVector<float> samples;
        quint32 dataSize = 0;
        
        char readHeader[12] = {};
        if (wavFile.read(readHeader, sizeof(readHeader)) != sizeof(readHeader))
        {
            LOG_ERROR(QString("WAV file too small to read data: %1").arg(inputPath));
            continue;
        }

        // Find data chunk
        while (wavFile.read(chunkHeader, 8) == 8)
        {
            QByteArray chunkId(chunkHeader, 4);
            memcpy(&dataSize, chunkHeader + 4, 4);
            
            if (chunkId == "data")
            {
                QByteArray rawData = wavFile.read(dataSize);
                if (rawData.isEmpty())
                {
                    LOG_ERROR(QString("Failed to read WAV data chunk: %1").arg(inputPath));
                    break;
                }

                // Convert samples to float based on bit depth
                if (bitsPerSample == 8)
                {
                    for (int i = 0; i < rawData.size(); ++i)
                    {
                        samples.append((static_cast<quint8>(rawData[i]) - 128) / 128.0f);
                    }
                }
                else if (bitsPerSample == 16)
                {
                    for (int i = 0; i < rawData.size(); i += 2)
                    {
                        qint16 sample = static_cast<qint16>(static_cast<quint16>(static_cast<uchar>(rawData[i])) | 
                                                           (static_cast<quint16>(static_cast<uchar>(rawData[i+1])) << 8));
                        samples.append(sample / 32768.0f);
                    }
                }
                else if (bitsPerSample == 32)
                {
                    for (int i = 0; i < rawData.size(); i += 4)
                    {
                        float sample;
                        memcpy(&sample, rawData.constData() + i, sizeof(float));
                        samples.append(sample);
                    }
                }
                else
                {
                    LOG_ERROR(QString("Unsupported WAV bit depth: %1 bits").arg(bitsPerSample));
                    break;
                }
                break;
            }
            else
            {
                wavFile.seek(wavFile.pos() + dataSize);
                if (dataSize % 2 != 0)
                    wavFile.seek(wavFile.pos() + 1);
            }
        }

        if (samples.isEmpty())
        {
            LOG_ERROR(QString("No audio samples found in WAV file: %1").arg(inputPath));
            continue;
        }

        // Encode to OGG
        if (!OggEncoder::encode(samples, outputPath, sampleRate, numChannels, 3))
        {
            LOG_ERROR(QString("Failed to encode OGG: %1").arg(outputPath));
            continue;
        }

        result.filesConverted++;
    }

    result.success = (result.filesConverted > 0);
    if (result.filesConverted > 0)
    {
        result.error = QString("Successfully converted %1 WAV file(s) to OGG").arg(result.filesConverted);
    }

    LOG_INFO(QString("Sound conversion: %1 of %2 files converted to OGG")
                 .arg(result.filesConverted).arg(inputPaths.size()));
    return result;
}
