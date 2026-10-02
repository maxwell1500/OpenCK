#include "nifanimationwriter.hpp"
#include "nifparser.hpp"
#include "nifblockfile.hpp"

#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QSet>

#include "logger.hpp"

#include <QtMath>

namespace {

// Convert a flat TransformKeyframe list into the channel-preserving raw
// representation, keeping each channel's own key count and time base.
//
// A NiTransformData block's channels are independent: translation, scale and
// each rotation axis carry their own counts, and on shipped meshes they
// routinely differ. Resampling them all onto the frame list's length would
// silently change the animation - a block whose translation channel holds 20
// keys would gain 21 - so each channel keeps the count it had and takes its
// values from the frame list sampled at that channel's own times. This is
// lossy: the flat list carries no interpolation or tangent data, so every
// channel becomes linear.
NifBlockFile::NiTransformDataRaw flatToRaw(
    const QVector<Nif::TransformKeyframe>& keyframes,
    const NifBlockFile::NiTransformDataRaw& original)
{
    NifBlockFile::NiTransformDataRaw raw;
    if (keyframes.isEmpty()) return raw;

    // Sample the edited animation at `time` by nearest earlier key, which for a
    // monotonic time base is a step-free resample.
    auto sampleAt = [&keyframes](float time) -> const Nif::TransformKeyframe& {
        int best = 0;
        for (int i = 1; i < keyframes.size(); ++i) {
            if (keyframes.at(i).time <= time) best = i;
            else break;
        }
        return keyframes.at(best);
    };

    auto quaternionToEuler = [](float w, float x, float y, float z, float* euler) {
        euler[0] = qAtan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
        const float sinp = 2.0f * (w * y - z * x);
        euler[1] = (qAbs(sinp) >= 1.0f)
                       ? ((sinp >= 0.0f) ? (M_PI / 2.0f) : -(M_PI / 2.0f))
                       : qAsin(sinp);
        euler[2] = qAtan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
    };

    const bool xyz = (original.rotationType == 4);
    raw.rotationType = original.rotationType;
    if (xyz && original.rotationGroups.size() == 3) {
        raw.numRotationKeys = original.numRotationKeys;
        for (int axis = 0; axis < 3; ++axis) {
            const NifBlockFile::KeyGroup& src = original.rotationGroups.at(axis);
            NifBlockFile::KeyGroup g;
            g.count = src.count;
            g.interpolation = 1; // linear
            for (int i = 0; i < static_cast<int>(src.count); ++i) {
                const float t = src.times.value(i, 0.0f);
                const Nif::TransformKeyframe& kf = sampleAt(t);
                float euler[3] = { 0.0f, 0.0f, 0.0f };
                quaternionToEuler(kf.rotation.w, kf.rotation.x, kf.rotation.y, kf.rotation.z,
                                  euler);
                g.times.append(t);
                g.values.append(euler[axis]);
            }
            raw.rotationGroups.append(g);
        }
    } else if (!original.rotationGroups.isEmpty()) {
        raw.numRotationKeys = original.numRotationKeys;
        const NifBlockFile::KeyGroup& src = original.rotationGroups.first();
        NifBlockFile::KeyGroup g;
        g.count = src.count;
        g.interpolation = 1;
        for (int i = 0; i < static_cast<int>(src.count); ++i) {
            const float t = src.times.value(i, 0.0f);
            const Nif::TransformKeyframe& kf = sampleAt(t);
            g.times.append(t);
            g.values.append(kf.rotation.w);
            g.values.append(kf.rotation.x);
            g.values.append(kf.rotation.y);
            g.values.append(kf.rotation.z);
        }
        raw.rotationGroups.append(g);
    } else {
        // The block carries no rotation keys at all, so it must not gain any.
        raw.numRotationKeys = 0;
    }

    raw.translation.count = original.translation.count;
    raw.translation.interpolation = 1;
    for (int i = 0; i < static_cast<int>(original.translation.count); ++i) {
        const float t = original.translation.times.value(i, 0.0f);
        const Nif::TransformKeyframe& kf = sampleAt(t);
        raw.translation.times.append(t);
        raw.translation.values.append(kf.translation.x);
        raw.translation.values.append(kf.translation.y);
        raw.translation.values.append(kf.translation.z);
    }

    raw.scale.count = original.scale.count;
    raw.scale.interpolation = 1;
    for (int i = 0; i < static_cast<int>(original.scale.count); ++i) {
        const float t = original.scale.times.value(i, 0.0f);
        const Nif::TransformKeyframe& kf = sampleAt(t);
        raw.scale.times.append(t);
        raw.scale.values.append(kf.scale.x); // the block stores one scalar
    }

    return raw;
}

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
    if (dataType == QLatin1String("NiTransformData")
        || dataType == QLatin1String("NiKeyframeControllerData")) {
        NifBlockFile::NiTransformDataRaw raw;
        if (!NifBlockFile::decodeNiTransformData(original, file.version(), raw)) {
            LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') is in an "
                                "unrecognised layout; refusing to rewrite it")
                            .arg(dataIndex).arg(dataType));
            return false;
        }
        QByteArray reencoded;
        if (!NifBlockFile::encodeNiTransformData(raw, file.version(), reencoded)
            || reencoded != original) {
            LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') does not "
                                "round-trip; refusing to rewrite it")
                            .arg(dataIndex).arg(dataType));
            return false;
        }

        NifBlockFile::NiTransformDataRaw newRaw = flatToRaw(keyframes, raw);
        QByteArray encoded;
        if (!NifBlockFile::encodeNiTransformData(newRaw, file.version(), encoded)) {
            LOG_WARNING(QString("NifAnimationWriter: failed to encode keyframes for block '%1'")
                            .arg(dataType));
            return false;
        }

        NifBlockFile::NiTransformDataRaw verify;
        if (!NifBlockFile::decodeNiTransformData(encoded, file.version(), verify)
            || verify.rotationGroups.size() != newRaw.rotationGroups.size()) {
            LOG_ERROR("NifAnimationWriter: keyframe encoder failed its own round-trip check");
            return false;
        }

        file.setBlockData(dataIndex, encoded);
        return true;
    }

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
            if (!file.decodeControllerSequence(file.block(sequenceIndex).data, entries))
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
    } else {
        // No clip was named, so the edit is not scoped to one. Take every
        // controller any sequence owns: on shipped Oblivion meshes the animated
        // nodes hang off a NiMultiTargetTransformController, so no node points
        // directly at a keyframe controller and a node-only search finds
        // nothing at all. Restricting to the union of the sequences' controllers
        // still keeps the write inside the animation this file actually owns.
        const QList<int> sequences = file.findBlocks(QStringLiteral("NiControllerSequence"));
        for (int sequenceIndex : sequences) {
            QList<QPair<quint32, QString>> entries;
            if (!file.decodeControllerSequence(file.block(sequenceIndex).data, entries))
                continue;
            for (const auto& entry : entries)
                clipControllers.insert(entry.first);
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

    // A controller sequence *is* the definition of a clip: it names the set of
    // controllers that clip drives. Requiring one of them to also be reached
    // from a node by name therefore over-constrains it, and on shipped Oblivion
    // meshes it excludes every file: the animated nodes there are driven
    // through a NiMultiTargetTransformController, so no node points directly at
    // a keyframe controller and the intersection is always empty.
    //
    // So when the caller named a clip and the sequence owns keyframe
    // controllers, those controllers are the clip. The node name stays the
    // display identity of the edit, not a precondition for making it.
    if (channelsPatched == 0 && !clipControllers.isEmpty()) {
        for (quint32 controllerRef : clipControllers) {
            if (!wantedControllers.contains(controllerRef)) continue;
            if (patchControllerData(file, static_cast<int>(controllerRef), keyframes))
                ++channelsPatched;
        }
        if (channelsPatched > 0)
            matchedNode = true;
    }

if (!matchedNode) {
        LOG_WARNING(QString("NifAnimationWriter: target node '%1' not found in %2")
                        .arg(nodeName).arg(nifPath));
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
