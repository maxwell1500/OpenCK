#pragma once

#include <QString>
#include <QVector>
#include <QList>
#include <QPair>
#include <QColor>
#include <QPointF>
#include <functional>
#include "../nifparser.hpp"
#include "logger.hpp"

enum class RendererType { Sprite = 0, Beam = 1, Ring = 2, Point = 3 };

struct ParticleSystemData {
    QString name;
    QString systemType = "Update";

    float emissionRate = 20.0f;
    float lifetime = 1.0f;
    float lifetimeRandom = 0.0f;
    float minSpeed = 1.0f;
    float maxSpeed = 1.0f;
    float xSpread = 0.0f, ySpread = 0.0f, zSpread = 0.0f;
    float xVelocity = 0.0f, yVelocity = 0.0f, zVelocity = 0.0f;
    float minRadius = 0.0f, maxRadius = 1.0f;
    float minAngle = 0.0f, maxAngle = 3.14159f;
    float gravityStrength = 0.0f;

    float startSize = 1.0f;
    float startSizeRandom = 0.0f;
    float endSize = 0.0f;
    float endSizeRandom = 0.0f;

    float startR = 1.0f, startG = 1.0f, startB = 1.0f, startA = 1.0f;
    float endR = 1.0f, endG = 1.0f, endB = 1.0f, endA = 0.0f;

    QString textureFile;
    quint32 columns = 1;
    quint32 numRows = 1;
    RendererType rendererType = RendererType::Sprite;
    bool additiveBlending = false;
    bool alphaTest = false;
    quint32 maxParticles = 1000;

    float initialRotation = 0.0f;
    float rotationSpeed = 0.0f;
    float rotationRandom = 0.0f;

    QVector<QPair<float, QColor>> colorOverLifetime;
    QVector<QPointF> sizeOverLifetime;

    struct GravityModifier {
        float strength = 0.0f;
        float dirX = 0.0f, dirY = 0.0f, dirZ = -1.0f;
    };
    QVector<GravityModifier> gravityModifiers;

    struct DragModifier {
        float strength = 0.0f;
    };
    QVector<DragModifier> dragModifiers;
};

class ParticleEffectsParser {
public:
    static QList<ParticleSystemData*> parse(const QString& nifPath) {
        QList<ParticleSystemData*> result;

        Nif::NifParser parser;
        if (!parser.load(nifPath)) {
            LOG_ERROR(QString("Failed to load NIF for particles: %1").arg(nifPath));
            return result;
        }

        Nif::Node* root = parser.getRoot();
        if (!root) {
            return result;
        }

        std::function<void(Nif::Node*)> findParticles = [&](Nif::Node* node) {
            if (!node) return;

            // Read particle system settings from NIF blocks attached to nodes
            if (!node->particleSystems.isEmpty()) {
                for (const auto& ps : node->particleSystems) {
                    ParticleSystemData* psys = new ParticleSystemData();
                    psys->name = node->name;
                    psys->systemType = "Update";

                    psys->emissionRate = ps.emissionRate;
                    psys->lifetime = ps.lifetime;
                    psys->lifetimeRandom = ps.lifetimeRandom;
                    psys->minSpeed = ps.speed;
                    psys->maxSpeed = ps.speed + ps.speedRandom;
                    psys->xSpread = ps.spread;
                    psys->ySpread = ps.spread;
                    psys->zSpread = ps.spread;
                    psys->gravityStrength = ps.gravityStrength;

                    psys->startSize = ps.startSize;
                    psys->startSizeRandom = ps.startSizeRandom;
                    psys->endSize = ps.endSize;
                    psys->endSizeRandom = ps.endSizeRandom;

                    psys->startR = ps.startColorR;
                    psys->startG = ps.startColorG;
                    psys->startB = ps.startColorB;
                    psys->startA = ps.startColorA;
                    psys->endR = ps.endColorR;
                    psys->endG = ps.endColorG;
                    psys->endB = ps.endColorB;
                    psys->endA = ps.endColorA;

                    psys->textureFile = ps.texturePath;
                    psys->columns = ps.numColumns;
                    psys->numRows = ps.numRows;
                    psys->additiveBlending = ps.additiveBlending;
                    psys->alphaTest = ps.alphaTest;

                    result.append(psys);
                }
            }

            // Also check node names as a fallback for nodes that reference particles
            // but may not have parsed block data
            if (result.isEmpty()) {
                QString nodeName = node->name.toLower();
                if (nodeName.contains("particle") || nodeName.contains("emitter") || nodeName.contains("psys")) {
                    ParticleSystemData* psys = new ParticleSystemData();
                    psys->name = node->name;
                    psys->systemType = "Update";

                    for (auto& shape : node->shapes) {
                        if (!shape.texture.isEmpty()) {
                            psys->textureFile = shape.texture;
                            break;
                        }
                    }

                    result.append(psys);
                }
            }

            for (auto child : node->children) {
                findParticles(child);
            }
        };

        findParticles(root);

        LOG_INFO(QString("Found %1 particle system(s) in %2").arg(result.size()).arg(nifPath));
        return result;
    }

    static void cleanup(QList<ParticleSystemData*>& systems) {
        for (auto sys : systems) {
            delete sys;
        }
        systems.clear();
    }
};
