#include "assettoolcli.hpp"
#include "assetconverter.hpp"
#include "materialruletemplate.hpp"
#include "materialcompiler.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../files/log/logger.hpp"

namespace
{

QStringList defaultGameRoots()
{
    QStringList roots;
    const QString env = qEnvironmentVariable("OPENCK_DATA_DIR");
    if (!env.isEmpty())
        roots << env;
    roots << QStringLiteral("C:/XboxGames/Starfield/Content")
           << QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Starfield")
           << QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition")
           << QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Fallout 4");
    return roots;
}

QString firstLine(const QString& text)
{
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    return lines.isEmpty() ? QString() : lines.first().trimmed();
}

QJsonArray stringListToJsonArray(const QStringList& list)
{
    QJsonArray arr;
    for (const QString& s : list)
        arr.append(s);
    return arr;
}

} // namespace

AssetToolCliBridge::AssetToolCliBridge(QObject* parent, QSettings* settings)
    : QObject(parent),
      mSettings(settings)
{
    if (!mSettings)
    {
        mSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                  QStringLiteral("OpenCK"), QStringLiteral("OpenCK"));
        mOwnsSettings = true;
    }
    mRoots = defaultGameRoots();
    registerDefaults(mRoots);
}

AssetToolCliBridge::~AssetToolCliBridge()
{
    if (mOwnsSettings)
        delete mSettings;
}

void AssetToolCliBridge::setGameRoots(const QStringList& roots)
{
    mRoots = roots;
    registerDefaults(mRoots);
}

QStringList AssetToolCliBridge::gameRoots() const
{
    return mRoots;
}

void AssetToolCliBridge::registerTool(const ToolSpec& spec)
{
    mTools.insert(spec.id, spec);
    clearDetectionCache();
}

QStringList AssetToolCliBridge::toolIds() const
{
    return mTools.keys();
}

void AssetToolCliBridge::registerDefaults(const QStringList& roots)
{
    Q_UNUSED(roots);   // roots are read from mRoots at detection time

    const ToolSpec assetTool = {
        QStringLiteral("AssetTool"), QStringLiteral("Bethesda Asset Tool CLI"),
        { QStringLiteral("OPENCK_ASSETTOOL") },
        { QStringLiteral("Tools/AssetTool/AssetToolCLI.exe"),
          QStringLiteral("Tools/AssetToolCLI.exe") }
    };
    const ToolSpec textureTool = {
        QStringLiteral("TextureTool"), QStringLiteral("Texture conversion CLI"),
        { QStringLiteral("OPENCK_TEXTURETOOL") },
        { QStringLiteral("Tools/TextureTool/TextureToolCLI.exe"),
          QStringLiteral("Tools/TextureTool/TextureTool.exe"),
          QStringLiteral("Tools/xtexconv/xtexconv.exe") }
    };
    const ToolSpec nifSkse = {
        QStringLiteral("NifSkse"), QStringLiteral("NifSkse NIF tool (nifopt)"),
        { QStringLiteral("OPENCK_NIFSKSE") },
        { QStringLiteral("nifopt.exe") }
    };
    const ToolSpec papyrusCompiler = {
        QStringLiteral("PapyrusCompiler"), QStringLiteral("Papyrus Compiler"),
        { QStringLiteral("OPENCK_PAPYRUS_COMPILER") },
        { QStringLiteral("Tools/Papyrus Compiler/PapyrusCompiler.exe") }
    };
    const ToolSpec papyrusAssembler = {
        QStringLiteral("PapyrusAssembler"), QStringLiteral("Papyrus Assembler"),
        { QStringList() },
        { QStringLiteral("Tools/Papyrus Compiler/PapyrusAssembler.exe") }
    };
    const ToolSpec lipGenerator = {
        QStringLiteral("LipGenerator"), QStringLiteral("Lip Sync Generator"),
        { QStringList() },
        { QStringLiteral("Tools/LipGenerator/LipGenerator.exe") }
    };
    const ToolSpec faceFx = {
        QStringLiteral("FaceFx"), QStringLiteral("FaceFX Compiler"),
        { QStringList() },
        { QStringLiteral("Tools/FaceFX/ffxc.exe") }
    };
    const ToolSpec iconGenerator = {
        QStringLiteral("IconGenerator"), QStringLiteral("Icon Generator"),
        { QStringList() },
        { QStringLiteral("Tools/IconGenerator/IconGenerator.exe") }
    };
    const ToolSpec archive2 = {
        QStringLiteral("Archive2"), QStringLiteral("Archive2 (BA2 packer)"),
        { QStringList() },
        { QStringLiteral("Tools/Archive2/Archive2.exe") }
    };
    const ToolSpec sqlite3 = {
        QStringLiteral("Sqlite3"), QStringLiteral("SQLite3 CLI"),
        { QStringList() },
        { QStringLiteral("Tools/SQlite3/sqlite3.exe") }
    };
    const ToolSpec xedit = {
        QStringLiteral("XEdit"), QStringLiteral("xEdit"),
        { QStringList() },
        { QStringLiteral("Tools/xEdit/xEdit.exe") }
    };
    const ToolSpec assetWatcher = {
        QStringLiteral("AssetWatcher"), QStringLiteral("Asset Watcher"),
        { QStringList() },
        { QStringLiteral("Tools/AssetWatcher/AssetWatcher.exe") }
    };

    const ToolSpec defaults[] = {
        assetTool, textureTool, nifSkse, papyrusCompiler, papyrusAssembler,
        lipGenerator, faceFx, iconGenerator, archive2, sqlite3, xedit, assetWatcher
    };
    for (const ToolSpec& spec : defaults)
        mTools.insert(spec.id, spec);

    clearDetectionCache();
}

