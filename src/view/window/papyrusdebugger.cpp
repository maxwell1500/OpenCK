#include "papyrusdebugger.hpp"
#include "../../model/world/data.hpp"
#include "papyruscompiler.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include <QTextBlock>
#include <QScrollBar>
#include <QFont>
#include <QApplication>

PapyrusDebugger::PapyrusDebugger(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mState(DebugState::Stopped),
      mCurrentLine(0),
      mCurrentFunctionStart(0),
      mCurrentFunctionEnd(0)
{
    setWindowTitle("Papyrus Script Debugger");
    resize(1000, 700);

    // Setup visual formats
    mCurrentLineFormat.setBackground(QColor(255, 255, 0, 80));
    mCurrentLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    mBreakpointFormat.setBackground(QColor(255, 80, 80, 120));
    mBreakpointFormat.setProperty(QTextFormat::FullWidthSelection, true);

    mNormalFormat.setProperty(QTextFormat::FullWidthSelection, true);

    setupUI();
    loadScripts();
}

PapyrusDebugger::~PapyrusDebugger()
{
}

void PapyrusDebugger::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top panel: Script list and breakpoints
    QHBoxLayout* topLayout = new QHBoxLayout();

    // Left: Script list
    QGroupBox* scriptGroup = new QGroupBox("Scripts");
    QVBoxLayout* scriptLayout = new QVBoxLayout(scriptGroup);
    mScriptList = new QTreeWidget();
    mScriptList->setHeaderLabels(QStringList() << "Script Name" << "Path");
    mScriptList->setColumnWidth(0, 200);
    mScriptList->setColumnWidth(1, 400);
    connect(mScriptList, &QTreeWidget::itemDoubleClicked, this, &PapyrusDebugger::selectScript);
    scriptLayout->addWidget(mScriptList);
    topLayout->addWidget(scriptGroup, 1);

    // Right: Breakpoints
    QGroupBox* breakpointGroup = new QGroupBox("Breakpoints");
    QVBoxLayout* breakpointLayout = new QVBoxLayout(breakpointGroup);
    mBreakpointList = new QListWidget();
    connect(mBreakpointList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;
        int line = item->data(Qt::UserRole).toInt();
        mBreakpoints.remove(line);
        updateBreakpointList();
        highlightBreakpoints();
    });
    breakpointLayout->addWidget(mBreakpointList);
    topLayout->addWidget(breakpointGroup, 1);

    mainLayout->addLayout(topLayout, 2);

    // Middle: Code editor
    QGroupBox* codeGroup = new QGroupBox("Script Code");
    QVBoxLayout* codeLayout = new QVBoxLayout(codeGroup);
    mCodeEditor = new QTextEdit();
    mCodeEditor->setReadOnly(true);
    QFont monoFont("Consolas", 10);
    monoFont.setFixedPitch(true);
    mCodeEditor->setFont(monoFont);
    mCodeEditor->setLineWrapMode(QTextEdit::NoWrap);
    mCodeEditor->setPlainText("// Select a script to view its code\n// Double-click on a line to toggle breakpoints");
    codeLayout->addWidget(mCodeEditor);
    mainLayout->addWidget(codeGroup, 3);

    // Bottom panel: Variables and call stack
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    // Left: Variables
    QGroupBox* variablesGroup = new QGroupBox("Variables");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesGroup);
    mVariableList = new QTreeWidget();
    mVariableList->setHeaderLabels(QStringList() << "Name" << "Type" << "Value");
    mVariableList->setColumnWidth(0, 150);
    mVariableList->setColumnWidth(1, 100);
    mVariableList->setColumnWidth(2, 200);
    variablesLayout->addWidget(mVariableList);
    bottomLayout->addWidget(variablesGroup, 1);

    // Right: Call stack
    QGroupBox* callStackGroup = new QGroupBox("Call Stack");
    QVBoxLayout* callStackLayout = new QVBoxLayout(callStackGroup);
    mCallStackList = new QTreeWidget();
    mCallStackList->setHeaderLabels(QStringList() << "Function" << "File" << "Line");
    callStackLayout->addWidget(mCallStackList);
    bottomLayout->addWidget(callStackGroup, 1);

    mainLayout->addLayout(bottomLayout, 2);

    // Status bar
    QHBoxLayout* statusLayout = new QHBoxLayout();
    mStatusLabel = new QLabel("Ready");
    mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    statusLayout->addWidget(mStatusLabel);

    // Debug controls
    QPushButton* runButton = new QPushButton("Run");
    QPushButton* stopButton = new QPushButton("Stop");
    QPushButton* stepOverButton = new QPushButton("Step Over");
    QPushButton* stepIntoButton = new QPushButton("Step Into");
    QPushButton* stepOutButton = new QPushButton("Step Out");

    runButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 6px 16px; }");
    stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 6px 16px; }");

    connect(runButton, &QPushButton::clicked, this, &PapyrusDebugger::runScript);
    connect(stopButton, &QPushButton::clicked, this, &PapyrusDebugger::stopScript);
    connect(stepOverButton, &QPushButton::clicked, this, &PapyrusDebugger::stepOver);
    connect(stepIntoButton, &QPushButton::clicked, this, &PapyrusDebugger::stepInto);
    connect(stepOutButton, &QPushButton::clicked, this, &PapyrusDebugger::stepOut);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(stepOverButton);
    buttonLayout->addWidget(stepIntoButton);
    buttonLayout->addWidget(stepOutButton);
    statusLayout->addLayout(buttonLayout);

    mainLayout->addLayout(statusLayout);
}

