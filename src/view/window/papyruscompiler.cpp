#include "papyruscompiler.hpp"
#include "logger.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QRegularExpression>

PapyrusCompiler::PapyrusCompiler(QObject* parent)
    : QObject(parent),
      process(this)
{
    connect(&process, &QProcess::readyReadStandardOutput, this, [this]() {
        QString text = QString::fromLocal8Bit(process.readAllStandardOutput());
        output += text;
        emit logMessage(text, false);
    });

    connect(&process, &QProcess::readyReadStandardError, this, [this]() {
        QString text = QString::fromLocal8Bit(process.readAllStandardError());
        output += text;
        emit logMessage(text, true);
    });

    connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        parseCompilerOutput(output);
        bool success = (exitCode == 0);
        emit compilationFinished(success);
        if (!success) {
            emit compilationError(QString("Compilation failed with exit code %1").arg(exitCode));
        }
    });
}

PapyrusCompiler::~PapyrusCompiler()
{
}

bool PapyrusCompiler::setCompilerPath(const QString& path)
{
    if (path.isEmpty()) {
        LOG_ERROR("PapyrusCompiler: Empty compiler path");
        return false;
    }

    QFileInfo fileInfo(path);
    
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("PapyrusCompiler: Compiler file does not exist: %1").arg(path));
        return false;
    }

    if (!fileInfo.isFile()) {
        LOG_ERROR(QString("PapyrusCompiler: Path is not a file: %1").arg(path));
        return false;
    }

    if (!fileInfo.isExecutable()) {
        LOG_ERROR(QString("PapyrusCompiler: File is not executable: %1").arg(path));
        return false;
    }

    QString fileName = fileInfo.fileName().toLower();
    if (!fileName.contains("pp64") && !fileName.contains("papyrus")) {
        LOG_WARNING(QString("PapyrusCompiler: File name doesn't match expected pp64.exe pattern: %1").arg(path));
    }

    qint64 fileSize = fileInfo.size();
    if (fileSize < 1024 * 1024) {
        LOG_WARNING(QString("PapyrusCompiler: File size seems too small for pp64.exe: %1 bytes").arg(fileSize));
    }

    compilerPath = path;
    LOG_INFO(QString("PapyrusCompiler: Set compiler path to %1 (%2 bytes)").arg(path).arg(fileSize));
    return true;
}

QString PapyrusCompiler::detectCompilerPath()
{
    QStringList candidates;
    
    QString homeDir = QDir::homePath();
    QString appData = qgetenv("APPDATA");
    QString localAppData = qgetenv("LOCALAPPDATA");
    
    if (!appData.isEmpty()) {
        candidates << appData + "/SkyrimSE";
        candidates << appData + "/Skyrim Special Edition";
    }
    
    if (!localAppData.isEmpty()) {
        candidates << localAppData + "/Skyrim Special Edition";
        candidates << localAppData + "/ModOrganizer2/skyrimse";
        candidates << localAppData + "/Vortex/skyrimse";
    }
    
    candidates << homeDir + "/Documents/My Games/Skyrim Special Edition";
    
    foreach (const QString& dir, candidates) {
        QString pp64Path = dir + "/pp64.exe";
        if (QFile::exists(pp64Path)) {
            LOG_INFO(QString("PapyrusCompiler: Found pp64.exe at %1").arg(pp64Path));
            return pp64Path;
        }
    }
    
    LOG_WARNING("PapyrusCompiler: pp64.exe not found in common locations");
    return QString();
}

bool PapyrusCompiler::setScriptPath(const QString& path)
{
    scriptPath = path;
    return QFile::exists(path);
}

bool PapyrusCompiler::setOutputPath(const QString& path)
{
    outputPath = path;
    QDir().mkpath(path);
    return true;
}

void PapyrusCompiler::addIncludePath(const QString& path)
{
    if (!includePaths.contains(path)) {
        includePaths.append(path);
    }
}

void PapyrusCompiler::setGameVersion(GameId version)
{
    gameVersion = version;
    LOG_INFO(QString("PapyrusCompiler: Game version set to %1").arg(FilePaths::gameName(version)));
}

