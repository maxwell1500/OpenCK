#include "nifanimation.hpp"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDebug>

#include "../log/logger.hpp"

namespace {

constexpr const char* NI_KEYFRAME_ANIM = "NiKeyFrameAnimation";

} // namespace

int NifAnimation::totalKeyframeCount() const
{
    int count = 0;
    for (const auto& clip : clips) {
        for (const auto& ch : clip.channels) {
            count += ch.keyframes.size();
        }
    }
    return count;
}

NifAnimation* NifAnimationParser::parse(const QString& nifPath)
{
    QFile file(nifPath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open NIF for animation parsing: %1").arg(nifPath));
        return nullptr;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 12) return nullptr;

    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    stream >> magic;
    if (magic != 0x46494E4E) return nullptr;

    quint32 version = 0;
    stream >> version;

    quint32 fileNameLen = 0;
    stream >> fileNameLen;
    if (fileNameLen > 0 && fileNameLen < 1024) {
        QByteArray fnBytes(fileNameLen, 0);
        stream.readRawData(fnBytes.data(), fileNameLen);
    }

    quint32 numShapes = 0;
    stream >> numShapes;

    // Skip shape data
    for (quint32 s = 0; s < numShapes; ++s) {
        quint32 nameLen = 0;
        stream >> nameLen;
        if (nameLen > 1024) break;
        QByteArray nameBytes(nameLen, 0);
        stream.readRawData(nameBytes.data(), nameLen);

        quint32 numVertices = 0;
        stream >> numVertices;
        for (quint32 v = 0; v < numVertices; ++v) {
            float fx, fy, fz, fu, fv, fr, fg, fb, fa;
            stream >> fx >> fy >> fz >> fu >> fv >> fr >> fg >> fb >> fa;
        }

        quint32 numIndices = 0;
        stream >> numIndices;
        for (quint32 i = 0; i < numIndices; ++i) {
            quint32 idx = 0;
            stream >> idx;
        }

        float cr, cg, cb, ca;
        stream >> cr >> cg >> cb >> ca;
        quint32 texLen = 0;
        stream >> texLen;
        if (texLen > 0 && texLen < 4096) {
            QByteArray texBytes(texLen, 0);
            stream.readRawData(texBytes.data(), texLen);
        }
    }

    // Scan for animation data by looking for NiKeyFrameAnimation class name
    QByteArray searchPattern = "NiKeyFrameAnimation";
    int pos = data.indexOf(searchPattern);

    NifAnimation* anim = new NifAnimation();
    anim->name = QFileInfo(nifPath).baseName();

    if (pos >= 0) {
        LOG_INFO(QString("Found animation data in NIF at offset 0x%1")
                     .arg(pos, 0, 16));

        QDataStream animStream(&data, QIODevice::ReadOnly);
        animStream.setByteOrder(QDataStream::LittleEndian);
        animStream.skipRawData(pos + searchPattern.size());

        AnimClip clip;
        clip.name = anim->name;

        // Skip className length and name (already past it)
        // Skip dataRef
        quint32 dataRef = 0;
        animStream >> dataRef;

        // Read number of channels
        quint32 numChannels = 0;
        if (animStream.atEnd()) {
            delete anim;
            return nullptr;
        }
        animStream >> numChannels;

        for (quint32 c = 0; c < numChannels && !animStream.atEnd(); ++c) {
            AnimChannel ch;

            quint32 nameLen = 0;
            animStream >> nameLen;
            if (nameLen > 256) break;
            QByteArray nameBytes(nameLen, 0);
            animStream.readRawData(nameBytes.data(), nameBytes.size());
            ch.boneName = QString::fromLatin1(nameBytes);

            quint32 typeLen = 0;
            animStream >> typeLen;
            if (typeLen > 64) break;
            QByteArray typeBytes(typeLen, 0);
            animStream.readRawData(typeBytes.data(), typeBytes.size());
            ch.type = QString::fromLatin1(typeBytes);

            // Read keyframe count
            quint32 numKeys = 0;
            animStream >> numKeys;

            for (quint32 k = 0; k < numKeys && !animStream.atEnd(); ++k) {
                AnimKeyframe kf;
                animStream >> kf.time;
                if (animStream.atEnd()) break;
                animStream >> kf.tx >> kf.ty >> kf.tz;
                if (animStream.atEnd()) break;
                animStream >> kf.rx >> kf.ry >> kf.rz;
                if (animStream.atEnd()) break;
                // Skip scale key (3 floats)
                float sx, sy, sz;
                animStream >> sx >> sy >> sz;
                ch.keyframes.append(kf);
            }

            if (!ch.keyframes.isEmpty()) {
                ch.duration = ch.keyframes.last().time;
                clip.channels.append(ch);
            }
        }

        if (!clip.channels.isEmpty()) {
            anim->clips.append(clip);
            LOG_INFO(QString("Parsed animation: %1 (%2 channels, %3 keyframes)")
                         .arg(anim->name)
                         .arg(clip.channels.size())
                         .arg(clip.channels[0].keyframes.size()));
        }
    }

    if (anim->clips.isEmpty()) {
        delete anim;
        return nullptr;
    }

    return anim;
}