void AssetToolCliBridge::clearDetectionCache()
{
    mDetected.clear();
}

QString AssetToolCliBridge::resolveCandidate(const ToolSpec& spec, const QString& root) const
{
    for (const QString& cand : spec.candidates)
    {
        const QString full = QDir(root).absoluteFilePath(cand);
        if (QFile::exists(full))
            return full;
    }
    return QString();
}

QString AssetToolCliBridge::detectTool(const QString& id)
{
    if (!mTools.contains(id))
        return QString();

    if (mDetected.contains(id))
        return mDetected.value(id);

    const ToolSpec spec = mTools.value(id);
    QString found;

    // 1. Manual override (persisted).
    const QString manual = mSettings->value(QStringLiteral("toolpaths/") + id).toString();
    if (!manual.isEmpty() && QFile::exists(manual))
        found = manual;

    // 2. Environment variable (exe path, or a directory containing the tool).
    if (found.isEmpty())
    {
        for (const QString& var : spec.envVars)
        {
            const QString value = qEnvironmentVariable(var.toUtf8().constData());
            if (value.isEmpty()) continue;
            if (QFile::exists(value))
            {
                found = value;
                break;
            }
            if (QDir(value).exists() && !spec.candidates.isEmpty())
            {
                const QString inDir = QDir(value).absoluteFilePath(spec.candidates.first());
                if (QFile::exists(inDir))
                {
                    found = inDir;
                    break;
                }
            }
        }
    }

    // 3. Candidate paths under each game root.
    if (found.isEmpty())
    {
        for (const QString& root : mRoots)
        {
            found = resolveCandidate(spec, root);
            if (!found.isEmpty()) break;
        }
    }

    if (!found.isEmpty())
    {
        LOG_INFO(QString("AssetToolCliBridge: detected %1 at %2").arg(id, found));
        emit toolDetected(id, found);
    }
    mDetected.insert(id, found);
    return found;
}

bool AssetToolCliBridge::isToolAvailable(const QString& id)
{
    return !detectTool(id).isEmpty();
}

void AssetToolCliBridge::setToolPath(const QString& id, const QString& path)
{
    if (path.isEmpty())
        mSettings->remove(QStringLiteral("toolpaths/") + id);
    else
        mSettings->setValue(QStringLiteral("toolpaths/") + id, path);
    mSettings->sync();
    clearDetectionCache();
}

QString AssetToolCliBridge::toolPath(const QString& id) const
{
    return mSettings->value(QStringLiteral("toolpaths/") + id).toString();
}

