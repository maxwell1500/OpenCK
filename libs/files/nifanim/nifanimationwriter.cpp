#include "nifanimationwriter.hpp"
#include "nifparser.hpp"
#include "nifblockfile.hpp"

#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QSet>

#include "logger.hpp"

namespace {

// Patch the keyframe data block a controller drives, rejecting refs that do
// not index a block we know how to re-encode.
bool patchControllerData(NifBlockFile& file, int controllerIndex,
                         const QVector<Nif::TransformKeyframe>& keyframes)
{
    const int dataIndex = file.keyframeDataBlockFor(controllerIndex);
    if (dataIndex < 0) {
        LOG_WARNING(QString("NifAnimationWriter: controller block %1 has no usable "
                            "keyframe data block").arg(controllerIndex));
        return false;
    }

    const QString dataType = file.block(dataIndex).type;
    const QByteArray& original = file.block(dataIndex).data;

    // Only touch layouts whose byte encoding this build has confirmed.
    if (!NifBlockFile::isWritableKeyframeType(dataType)) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') uses an "
                            "unconfirmed layout; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }

    // Precondition: our codec must reproduce the untouched block exactly.
    // If it cannot, this build does not actually understand this game's
    // keyframe layout, and rewriting it would corrupt the NIF. Refuse.
    QVector<Nif::TransformKeyframe> decoded;
    if (!NifBlockFile::decodeKeyframeData(dataType, original, decoded)) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') is in an "
                            "unrecognised layout; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }
    QByteArray reencoded;
    if (!NifBlockFile::encodeKeyframeData(dataType, decoded, reencoded)
        || reencoded != original) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') does not "
                            "round-trip; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }

    QByteArray encoded;
    if (!NifBlockFile::encodeKeyframeData(dataType, keyframes, encoded)) {
        LOG_WARNING(QString("NifAnimationWriter: unsupported keyframe data block '%1'")
                        .arg(dataType));
        return false;
    }

    // The encoder must also reproduce the requested frames.
    QVector<Nif::TransformKeyframe> verify;
    if (!NifBlockFile::decodeKeyframeData(dataType, encoded, verify)
        || verify.size() != keyframes.size()) {
        LOG_ERROR("NifAnimationWriter: keyframe encoder failed its own round-trip check");
        return false;
    }

    file.setBlockData(dataIndex, encoded);
    return true;
}

// Patch every node whose name matches, optionally restricted to the
// controllers a named clip sequence points at.
bool patchBethesdaNif(const QString& nifPath, const QString& nodeName,
                      const QVector<Nif::TransformKeyframe>& keyframes,
                      const QString& clipName, int& channelsPatched)
{
    NifBlockFile file;
    if (!file.load(nifPath)) return false;

    // Clip names only exist in a controller sequence, so resolve the set of
    // controller refs the requested clip owns up front.
    QSet<quint32> clipControllers;
    if (!clipName.isEmpty()) {
        bool foundSequence = false;
        const QList<int> sequences = file.findBlocks(QStringLiteral("NiControllerSequence"));
        for (int sequenceIndex : sequences) {
            QList<QPair<quint32, QString>> entries;
            if (!NifBlockFile::decodeControllerSequence(file.block(sequenceIndex).data, entries))
                continue;
            for (const auto& entry : entries) {
                if (entry.second == clipName) {
                    clipControllers.insert(entry.first);
                    foundSequence = true;
                }
            }
        }
        if (!foundSequence) {
            LOG_WARNING(QString("NifAnimationWriter: NIF has no clip named '%1'").arg(clipName));
            return false;
        }
    }

    // Nodes point at their own controller. Going that direction avoids
    // depending on the controller's field layout, which differs between the
    // 1.5 (NiTransformController) and 1.6+ (NiKeyframeController) formats.
    QSet<quint32> wantedControllers;
    wantedControllers.reserve(8);
    for (const QString& type : {QStringLiteral("NiTransformController"),
                                QStringLiteral("NiKeyframeController")}) {
        const QList<int> indices = file.findBlocks(type);
        for (int index : indices)
            wantedControllers.insert(static_cast<quint32>(index));
    }
    if (wantedControllers.isEmpty()) {
        LOG_WARNING("NifAnimationWriter: NIF has no keyframe controller blocks");
        return false;
    }

    bool matchedNode = false;
    for (int block = 0; block < file.count(); ++block) {
        QString name;
        quint32 controllerRef = 0xFFFFFFFFu;
        if (!file.nodeNetInfo(block, name, controllerRef)) continue;
        if (name != nodeName) continue;
        if (!wantedControllers.contains(controllerRef)) continue;
        if (!clipControllers.isEmpty() && !clipControllers.contains(controllerRef)) continue;

        matchedNode = true;
        if (patchControllerData(file, static_cast<int>(controllerRef), keyframes))
            ++channelsPatched;
    }

    if (!matchedNode) {
        LOG_WARNING(QString("NifAnimationWriter: target node '%1' not found in %2")
                        .arg(nodeName, nifPath));
        return false;
    }
    if (channelsPatched == 0) return false;
    return file.save(nifPath);
}

} // namespace

