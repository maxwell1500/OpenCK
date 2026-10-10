#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QProcess>
#include <QSettings>

// AssetToolCliBridge locates, launches, and drives the Bethesda asset-pipeline
// command-line tools that sit alongside the Creation Kit:
//
//   AssetTool      - Bethesda Asset Tool CLI (asset validation/packaging)
//   TextureTool    - texture conversion CLI (TIF/TGA/PNG -> DDS)
//   NifSkse        - NifSkse NIF manipulation CLI (nifopt)
//   + the CK-bundled utilities (Papyrus Compiler/Assembler, LipGenerator,
//     FaceFX, Archive2, SQLite, xEdit, AssetWatcher).
//
// Detection order per tool: manual override (QSettings), environment
// variable, then the candidate paths derived from the configured game
// roots. Batch pipelines (convertTextures, packageMaterials) fall back to
// OpenCK's in-process converters when the external CLI is not installed,
// and report per-file progress via signals for the Asset Browser dock.

class AssetToolCliBridge : public QObject
{
    Q_OBJECT

public:
    struct ToolSpec
    {
        QString id;
        QString displayName;
        QStringList envVars;        // env var holding an exe path or a dir
        QStringList candidates;     // relative exe paths under each game root
    };

    struct RunResult
    {
        bool ok = false;
        int exitCode = -1;
        QString stdOut;
        QString stdErr;
    };

    struct FileResult
    {
        QString inputPath;
        QString outputPath;
        bool success = false;
        QString message;
        double durationMs = 0.0;
    };

    struct PipelineSummary
    {
        QString name;
        int total = 0;
        int success = 0;
        int failed = 0;
        int skipped = 0;
        QStringList errors;

        bool allOk() const { return failed == 0 && skipped == 0; }
        QString summaryLine() const;
    };

    explicit AssetToolCliBridge(QObject* parent = nullptr, QSettings* settings = nullptr);
    ~AssetToolCliBridge() override;

    // Game roots: directories containing a Tools/ sub-folder (the game
    // Content folder or the CK program folder).
    void setGameRoots(const QStringList& roots);
    QStringList gameRoots() const;

    void registerTool(const ToolSpec& spec);
    QStringList toolIds() const;

    bool isToolAvailable(const QString& id);
    QString detectTool(const QString& id);
    void setToolPath(const QString& id, const QString& path);   // persisted
    QString toolPath(const QString& id) const;

    // Runs a tool with the given argument list and captures output.
    RunResult runTool(const QString& id,
                      const QStringList& args,
                      int timeoutMs = 120000,
                      const QString& workingDir = QString());

    // Batch TIF/TGA/PNG -> DDS (or tga/png) conversion. Uses TextureTool
    // when available, otherwise the in-process AssetConverter.
    PipelineSummary convertTextures(const QStringList& inputs,
                                    const QString& outputDir,
                                    const QString& format = QStringLiteral("dds"),
                                    const QString& rulesPath = QString());

    // Batch .mat packaging: applies the named rule template to each
    // material's texture-slot map (JSON: {name, template, textures}),
    // resolves textures against the texture root, and writes a
    // <name>.compiled.json manifest per material.
    PipelineSummary packageMaterials(const QStringList& materialJsons,
                                     const QString& rulesDir,
                                     const QString& outputDir,
                                     const QString& textureRoot = QString());

signals:
    void toolDetected(const QString& id, const QString& path);
    void fileProcessed(const QString& input, bool success, const QString& message);
    void pipelineStarted(const QString& name, int fileCount);
    void pipelineFinished(const AssetToolCliBridge::PipelineSummary& summary);

private:
    void registerDefaults(const QStringList& roots);
    void clearDetectionCache();
    QString resolveCandidate(const ToolSpec& spec, const QString& root) const;

    QStringList mRoots;
    QMap<QString, ToolSpec> mTools;
    QMap<QString, QString> mDetected;
    QSettings* mSettings;
    bool mOwnsSettings = false;
};
