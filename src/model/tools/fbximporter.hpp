#ifndef FBXIMPORTER_H
#define FBXIMPORTER_H

#include <QString>
#include <QStringList>

// FbxImporter models the FBX -> NIF import pipeline the real Creation Kit
// runs through its AssetWatcher. The import goes through Blender: load the
// .fbx, optionally weld skinning, keep bones/editor markers, then export via
// the bundled nif_export.py. The settings mirror what the CK exposes when
// importing a model for a record: welding skin, keeping bones and editor
// markers, and the physics LOD level to generate.
class FbxImporter
{
public:
    struct Settings
    {
        bool weldSkin = true;          // merge skinned verts at seams
        bool keepBones = true;         // retain the armature bones
        bool keepEditorMarkers = true; // keep editor-marker empties
        int physicsLod = 0;            // 0 = none, 1..3 = physics LOD level
        QString game = QStringLiteral("SKYRIM");
        QString scriptPath;            // bundled nif_export.py (auto-detected)
    };

    // Returns the path of the bundled nif_export.py (empty when missing).
    static QString exportScriptPath();

    // Builds the Blender command-line arguments for importing an FBX and
    // exporting it as NIF. Returns [blender, --background, --python, script,
    // --, input.fbx, output.nif, game].
    static QStringList blenderArguments(const QString& blenderExecutable,
                                        const QString& fbxPath,
                                        const QString& nifPath,
                                        const Settings& settings);

    // True when the settings request any non-default processing.
    static bool hasCustomProcessing(const Settings& settings);

    // Renders the settings into a short human-readable summary.
    static QString summary(const Settings& settings);
};

#endif // FBXIMPORTER_H