void PapyrusDebugger::loadScripts()
{
    mScriptList->clear();

    // Find all .psc files in the game directory
    QString scriptPath = "C:/XboxGames/Starfield/Content/Data/Scripts";
    QDir dir(scriptPath);

    if (!dir.exists())
    {
        // Try alternative path
        scriptPath = "C:/Program Files/Steam/steamapps/common/Starfield/Content/Data/Scripts";
        dir.setPath(scriptPath);
    }

    if (!dir.exists())
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, "Select Papyrus Scripts Directory");
        if (!selectedDir.isEmpty())
        {
            dir.setPath(selectedDir);
        }
    }

    if (dir.exists())
    {
        QStringList filters;
        filters << "*.psc";
        QStringList files = dir.entryList(filters, QDir::Files);

        for (const QString& file : files)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(mScriptList);
            item->setText(0, file);
            item->setText(1, dir.filePath(file));
        }
    }

    LOG_INFO(QString("Loaded %1 Papyrus scripts").arg(mScriptList->topLevelItemCount()));
}

void PapyrusDebugger::selectScript(QTreeWidgetItem* item, int column)
{
    if (!item) return;

    QString filePath = item->text(1);
    loadScriptFile(filePath);
}

void PapyrusDebugger::loadScriptFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Error", QString("Cannot open script file: %1").arg(filePath));
        return;
    }

    QTextStream in(&file);
    mScriptContent = in.readAll();
    file.close();

    mScriptLines = mScriptContent.split('\n');
    mCodeEditor->setPlainText(mScriptContent);
    mCurrentScript = filePath;

    mBreakpoints.clear();
    mBreakpointList->clear();
    mFunctions.clear();
    mVariables.clear();
    mFunctionMap.clear();
    mCallStack.clear();
    mCurrentFunction.clear();

    parseScript(mScriptContent);

    LOG_INFO(QString("Loaded script: %1 (%2 lines, %3 functions, %4 variables)")
        .arg(QFileInfo(filePath).fileName())
        .arg(mScriptLines.size())
        .arg(mFunctions.size())
        .arg(mVariables.size()));
}

