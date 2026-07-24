#include "nifanimationwriter.hpp"
#include "nifparser.hpp"
#include "nifrecord.hpp"

#include <QFile>
#include <QDir>

#include "logger.hpp"

bool NifAnimationWriter::writeKeyframesToNif(const QString& nifPath,
                                               const QString& nodeName,
                                               const QVector<Nif::TransformKeyframe>& keyframes)
{
    QFile file(nifPath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to open NIF: %1").arg(nifPath));
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.size() < 12) {
        LOG_ERROR("NifAnimationWriter: NIF file too small");
        return false;
    }

    // Write the keyframe data back using NifParser for proper structure handling
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

    // Find the target node and update its animation keyframes
    bool found = false;
    auto updateNode = [&](auto* node, auto&& self) -> bool {
        if (!node) return false;

        for (auto& anim : node->animations) {
            if (anim.clipName == nodeName || QString::number(anim.targetNode) == nodeName) {
                // Check if keyframe count matches
                if (anim.keyframes.size() != keyframes.size()) {
                    LOG_WARNING(QString("NifAnimationWriter: keyframe count mismatch for node %1 "
                                        "(existing: %2, new: %3)")
                                    .arg(nodeName)
                                    .arg(anim.keyframes.size())
                                    .arg(keyframes.size()));
                    return false;
                }

                // Update keyframes in-place
                for (int i = 0; i < keyframes.size(); ++i) {
                    anim.keyframes[i] = keyframes[i];
                }
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

    QString tmpPath = nifPath + ".tmp";
    if (!parser.save(tmpPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to save NIF to temp file: %1").arg(tmpPath));
        QFile::remove(tmpPath);
        return false;
    }

    if (!QFile::remove(nifPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to remove original NIF: %1").arg(nifPath));
        QFile::remove(tmpPath);
        return false;
    }

    if (!QFile::rename(tmpPath, nifPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to rename temp NIF to: %1").arg(nifPath));
        return false;
    }

    LOG_INFO(QString("NifAnimationWriter: successfully wrote keyframes to NIF: %1").arg(nifPath));
    return true;
}
