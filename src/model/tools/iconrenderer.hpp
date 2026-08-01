#ifndef ICONRENDERER_H
#define ICONRENDERER_H

#include <QString>
#include <QVector>
#include <QColor>

// IconRenderer drives the Blender icon-generation script
// (scripts/blender/icon_generator.py) that renders a NIF into a square icon
// with a three-light rig (warm fill, cool rim, key light) over a transparent
// background. The renderer owns the per-context size table (inventory,
// workshop, shipbuilder) and the light-rig defaults the real Creation Kit
// configures in its [Preview]/[IconGenerator] sections.
class IconRenderer
{
public:
    // The output contexts the renderer supports, each with its own size.
    enum class Context
    {
        Inventory,     // 128x128 inventory icons
        Workshop,      // 512x512 workshop / settlement
        ShipBuilder    // 512x512 ship builder
    };

    // Returns the square output size in pixels for a context.
    static int contextSize(Context ctx);

    // Returns a user-facing name for a context.
    static QString contextName(Context ctx);

    // One light in the rig.
    struct Light
    {
        QString name;
        QColor color;
        double x, y, z;
        double energy;
    };

    // The default three-light rig: warm fill front-left, cool rim back-right,
    // and a brighter key light front-top.
    static QVector<Light> defaultRig();

    // Returns the path of the Blender icon script shipped next to the
    // executable (scripts/blender/icon_generator.py). Empty when missing.
    static QString scriptPath();

    // Builds the command-line arguments passed to Blender for the given
    // input NIF, output icon path, and context size. The first argument is
    // the Blender executable.
    static QStringList blenderArguments(const QString& blenderExecutable,
                                        const QString& nifPath,
                                        const QString& outputIconPath,
                                        Context ctx);
};

#endif // ICONRENDERER_H