void PapyrusDebugger::parseScript(const QString& content)
{
    mFunctions.clear();
    mVariables.clear();
    mFunctionMap.clear();

    for (int i = 0; i < mScriptLines.size(); i++)
    {
        QString line = mScriptLines[i].trimmed();

        // Skip comments and blank lines
        if (line.startsWith("//") || line.isEmpty())
            continue;

        // Look for Function/Event declarations
        QRegularExpression funcRegex(
            R"((?:Function|Event)\s+(\w+)\s*\(([^)]*)\))",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch funcMatch = funcRegex.match(line);
        if (funcMatch.hasMatch())
        {
            FunctionInfo func;
            func.name = funcMatch.captured(1);
            func.startLine = i;
            func.endLine = findEndOfFunction(i);

            // Parse parameters
            QString paramStr = funcMatch.captured(2).trimmed();
            if (!paramStr.isEmpty())
            {
                QStringList params = paramStr.split(',');
                for (const QString& param : params)
                {
                    QString trimmed = param.trimmed();
                    // Extract just the parameter name (last word)
                    QStringList parts = trimmed.split(' ');
                    if (!parts.isEmpty())
                        func.parameterNames.append(parts.last());
                }
            }

            mFunctions.append(func);
            mFunctionMap[func.name] = func;
        }

        // Look for Property declarations
        QRegularExpression propRegex(
            R"((?:Property)\s+(\w+)\s+(\w+)(?:\s*=\s*(.+))?)",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch propMatch = propRegex.match(line);
        if (propMatch.hasMatch())
        {
            VariableInfo var;
            var.name = propMatch.captured(1);
            var.type = propMatch.captured(2);
            var.value = propMatch.captured(3).trimmed();
            if (var.value.isEmpty())
                var.value = "None";
            var.line = i;
            mVariables.append(var);
        }

        // Look for Var declarations (local variables inside functions)
        QRegularExpression varRegex(
            R"((?:Var)\s+(\w+)\s+(\w+))",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch varMatch = varRegex.match(line);
        if (varMatch.hasMatch() && !line.toLower().startsWith("property"))
        {
            VariableInfo var;
            var.name = varMatch.captured(2);
            var.type = varMatch.captured(1);
            var.value = "Uninitialized";
            var.line = i;
            mVariables.append(var);
        }
    }

    updateVariables();
}

void PapyrusDebugger::runScript()
{
    if (mCurrentScript.isEmpty())
    {
        QMessageBox::information(this, "No Script", "Please load a script first.");
        return;
    }

    if (mScriptContent.isEmpty() || mScriptLines.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Script is empty.");
        return;
    }

    // Validate script structure
    if (!mScriptContent.contains("ScriptName") || !mScriptContent.contains("EndScript"))
    {
        QMessageBox::warning(this, "Error",
            "Invalid Papyrus script structure.\n"
            "Script must contain ScriptName and EndScript keywords.");
        return;
    }

    // Check if we have at least one function
    if (mFunctions.isEmpty())
    {
        QMessageBox::warning(this, "Error",
            "No functions found in script.\n"
            "Script must contain at least one Function or Event.");
        return;
    }

    // Try to compile
    mStatusLabel->setText("Compiling...");
    QApplication::processEvents();

    PapyrusCompiler compiler;
    compiler.setScriptPath(mCurrentScript);

    // Set output path to temp directory
    QString tempDir = QDir::tempPath() + "/papyrus_debug";
    QDir().mkpath(tempDir);
    compiler.setOutputPath(tempDir);

    bool compileSuccess = compiler.compile();
    QVector<CompilerError> errors = compiler.getLastErrors();

    if (!compileSuccess)
    {
        // Show errors in the variable list
        mVariableList->clear();
        for (const CompilerError& err : errors)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(mVariableList);
            QString severity;
            switch (err.severity)
            {
                case CompilerError::Severity::Error: severity = "ERROR"; break;
                case CompilerError::Severity::Warning: severity = "WARNING"; break;
                case CompilerError::Severity::Fatal: severity = "FATAL"; break;
            }
            item->setText(0, QString("[%1] Line %2").arg(severity).arg(err.line));
            item->setText(1, "Compiler");
            item->setText(2, err.message);
            item->setForeground(0, err.severity == CompilerError::Severity::Error ? Qt::red : Qt::darkYellow);
        }

        mStatusLabel->setText("Compilation Failed");
        mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: red;");
        LOG_INFO("Compilation failed with " + QString::number(errors.size()) + " errors");
        return;
    }

    // Compilation succeeded - simulate running
    mState = DebugState::Running;
    mCallStack.clear();

    // Find the script name
    QString scriptName = QFileInfo(mCurrentScript).baseName();

    // Find OnInit or first function
    QString entryFunc;
    if (mFunctionMap.contains("OnInit"))
        entryFunc = "OnInit";
    else if (mFunctionMap.contains("OnUpdate"))
        entryFunc = "OnUpdate";
    else if (!mFunctions.isEmpty())
        entryFunc = mFunctions.first().name;

    if (!entryFunc.isEmpty())
    {
        mCurrentFunction = entryFunc;
        const FunctionInfo& func = mFunctionMap[entryFunc];
        mCurrentFunctionStart = func.startLine;
        mCurrentFunctionEnd = func.endLine;

        // Push entry function onto call stack
        mCallStack.append({entryFunc, -1});

        // Add simulated event chain
        if (entryFunc == "OnInit")
        {
            mCallStack.append({scriptName + ".OnInit() -> " + scriptName + ".OnUpdate()", -1});
        }

        // Start at first executable line in the function
        mCurrentLine = findNextExecutableLine(func.startLine);
    }
    else
    {
        mCurrentLine = findNextExecutableLine(0);
    }

    mState = DebugState::Stepping;
    mStatusLabel->setText("Running - " + mCurrentFunction);
    mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: green;");

    highlightCurrentLine(mCurrentLine);
    updateCallStack();
    updateVariables();

    LOG_INFO(QString("Script started: %1 (at line %2)").arg(scriptName).arg(mCurrentLine + 1));
}

void PapyrusDebugger::stopScript()
{
    mState = DebugState::Stopped;
    mCurrentLine = 0;
    mCallStack.clear();
    mCurrentFunction.clear();
    mCurrentFunctionStart = 0;
    mCurrentFunctionEnd = 0;

    clearHighlighting();
    mVariableList->clear();
    mCallStackList->clear();
    mStatusLabel->setText("Stopped");
    mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: gray;");

    LOG_INFO("Script stopped");
}

void PapyrusDebugger::stepOver()
{
    if (mState == DebugState::Stopped)
    {
        QMessageBox::information(this, "Not Running", "No script is running. Click Run first.");
        return;
    }

    if (mCurrentScript.isEmpty() || mScriptLines.isEmpty())
        return;

    int nextLine = findNextExecutableLine(mCurrentLine + 1);

    // If we're past the current function, pop the call stack
    if (nextLine > mCurrentFunctionEnd || nextLine < 0)
    {
        if (mCallStack.size() > 1)
        {
            popCallStack();
            const CallFrame& frame = mCallStack.last();
            if (mFunctionMap.contains(frame.functionName))
            {
                const FunctionInfo& func = mFunctionMap[frame.functionName];
                mCurrentFunction = frame.functionName;
                mCurrentFunctionStart = func.startLine;
                mCurrentFunctionEnd = func.endLine;
            }
            // Move to next line after the call
            nextLine = findNextExecutableLine(mCurrentLine + 1);
            if (nextLine < 0 || nextLine > mCurrentFunctionEnd)
            {
                // End of function, continue stepping
                nextLine = mCurrentLine + 1;
                while (nextLine < mScriptLines.size() && isCommentOrBlank(mScriptLines[nextLine]))
                    nextLine++;
            }
        }
        else
        {
            stopScript();
            return;
        }
    }

    mCurrentLine = nextLine;

    // Check for breakpoints
    if (mBreakpoints.contains(mCurrentLine))
    {
        mState = DebugState::Paused;
        mStatusLabel->setText("Paused at breakpoint (line " + QString::number(mCurrentLine + 1) + ")");
        mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: orange;");
    }
    else
    {
        mStatusLabel->setText("Stepping - " + mCurrentFunction);
        mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: blue;");
    }

    highlightCurrentLine(mCurrentLine);
    updateVariables();
}

void PapyrusDebugger::stepInto()
{
    if (mState == DebugState::Stopped)
    {
        QMessageBox::information(this, "Not Running", "No script is running. Click Run first.");
        return;
    }

    if (mCurrentScript.isEmpty() || mScriptLines.isEmpty())
        return;

    QString line = mScriptLines[mCurrentLine].trimmed();

    // Check if current line is a function call
    if (isFunctionCallLine(line))
    {
        QString callFunc = extractCallFunctionName(line);
        if (!callFunc.isEmpty() && mFunctionMap.contains(callFunc))
        {
            // Push current position onto call stack
            pushCallStack(mCurrentFunction, mCurrentLine);

            // Enter the called function
            const FunctionInfo& func = mFunctionMap[callFunc];
            mCurrentFunction = callFunc;
            mCurrentFunctionStart = func.startLine;
            mCurrentFunctionEnd = func.endLine;
            mCurrentLine = findNextExecutableLine(func.startLine);

            highlightCurrentLine(mCurrentLine);
            updateCallStack();
            updateVariables();
            mStatusLabel->setText("Stepping Into - " + mCurrentFunction);
            mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: blue;");
            return;
        }
    }

    // Not a function call, behave like stepOver
    stepOver();
}

void PapyrusDebugger::stepOut()
{
    if (mState == DebugState::Stopped)
    {
        QMessageBox::information(this, "Not Running", "No script is running. Click Run first.");
        return;
    }

    if (mCallStack.size() <= 1)
    {
        // At top level, stop execution
        stopScript();
        return;
    }

    // Pop the current function
    popCallStack();

    const CallFrame& frame = mCallStack.last();
    if (mFunctionMap.contains(frame.functionName))
    {
        const FunctionInfo& func = mFunctionMap[frame.functionName];
        mCurrentFunction = frame.functionName;
        mCurrentFunctionStart = func.startLine;
        mCurrentFunctionEnd = func.endLine;

        // Move to the line after the call that invoked current function
        mCurrentLine = findNextExecutableLine(frame.returnLine + 1);
        if (mCurrentLine < 0 || mCurrentLine > mCurrentFunctionEnd)
        {
            mCurrentLine = findNextExecutableLine(mCurrentFunctionStart);
        }
    }
    else
    {
        mCurrentFunction = frame.functionName;
        mCurrentLine = findNextExecutableLine(0);
    }

    highlightCurrentLine(mCurrentLine);
    updateCallStack();
    updateVariables();
    mStatusLabel->setText("Stepping Out - " + mCurrentFunction);
    mStatusLabel->setStyleSheet("font-weight: bold; padding: 4px; color: blue;");
}

void PapyrusDebugger::toggleBreakpoint()
{
    if (mCurrentScript.isEmpty())
    {
        QMessageBox::information(this, "No Script", "Please load a script first.");
        return;
    }

    // Get current line from cursor position
    QTextCursor cursor = mCodeEditor->textCursor();
    int line = cursor.blockNumber();

    if (line < 0 || line >= mScriptLines.size())
        return;

    // Don't allow breakpoints on comments or blank lines
    QString text = mScriptLines[line].trimmed();
    if (text.isEmpty() || text.startsWith("//"))
    {
        QMessageBox::information(this, "Invalid Line",
            "Cannot set breakpoint on comments or empty lines.");
        return;
    }

    if (mBreakpoints.contains(line))
    {
        mBreakpoints.remove(line);
    }
    else
    {
        mBreakpoints.insert(line);
    }

    updateBreakpointList();
    highlightBreakpoints();

    LOG_INFO(QString("Breakpoint %1 at line %2")
        .arg(mBreakpoints.contains(line) ? "set" : "removed")
        .arg(line + 1));
}

void PapyrusDebugger::updateVariables()
{
    if (mCurrentScript.isEmpty())
        return;

    mVariableList->clear();

    // Add all property/variable declarations from the script
    for (const VariableInfo& var : mVariables)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(mVariableList);
        item->setText(0, var.name);
        item->setText(1, var.type);
        item->setText(2, var.value);
    }

    // Add simulated runtime variables based on current function parameters
    if (!mCurrentFunction.isEmpty() && mFunctionMap.contains(mCurrentFunction))
    {
        const FunctionInfo& func = mFunctionMap[mCurrentFunction];
        for (const QString& param : func.parameterNames)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(mVariableList);
            item->setText(0, param);
            item->setText(1, "Parameter");
            item->setText(2, "<runtime>");
            item->setForeground(0, Qt::darkBlue);
        }
    }

    // Show local Var declarations from the current function scope
    if (!mCurrentFunction.isEmpty() && mFunctionMap.contains(mCurrentFunction))
    {
        const FunctionInfo& func = mFunctionMap[mCurrentFunction];
        for (int i = func.startLine; i <= func.endLine && i < mScriptLines.size(); i++)
        {
            QString line = mScriptLines[i].trimmed();
            QRegularExpression varRegex(
                R"((?:Var)\s+(\w+)\s+(\w+))",
                QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = varRegex.match(line);
            if (match.hasMatch() && !line.toLower().startsWith("property"))
            {
                // Check if already added as a property
                QString varName = match.captured(2);
                bool alreadyAdded = false;
                for (int j = 0; j < mVariableList->topLevelItemCount(); j++)
                {
                    if (mVariableList->topLevelItem(j)->text(0) == varName)
                    {
                        alreadyAdded = true;
                        break;
                    }
                }
                if (!alreadyAdded)
                {
                    QTreeWidgetItem* item = new QTreeWidgetItem(mVariableList);
                    item->setText(0, varName);
                    item->setText(1, match.captured(1));
                    item->setText(2, "Uninitialized");
                }
            }
        }
    }
}

void PapyrusDebugger::updateCallStack()
{
    mCallStackList->clear();

    QString fileName = QFileInfo(mCurrentScript).fileName();

    // Display call stack in reverse order (most recent first)
    for (int i = mCallStack.size() - 1; i >= 0; i--)
    {
        const CallFrame& frame = mCallStack[i];
        QTreeWidgetItem* item = new QTreeWidgetItem(mCallStackList);
        item->setText(0, frame.functionName);
        item->setText(1, fileName);

        // Find the line number for this function
        if (mFunctionMap.contains(frame.functionName))
        {
            item->setText(2, QString::number(mFunctionMap[frame.functionName].startLine + 1));
        }
        else
        {
            item->setText(2, "-");
        }

        // Highlight current function
        if (i == mCallStack.size() - 1)
        {
            QFont font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
            item->setForeground(0, Qt::darkBlue);
        }
    }

    // If call stack is empty but we have a current function, show it
    if (mCallStack.isEmpty() && !mCurrentFunction.isEmpty())
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(mCallStackList);
        item->setText(0, mCurrentFunction);
        item->setText(1, fileName);

        if (mFunctionMap.contains(mCurrentFunction))
        {
            item->setText(2, QString::number(mFunctionMap[mCurrentFunction].startLine + 1));
        }

        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
        item->setForeground(0, Qt::darkBlue);
    }
}