bool NifAnimationWriter::writeKeyframesToNif(const QString& nifPath,
                                             const QString& nodeName,
                                             const QVector<Nif::TransformKeyframe>& keyframes,
                                             const QString& clipName)
{
    if (keyframes.isEmpty()) {
        LOG_ERROR("NifAnimationWriter: refusing to write an empty keyframe list");
        return false;
    }

    if (NifBlockFile::isBethesdaNif(nifPath)) {
        int patched = 0;
        if (!patchBethesdaNif(nifPath, nodeName, keyframes, clipName, patched)) return false;
        LOG_INFO(QString("NifAnimationWriter: patched %1 controller(s) in %2")
                     .arg(patched).arg(nifPath));
        return true;
    }

    // Internal dialect: re-parse the source so the write keeps the rest of
    // the tree intact.
    Nif::NifParser parser;
    if (!parser.load(nifPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to parse NIF for write-back: %1").arg(nifPath));
        return false;
    }

    Nif::Node* root = parser.getRoot();
    if (!root) {
        LOG_ERROR("NifAnimationWriter: NIF has no root node");
        return false;
    }

    bool found = false;
    auto updateNode = [&](auto* node, auto&& self) -> bool {
        if (!node) return false;

        bool nodeMatches = node->name == nodeName;
        for (auto& anim : node->animations) {
            if (QString::number(anim.targetNode) == nodeName) nodeMatches = true;
        }
        if (nodeMatches) {
            for (auto& anim : node->animations) {
                if (!clipName.isEmpty() && anim.clipName != clipName) continue;
                anim.keyframes = keyframes;
                found = true;
                LOG_INFO(QString("NifAnimationWriter: updated %1 keyframes for node %2")
                             .arg(keyframes.size())
                             .arg(nodeName));
                return true;
            }
        }

        for (auto* child : node->children) {
            if (self(child, self)) return true;
        }
        return false;
    };

    updateNode(root, updateNode);

    if (!found) {
        LOG_WARNING(QString("NifAnimationWriter: target node '%1' not found in NIF").arg(nodeName));
        return false;
    }

    const QString tempPath = nifPath + QStringLiteral(".openck.tmp");
    QFile::remove(tempPath);
    if (!parser.save(tempPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to save NIF to temp file: %1").arg(tempPath));
        QFile::remove(tempPath);
        return false;
    }

    QFile source(tempPath);
    if (!source.open(QIODevice::ReadOnly)) {
        QFile::remove(tempPath);
        return false;
    }
    QSaveFile output(nifPath);
    if (!output.open(QIODevice::WriteOnly)) {
        source.close();
        QFile::remove(tempPath);
        return false;
    }
    const QByteArray bytes = source.readAll();
    source.close();
    const bool written = output.write(bytes) == bytes.size() && output.commit();
    QFile::remove(tempPath);
    if (!written) {
        LOG_ERROR(QString("NifAnimationWriter: failed to atomically replace NIF: %1").arg(nifPath));
        return false;
    }

    LOG_INFO(QString("NifAnimationWriter: successfully wrote keyframes to NIF: %1").arg(nifPath));
    return true;
}