AssetToolCliBridge::RunResult AssetToolCliBridge::runTool(const QString& id,
                                                          const QStringList& args,
                                                          int timeoutMs,
                                                          const QString& workingDir)
{
    RunResult result;
    const QString exe = detectTool(id);
    if (exe.isEmpty())
    {
        result.stdErr = QStringLiteral("Tool not found: %1").arg(id);
        LOG_ERROR(result.stdErr);
        return result;
    }

    QProcess process;
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    LOG_DEBUG(QString("AssetToolCliBridge: running %1 %2").arg(exe, args.join(' ')));
    process.start(exe, args);
    if (!process.waitForStarted(5000))
    {
        result.stdErr = QStringLiteral("Failed to start %1").arg(exe);
        LOG_ERROR(result.stdErr);
        return result;
    }
    if (!process.waitForFinished(timeoutMs))
    {
        process.kill();
        process.waitForFinished(2000);
        result.stdErr = QStringLiteral("Timed out after %1 ms").arg(timeoutMs);
        LOG_ERROR(QString("AssetToolCliBridge: %1 %2").arg(id, result.stdErr));
        return result;
    }

    result.exitCode = process.exitCode();
    result.stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.stdErr = QString::fromLocal8Bit(process.readAllStandardError());
    result.ok = (result.exitCode == 0);
    if (!result.ok)
        LOG_WARNING(QString("AssetToolCliBridge: %1 exited with code %2: %3")
            .arg(id).arg(result.exitCode).arg(firstLine(result.stdErr)));
    return result;
}

AssetToolCliBridge::PipelineSummary AssetToolCliBridge::convertTextures(
    const QStringList& inputs,
    const QString& outputDir,
    const QString& format,
    const QString& rulesPath)
{
    emit pipelineStarted(QStringLiteral("texture-conversion"), inputs.size());

    PipelineSummary summary;
    summary.name = QStringLiteral("texture-conversion");
    summary.total = inputs.size();

    const bool haveTool = isToolAvailable(QStringLiteral("TextureTool"));
    for (const QString& input : inputs)
    {
        if (!QFile::exists(input))
        {
            ++summary.skipped;
            emit fileProcessed(input, false, QStringLiteral("file not found"));
            continue;
        }

        QElapsedTimer timer;
        timer.start();
        FileResult fr;
        fr.inputPath = input;
        fr.outputPath = QDir(outputDir).absoluteFilePath(
            QFileInfo(input).completeBaseName() + QLatin1Char('.') + format);

        if (haveTool)
        {
            const RunResult r = runTool(QStringLiteral("TextureTool"),
                                         { input, QStringLiteral("--out"), outputDir,
                                           QStringLiteral("--format"), format },
                                         120000);
            fr.success = r.ok;
            fr.message = r.ok
                ? QStringLiteral("converted via TextureTool")
                : QStringLiteral("TextureTool failed: %1").arg(firstLine(r.stdErr + QLatin1Char('\n') + r.stdOut));
        }
        else
        {
            const AssetConverter::ConversionResult cr = rulesPath.isEmpty()
                ? AssetConverter::convertTextures({ input }, outputDir, format)
                : AssetConverter::convertTexturesByRules({ input }, outputDir, rulesPath);
            fr.success = cr.success && cr.filesConverted > 0;
            fr.message = fr.success
                ? QStringLiteral("converted to %1 (in-process fallback)").arg(format)
                : cr.error;
        }

        fr.durationMs = timer.elapsed();
        emit fileProcessed(input, fr.success, fr.message);
        if (fr.success)
            ++summary.success;
        else
        {
            ++summary.failed;
            summary.errors << input + QStringLiteral(": ") + fr.message;
        }
    }

    emit pipelineFinished(summary);
    return summary;
}