void PapyrusDebugger::highlightCurrentLine(int line)
{
    clearHighlighting();

    if (line < 0 || line >= mScriptLines.size())
        return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    // Add breakpoint highlights first
    for (int bp : mBreakpoints)
    {
        QTextBlock block = mCodeEditor->document()->findBlockByNumber(bp);
        if (block.isValid())
        {
            QTextEdit::ExtraSelection sel;
            sel.format = mBreakpointFormat;
            sel.cursor = QTextCursor(block);
            sel.cursor.movePosition(QTextCursor::StartOfBlock);
            extraSelections.append(sel);
        }
    }

    // Add current line highlight on top
    QTextBlock block = mCodeEditor->document()->findBlockByNumber(line);
    if (block.isValid())
    {
        QTextEdit::ExtraSelection sel;
        sel.format = mCurrentLineFormat;
        sel.cursor = QTextCursor(block);
        sel.cursor.movePosition(QTextCursor::StartOfBlock);
        extraSelections.append(sel);

        // Scroll to make the line visible
        mCodeEditor->setTextCursor(sel.cursor);
        mCodeEditor->ensureCursorVisible();
    }

    mCodeEditor->setExtraSelections(extraSelections);
}

void PapyrusDebugger::highlightBreakpoints()
{
    if (mCurrentLine >= 0 && mState != DebugState::Stopped)
    {
        highlightCurrentLine(mCurrentLine);
        return;
    }

    // Just show breakpoint highlights
    QList<QTextEdit::ExtraSelection> extraSelections;

    for (int bp : mBreakpoints)
    {
        QTextBlock block = mCodeEditor->document()->findBlockByNumber(bp);
        if (block.isValid())
        {
            QTextEdit::ExtraSelection sel;
            sel.format = mBreakpointFormat;
            sel.cursor = QTextCursor(block);
            sel.cursor.movePosition(QTextCursor::StartOfBlock);
            extraSelections.append(sel);
        }
    }

    mCodeEditor->setExtraSelections(extraSelections);
}