QStringList PapyrusCompiler::getCompilerFlags() const
{
    QStringList flags;

    switch (gameVersion) {
        case Game_Skyrim:
        case Game_SkyrimSpecialEdition:
        case Game_SkyrimAnniversaryEdition:
            flags << QStringLiteral("-i=\"Data/Scripts/Source\"")
                  << QStringLiteral("-o=\"Data/Scripts\"")
                  << QStringLiteral("-f=\"Papyrus Flags.flg\"");
            break;
        case Game_Fallout4:
            flags << QStringLiteral("-i=\"Data/Scripts/Source\"")
                  << QStringLiteral("-o=\"Data/Scripts\"")
                  << QStringLiteral("-f=\"Papyrus Flags.flg\"");
            break;
        case Game_Starfield:
            flags << QStringLiteral("-i=\"Data/Scripts/Source\"")
                  << QStringLiteral("-o=\"Data/Scripts\"")
                  << QStringLiteral("-f=\"Papyrus Flags.flg\"");
            break;
        case Game_Morrowind:
        case Game_Oblivion:
        case Game_Fallout3:
        case Game_FalloutNewVegas:
        case Game_None:
        default:
            flags << QStringLiteral("-i=\"Data/Scripts/Source\"")
                  << QStringLiteral("-o=\"Data/Scripts\"")
                  << QStringLiteral("-f=\"Papyrus Flags.flg\"");
            break;
    }

    return flags;
}

bool PapyrusCompiler::compile()
{
    if (compilerPath.isEmpty()) {
        emit compilationError("Compiler path not set");
        return false;
    }

    if (scriptPath.isEmpty()) {
        emit compilationError("Script path not set");
        return false;
    }

    if (!QFile::exists(compilerPath)) {
        emit compilationError(QString("Compiler not found: %1").arg(compilerPath));
        return false;
    }

    if (!QFile::exists(scriptPath)) {
        emit compilationError(QString("Script not found: %1").arg(scriptPath));
        return false;
    }

    output.clear();
    lastErrors.clear();

    emit compilationStarted();

    QStringList arguments;

    // Add script file
    arguments << escapePath(scriptPath);

    // Add output directory
    if (!outputPath.isEmpty()) {
        arguments << "-output" << escapePath(outputPath);
    }

    // Add mod file
    if (!modFile.isEmpty()) {
        arguments << "-mod" << escapePath(modFile);
    }

    // Add game-specific compiler flags
    arguments << getCompilerFlags();

    // Add include paths
    for (const auto& includePath : includePaths) {
        arguments << "-i" << escapePath(includePath);
    }

    // Add libraries
    for (const auto& lib : libraries) {
        arguments << "-lib" << escapePath(lib);
    }

    LOG_INFO(QString("Compiling %1 with pp64.exe").arg(scriptPath));
    LOG_INFO(QString("Command: %1 %2").arg(compilerPath, arguments.join(" ")));

    process.start(compilerPath, arguments);

    if (!process.waitForStarted()) {
        emit compilationError(QString("Failed to start compiler: %1").arg(process.errorString()));
        return false;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        emit compilationError("Compilation timed out after 30 seconds");
        return false;
    }

    return process.exitCode() == 0;
}

QString PapyrusCompiler::escapePath(const QString& path) const
{
    if (path.contains(' ') || path.contains('"')) {
        return QString("\"%1\"").arg(path);
    }
    return path;
}

void PapyrusCompiler::parseCompilerOutput(const QString& output)
{
    lastErrors.clear();

    QString text = output;
    QStringList lines = text.split('\n');

    for (const auto& line : lines) {
        if (!line.contains("Error") && !line.contains("Warning") && !line.contains("Fatal")) {
            continue;
        }

        CompilerError error;
        error.line = 0;
        error.column = 0;
        bool parsed = false;

        if (parseWithStrategy1(line, error)) {
            parsed = true;
        } else if (parseWithStrategy2(line, error)) {
            parsed = true;
        } else if (parseWithStrategy3(line, error)) {
            parsed = true;
        } else if (parseWithStrategy4(line, error)) {
            parsed = true;
        }

        if (parsed) {
            lastErrors.append(error);
        }
    }

    if (!lastErrors.isEmpty()) {
        LOG_WARNING(QString("Compilation produced %1 errors/warnings").arg(lastErrors.size()));
    }
}