AssetToolCliBridge::PipelineSummary AssetToolCliBridge::packageMaterials(
    const QStringList& materialJsons,
    const QString& rulesDir,
    const QString& outputDir,
    const QString& textureRoot)
{
    emit pipelineStarted(QStringLiteral("material-packaging"), materialJsons.size());

    QVector<MaterialRuleTemplate> templates;
    MaterialRuleTemplate::loadDirectory(rulesDir, templates);
    QMap<QString, int> templateIndex;
    for (int i = 0; i < templates.size(); ++i)
        templateIndex.insert(templates[i].name, i);

    QDir outDir(outputDir);
    if (!outDir.exists())
        outDir.mkpath(QStringLiteral("."));

    PipelineSummary summary;
    summary.name = QStringLiteral("material-packaging");
    summary.total = materialJsons.size();

    for (const QString& path : materialJsons)
    {
        if (!QFile::exists(path))
        {
            ++summary.skipped;
            emit fileProcessed(path, false, QStringLiteral("file not found"));
            continue;
        }

        QJsonParseError perr;
        QFile mfIn(path);
        if (!mfIn.open(QIODevice::ReadOnly))
        {
            ++summary.failed;
            summary.errors << path + QStringLiteral(": cannot open file");
            emit fileProcessed(path, false, QStringLiteral("cannot open file"));
            continue;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(mfIn.readAll(), &perr);
        mfIn.close();
        if (perr.error != QJsonParseError::NoError || !doc.isObject())
        {
            ++summary.failed;
            summary.errors << path + QStringLiteral(": invalid JSON");
            emit fileProcessed(path, false, QStringLiteral("invalid JSON"));
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString name = obj.value(QStringLiteral("name")).toString(
            QFileInfo(path).completeBaseName());
        const QString tplName = obj.value(QStringLiteral("template")).toString();

        QMap<QString, QString> textures;
        const QJsonObject texObj = obj.value(QStringLiteral("textures")).toObject();
        for (auto it = texObj.constBegin(); it != texObj.constEnd(); ++it)
            textures.insert(it.key(), it.value().toString());

        MaterialRuleTemplate tpl;
        if (!tplName.isEmpty())
        {
            if (!templateIndex.contains(tplName))
            {
                ++summary.failed;
                summary.errors << name + QStringLiteral(": template not found: ") + tplName;
                emit fileProcessed(path, false, QStringLiteral("template not found: %1").arg(tplName));
                continue;
            }
            tpl = templates[templateIndex.value(tplName)];
        }
        else
        {
            tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("1LayerStandard"));
        }

        const MaterialCompileReport report = MaterialCompiler::compile(tpl, textures, textureRoot);

        // Write the compiled manifest.
        QJsonObject manifest;
        manifest.insert(QStringLiteral("name"), name);
        manifest.insert(QStringLiteral("template"), report.templateName);
        manifest.insert(QStringLiteral("status"), report.ok ? QStringLiteral("ok")
                                                            : QStringLiteral("incomplete"));
        manifest.insert(QStringLiteral("appliedRules"),
                        stringListToJsonArray(report.appliedOperations));

        QJsonObject slotsObj;
        for (const QString& slot : report.finalSlots)
        {
            QJsonObject so;
            so.insert(QStringLiteral("path"), textures.value(slot));
            so.insert(QStringLiteral("resolved"), report.resolvedSlots.value(slot));
            slotsObj.insert(slot, so);
        }
        manifest.insert(QStringLiteral("slots"), slotsObj);
        manifest.insert(QStringLiteral("missingSlots"),
                        stringListToJsonArray(report.missingSlots));
        manifest.insert(QStringLiteral("unresolvedTextures"),
                        stringListToJsonArray(report.unresolvedTextures));

        const QString manifestPath = outDir.absoluteFilePath(name + QStringLiteral(".compiled.json"));
        QFile mf(manifestPath);
        bool written = false;
        if (mf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            mf.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
            mf.close();
            written = true;
        }

        const bool ok = written && report.ok;
        const QString message = written
            ? report.summary()
            : QStringLiteral("failed to write manifest: %1").arg(manifestPath);
        emit fileProcessed(path, ok, message);
        if (ok)
            ++summary.success;
        else
        {
            ++summary.failed;
            summary.errors << name + QStringLiteral(": ") + message;
        }
    }

    emit pipelineFinished(summary);
    return summary;
}

QString AssetToolCliBridge::PipelineSummary::summaryLine() const
{
    return QStringLiteral("%1: %2/%3 ok (%4 failed, %5 skipped)")
        .arg(name)
        .arg(success)
        .arg(total)
        .arg(failed)
        .arg(skipped);
}