void PapyrusDebugger::updateBreakpointList()
{
    mBreakpointList->clear();

    QList<int> sortedBreakpoints = mBreakpoints.values();
    std::sort(sortedBreakpoints.begin(), sortedBreakpoints.end());

    for (int line : sortedBreakpoints)
    {
        QString lineText;
        if (line >= 0 && line < mScriptLines.size())
        {
            lineText = mScriptLines[line].trimmed();
            if (lineText.length() > 60)
                lineText = lineText.left(57) + "...";
        }

        QListWidgetItem* item = new QListWidgetItem(
            QString("Line %1: %2").arg(line + 1).arg(lineText));
        item->setData(Qt::UserRole, line);
        item->setForeground(Qt::red);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        mBreakpointList->addItem(item);
    }
}

void PapyrusDebugger::clearHighlighting()
{
    mCodeEditor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

int PapyrusDebugger::findNextExecutableLine(int fromLine)
{
    for (int i = fromLine; i < mScriptLines.size(); i++)
    {
        if (!isCommentOrBlank(mScriptLines[i]))
            return i;
    }
    return -1;
}

int PapyrusDebugger::findEndOfFunction(int startLine)
{
    // Find the EndFunction/EndEvent matching the function at startLine
    int depth = 0;
    for (int i = startLine; i < mScriptLines.size(); i++)
    {
        QString line = mScriptLines[i].trimmed().toLower();

        if (line.startsWith("function ") || line.startsWith("event "))
            depth++;
        else if (line.startsWith("endfunction") || line.startsWith("endevent"))
        {
            depth--;
            if (depth == 0)
                return i;
        }
    }
    return mScriptLines.size() - 1;
}

bool PapyrusDebugger::isFunctionCallLine(const QString& line)
{
    // Check if line contains a function call (identifier followed by parenthesis)
    QRegularExpression callRegex(R"(\w+\s*\()");
    return callRegex.match(line).hasMatch() &&
           !line.toLower().startsWith("function") &&
           !line.toLower().startsWith("event") &&
           !line.toLower().startsWith("if") &&
           !line.toLower().startsWith("while") &&
           !line.toLower().startsWith("for");
}

QString PapyrusDebugger::extractFunctionName(const QString& line)
{
    // Extract function name from a Function/Event declaration
    QRegularExpression funcRegex(
        R"((?:Function|Event)\s+(\w+))",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = funcRegex.match(line);
    if (match.hasMatch())
        return match.captured(1);
    return QString();
}

QString PapyrusDebugger::extractCallFunctionName(const QString& line)
{
    // Extract the function name being called
    QRegularExpression callRegex(R"((\w+)\s*\()");
    QRegularExpressionMatch match = callRegex.match(line);
    if (match.hasMatch())
    {
        QString name = match.captured(1);
        // Check if it's a known function in our parsed script
        if (mFunctionMap.contains(name))
            return name;
    }
    return QString();
}

bool PapyrusDebugger::isCommentOrBlank(const QString& line)
{
    QString trimmed = line.trimmed();
    return trimmed.isEmpty() || trimmed.startsWith("//");
}

void PapyrusDebugger::pushCallStack(const QString& funcName, int line)
{
    CallFrame frame;
    frame.functionName = funcName;
    frame.returnLine = line;
    mCallStack.append(frame);
}

void PapyrusDebugger::popCallStack()
{
    if (!mCallStack.isEmpty())
        mCallStack.removeLast();
}