bool PapyrusCompiler::parseWithStrategy1(const QString& line, CompilerError& error)
{
    if (!line.contains(".psc", Qt::CaseInsensitive)) {
        return false;
    }

    if (line.contains("Error", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Error;
    } else if (line.contains("Fatal", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Fatal;
    } else if (line.contains("Warning", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Warning;
    } else {
        return false;
    }

    int pscPos = line.indexOf(".psc", Qt::CaseInsensitive);
    if (pscPos < 0) {
        return false;
    }

    int fileEnd = pscPos + 4;
    int fileStart = fileEnd;
    while (fileStart > 0 && line[fileStart - 1] != '/' && line[fileStart - 1] != '\\' && !line[fileStart - 1].isSpace()) {
        fileStart--;
    }
    error.file = line.mid(fileStart, fileEnd - fileStart);

    int linePos = line.indexOf("line ", pscPos);
    if (linePos < 0) {
        linePos = line.indexOf('(', pscPos);
        if (linePos >= 0) {
            int parenEnd = line.indexOf(')', linePos);
            if (parenEnd > linePos) {
                QString numStr = line.mid(linePos + 1, parenEnd - linePos - 1);
                if (!numStr.isEmpty() && numStr.toInt() > 0) {
                    error.line = numStr.toInt();
                }
            }
        }
    } else {
        error.line = extractLineNumber(line, linePos + 5).toInt();
    }

    int colonAfterPsc = line.indexOf(':', fileEnd);
    if (colonAfterPsc >= 0) {
        int msgStart = colonAfterPsc + 1;
        while (msgStart < line.length() && line[msgStart].isSpace()) {
            msgStart++;
        }
        error.message = line.mid(msgStart).trimmed();
    } else {
        error.message = line.trimmed();
    }

    return true;
}

bool PapyrusCompiler::parseWithStrategy2(const QString& line, CompilerError& error)
{
    if (!line.contains(".psc", Qt::CaseInsensitive)) {
        return false;
    }

    int pscPos = line.indexOf(".psc", Qt::CaseInsensitive);
    int parenOpen = line.indexOf('(', pscPos);
    int parenClose = line.indexOf(')', parenOpen);

    if (parenOpen < 0 || parenClose < 0) {
        return false;
    }

    QString numStr = line.mid(parenOpen + 1, parenClose - parenOpen - 1);
    bool ok = false;
    int lineNumber = numStr.toInt(&ok);
    if (!ok || lineNumber <= 0) {
        return false;
    }

    if (line.contains("Error", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Error;
    } else if (line.contains("Fatal", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Fatal;
    } else if (line.contains("Warning", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Warning;
    } else {
        return false;
    }

    int fileEnd = pscPos + 4;
    int fileStart = fileEnd;
    while (fileStart > 0 && line[fileStart - 1] != '/' && line[fileStart - 1] != '\\' && !line[fileStart - 1].isSpace()) {
        fileStart--;
    }
    error.file = line.mid(fileStart, fileEnd - fileStart);
    error.line = lineNumber;

    int afterParen = parenClose + 1;
    int colonAfterParen = line.indexOf(':', afterParen);
    if (colonAfterParen >= 0) {
        int msgStart = colonAfterParen + 1;
        while (msgStart < line.length() && line[msgStart].isSpace()) {
            msgStart++;
        }
        error.message = line.mid(msgStart).trimmed();
    } else {
        error.message = line.trimmed();
    }

    return true;
}

bool PapyrusCompiler::parseWithStrategy3(const QString& line, CompilerError& error)
{
    if (!line.contains("error", Qt::CaseInsensitive) && !line.contains("warning", Qt::CaseInsensitive)) {
        return false;
    }

    QRegularExpression re(R"(([\\\/][\w\-\.]+\.psc)\s*\((\d+)\)\s*:\s*(error|warning|fatal)\s*:\s*(.*))");
    QRegularExpressionMatch match = re.match(line);

    if (match.hasMatch()) {
        error.file = match.captured(1).mid(1);
        error.line = match.captured(2).toInt();
        QString severityStr = match.captured(3).toLower();
        
        if (severityStr == "fatal") {
            error.severity = CompilerError::Severity::Fatal;
        } else if (severityStr == "error") {
            error.severity = CompilerError::Severity::Error;
        } else {
            error.severity = CompilerError::Severity::Warning;
        }
        
        error.message = match.captured(5).trimmed();
        return true;
    }

    return false;
}

bool PapyrusCompiler::parseWithStrategy4(const QString& line, CompilerError& error)
{
    if (!line.contains("error", Qt::CaseInsensitive) && !line.contains("warning", Qt::CaseInsensitive) && !line.contains("fatal", Qt::CaseInsensitive)) {
        return false;
    }

    if (line.contains("Error", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Error;
    } else if (line.contains("Fatal", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Fatal;
    } else if (line.contains("Warning", Qt::CaseInsensitive)) {
        error.severity = CompilerError::Severity::Warning;
    } else {
        return false;
    }

    QRegularExpression fileRe(R"([\w\-\.]+\.psc)");
    QRegularExpressionMatch fileMatch = fileRe.match(line);
    if (fileMatch.hasMatch()) {
        error.file = fileMatch.captured(0);
    }

    QRegularExpression lineRe(R"(line\s+(\d+)|\((\d+)\))");
    QRegularExpressionMatch lineMatch = lineRe.match(line);
    if (lineMatch.hasMatch()) {
        QString numStr = lineMatch.captured(1).isEmpty() ? lineMatch.captured(2) : lineMatch.captured(1);
        if (!numStr.isEmpty()) {
            error.line = numStr.toInt();
        }
    }

    error.message = line.trimmed();
    return true;
}

QString PapyrusCompiler::extractLineNumber(const QString& line, int startPos) const
{
    QString numStr;
    for (int i = startPos; i < line.length(); i++) {
        if (line[i].isDigit()) {
            numStr += line[i];
        } else {
            break;
        }
    }
    return numStr;
}

bool PapyrusCompiler::parseCompileOrder(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("PapyrusCompiler: Cannot open compile_order.txt: %1").arg(filePath));
        return false;
    }

    dependencyGraph.clear();
    compilationOrder.clear();
    hasCircularDeps = false;
    circularDepList.clear();

    QByteArray lineData;
    while (!file.atEnd()) {
        lineData = file.readLine().trimmed();
        if (lineData.isEmpty() || lineData.startsWith("//")) {
            continue;
        }

        QString scriptPath = QString::fromLocal8Bit(lineData);
        QString scriptName = extractScriptName(scriptPath);

        if (scriptName.isEmpty()) {
            continue;
        }

        ScriptDependency dep;
        dep.scriptName = scriptName;
        dependencyGraph[scriptName] = dep;
    }

    file.close();

    for (int i = 0; i < dependencyGraph.size(); i++) {
        QString currentScript = dependencyGraph.keys()[i];
        QString currentPath = getScriptPathForName(currentScript, filePath);
        
        QFile scriptFile(currentPath);
        if (!scriptFile.exists()) {
            LOG_WARNING(QString("PapyrusCompiler: Script file not found for %1 at %2").arg(currentScript, currentPath));
            continue;
        }

        if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            LOG_WARNING(QString("PapyrusCompiler: Cannot read script file: %1").arg(currentPath));
            continue;
        }

        QSet<QString> referencedScripts;
        QByteArray scriptContent;
        while (!scriptFile.atEnd()) {
            QByteArray line = scriptFile.readLine();
            scriptContent += line;
            
            QString lineStr = QString::fromUtf8(line);
            
            if (lineStr.contains("Import ", Qt::CaseInsensitive) || 
                lineStr.contains("import ", Qt::CaseInsensitive)) {
                QStringList parts = lineStr.split(' ', Qt::SkipEmptyParts);
                for (const auto& part : parts) {
                    if (part.startsWith("\"") && part.endsWith("\"")) {
                        QString importName = part.mid(1, part.length() - 2);
                        referencedScripts.insert(importName);
                    } else if (part.contains(".psc", Qt::CaseInsensitive)) {
                        QString scriptRef = part;
                        scriptRef.replace(".psc", "");
                        referencedScripts.insert(scriptRef);
                    }
                }
            }
            
            if (lineStr.contains("Function ", Qt::CaseInsensitive) ||
                lineStr.contains("Function\n", Qt::CaseInsensitive) ||
                lineStr.contains("Function\r", Qt::CaseInsensitive)) {
                // Skip function bodies - dependencies are at top level
            }
        }
        scriptFile.close();

        QVector<QString> deps;
        for (const auto& ref : referencedScripts) {
            if (dependencyGraph.contains(ref)) {
                deps.append(ref);
            } else {
                LOG_WARNING(QString("PapyrusCompiler: Dependency %1 not found in compile_order.txt").arg(ref));
            }
        }

        dependencyGraph[currentScript].dependencies = deps;
    }

        auto result = topologicalSort();
        compilationOrder = result.order;
        hasCircularDeps = result.hasCycle;
    
        if (hasCircularDeps) {
        circularDepList = getCircularDependencies();
        LOG_ERROR(QString("PapyrusCompiler: Circular dependencies detected involving %1 scripts").arg(circularDepList.size()));
        return false;
    }

    LOG_INFO(QString("PapyrusCompiler: Parsed %1 scripts with dependency order").arg(dependencyGraph.size()));
    return true;
}

QString PapyrusCompiler::getScriptPathForName(const QString& scriptName, const QString& compileOrderPath)
{
    QFileInfo coInfo(compileOrderPath);
    QString coDir = coInfo.path();
    
    QString searchPaths[] = {
        coDir + "/scripts/" + scriptName + ".psc",
        coDir + "/Scripts/" + scriptName + ".psc",
        compileOrderPath.left(compileOrderPath.lastIndexOf('/')) + "/scripts/" + scriptName + ".psc"
    };
    
    for (const auto& path : searchPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    return coDir + "/scripts/" + scriptName + ".psc";
}

QVector<QString> PapyrusCompiler::getCompilationOrder() const
{
    return compilationOrder;
}

bool PapyrusCompiler::hasCircularDependencies() const
{
    return hasCircularDeps;
}

QVector<QString> PapyrusCompiler::getCircularDependencies() const
{
    return circularDepList;
}

bool PapyrusCompiler::batchCompile(const QVector<QString>& scriptPaths)
{
    if (compilerPath.isEmpty()) {
        emit compilationError("Compiler path not set");
        return false;
    }

    if (scriptPaths.isEmpty()) {
        emit compilationError("No scripts to compile");
        return false;
    }

    clearDependencies();
    
    for (int i = 0; i < scriptPaths.size(); i++) {
        const QString& scriptPath = scriptPaths[i];
        
        if (!QFile::exists(scriptPath)) {
            emit compilationError(QString("Script not found: %1").arg(scriptPath));
            continue;
        }

        ScriptDependency dep;
        dep.scriptName = extractScriptName(scriptPath);
        
        QFile scriptFile(scriptPath);
        if (scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray scriptContent;
            while (!scriptFile.atEnd()) {
                QByteArray line = scriptFile.readLine();
                scriptContent += line;
                
                QString lineStr = QString::fromUtf8(line);
                
                if (lineStr.contains("import ", Qt::CaseInsensitive) ||
                    lineStr.contains("Import ", Qt::CaseInsensitive)) {
                    QStringList parts = lineStr.split(' ', Qt::SkipEmptyParts);
                    for (const auto& part : parts) {
                        if (part.startsWith("\"") && part.endsWith("\"")) {
                            QString importName = part.mid(1, part.length() - 2);
                            dep.dependencies.append(importName);
                        } else if (part.contains(".psc", Qt::CaseInsensitive)) {
                            QString scriptRef = part;
                            scriptRef.replace(".psc", "");
                            dep.dependencies.append(scriptRef);
                        }
                    }
                }
            }
            scriptFile.close();
        }

        dependencyGraph[dep.scriptName] = dep;
    }

        auto result = topologicalSort();
        compilationOrder = result.order;
    
        if (result.hasCycle) {
        hasCircularDeps = true;
        circularDepList = getCircularDependencies();
        emit compilationError(QString("Circular dependencies detected: %1").arg(circularDepList.join(", ")));
        return false;
    }

    hasCircularDeps = false;
    emit compilationStarted();

    int total = compilationOrder.size();
    int completed = 0;
    bool allSuccess = true;

    for (const auto& scriptName : compilationOrder) {
        QString scriptPath;
        for (const auto& sp : scriptPaths) {
            if (extractScriptName(sp) == scriptName) {
                scriptPath = sp;
                break;
            }
        }

        if (scriptPath.isEmpty()) {
            emit compilationError(QString("Script not found: %1").arg(scriptName));
            allSuccess = false;
            continue;
        }

        emit batchProgress(completed, total);
        
        if (!compileScript(scriptPath)) {
            allSuccess = false;
        }
        
        completed++;
        emit batchProgress(completed, total);
    }

    emit batchProgress(total, total);
    emit compilationFinished(allSuccess);
    
    return allSuccess;
}

void PapyrusCompiler::clearDependencies()
{
    dependencyGraph.clear();
    compilationOrder.clear();
    hasCircularDeps = false;
    circularDepList.clear();
}

bool PapyrusCompiler::compileScript(const QString& scriptPath)
{
    if (scriptPath.isEmpty()) {
        return false;
    }

    output.clear();
    lastErrors.clear();

    QStringList arguments;
    arguments << escapePath(scriptPath);

    if (!outputPath.isEmpty()) {
        arguments << "-output" << escapePath(outputPath);
    }

    if (!modFile.isEmpty()) {
        arguments << "-mod" << escapePath(modFile);
    }

    arguments << getCompilerFlags();

    for (const auto& includePath : includePaths) {
        arguments << "-i" << escapePath(includePath);
    }

    for (const auto& lib : libraries) {
        arguments << "-lib" << escapePath(lib);
    }

    LOG_INFO(QString("PapyrusCompiler: Compiling %1").arg(scriptPath));

    process.start(compilerPath, arguments);

    if (!process.waitForStarted()) {
        emit compilationError(QString("Failed to start compiler: %1").arg(process.errorString()));
        return false;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        emit compilationError("Compilation timed out after 30 seconds");
        return false;
    }

    parseCompilerOutput(output);
    
    bool success = (process.exitCode() == 0);
    if (!success) {
        LOG_ERROR(QString("PapyrusCompiler: Failed to compile %1").arg(scriptPath));
    } else {
        LOG_INFO(QString("PapyrusCompiler: Successfully compiled %1").arg(scriptPath));
    }

    return success;
}

PapyrusCompiler::TopologicalResult PapyrusCompiler::topologicalSort() const
{
    QMap<QString, int> inDegree;
    QSet<QString> allNodes;

    for (auto it = dependencyGraph.begin(); it != dependencyGraph.end(); ++it) {
        allNodes.insert(it.key());
        if (!inDegree.contains(it.key())) {
            inDegree[it.key()] = 0;
        }
        
        for (const auto& dep : it.value().dependencies) {
            if (dependencyGraph.contains(dep)) {
                inDegree[it.key()]++;
                allNodes.insert(dep);
            }
        }
    }

    for (const auto& node : allNodes) {
        if (!inDegree.contains(node)) {
            inDegree[node] = 0;
        }
    }

    QVector<QString> queue;
    for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
        if (it.value() == 0) {
            queue.append(it.key());
        }
    }

    QVector<QString> sorted;
    int front = 0;
    
    while (front < queue.size()) {
        QString current = queue[front++];
        sorted.append(current);

        for (auto it = dependencyGraph.begin(); it != dependencyGraph.end(); ++it) {
            if (it.value().dependencies.contains(current)) {
                inDegree[it.key()]--;
                if (inDegree[it.key()] == 0) {
                    queue.append(it.key());
                }
            }
        }
    }

    if (sorted.size() != dependencyGraph.size()) {
        return { {}, true };
    }

    return { sorted, false };
}

bool PapyrusCompiler::detectCycleDFS(const QString& node, QSet<QString>& visited, QSet<QString>& recStack, QVector<QString>& cyclePath) const
{
    visited.insert(node);
    recStack.insert(node);
    cyclePath.append(node);

    if (dependencyGraph.contains(node)) {
        for (const auto& dep : dependencyGraph[node].dependencies) {
            if (!dependencyGraph.contains(dep)) {
                continue;
            }

            if (!visited.contains(dep)) {
                if (detectCycleDFS(dep, visited, recStack, cyclePath)) {
                    return true;
                }
            } else if (recStack.contains(dep)) {
                cyclePath.append(dep);
                return true;
            }
        }
    }

    cyclePath.removeLast();
    recStack.remove(node);
    return false;
}

QString PapyrusCompiler::extractScriptName(const QString& path) const
{
    QFileInfo fileInfo(path);
    QString name = fileInfo.baseName();
    return name;
}
