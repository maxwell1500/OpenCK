#include "scripteditorwidget.hpp"

#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QDir>
#include <QTextBlockUserData>
#include <QCompleter>
#include <QStringListModel>
#include <QMouseEvent>
#include <QStack>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>
#include <QRegularExpression>
#include <QKeyEvent>

#include "papyruscompiler.hpp"
#include "../../model/tools/papyrusprevalidator.hpp"
#include "../../model/tools/papyrustypechecker.hpp"
#include "logger.hpp"

#include <QToolTip>
#include <QHelpEvent>

class BlockData : public QTextBlockUserData
{
public:
    bool hasFold = false;
    bool folded = false;
    int endBlockNumber = -1;
    QString foldKeyword;

    bool hasError = false;
    QString errorMessage;

    bool hasTypeError = false;
    QString typeErrorMessage;
};

static QStringList buildCompleterWords()
{
    QStringList words;
    words << "ScriptName" << "EndScript" << "Function" << "EndFunction"
          << "Event" << "EndEvent" << "Property" << "EndProperty" << "Variable"
          << "EndVariable" << "Bool" << "Int" << "Float" << "String" << "Form"
          << "FormList" << "ObjectReference" << "Actor" << "Quest" << "Package"
          << "Spell" << "EffectShader" << "Keyword" << "True" << "False"
          << "None" << "Return" << "If" << "Else" << "ElseIf" << "EndIf"
          << "While" << "EndWhile" << "For" << "EndFor" << "Var" << "Const"
          << "Auto" << "Static" << "Global" << "Private" << "Native" << "Required"
          << "MustImplement" << "ShowInObjectMenu" << "AllowPlayerDialogue"
          << "BlockBegin" << "BlockEnd"
          << "ActorBase" << "Weapon" << "Armor" << "AlchemyItem" << "Ingredient"
          << "MiscItem" << "Book" << "Note" << "Apparatus" << "Ammo" << "Food"
          << "Key" << "Tool" << "SoulGem" << "ScrollItem"
          << "Enchantment" << "MagicEffect" << "Activator" << "Tree" << "Flora"
          << "Projectile" << "Shader" << "Texture" << "Sound" << "Music"
          << "AmbientSound" << "Water" << "Light" << "Cell" << "WorldSpace"
          << "InteriorCell" << "ExteriorCell" << "NavMesh"
          << "QuestStage" << "QuestObjective" << "QuestTopic"
          << "QuestTopicInfo" << "DialogueTopic" << "DialogueTopicInfo"
          << "Conversation" << "Topic" << "Info";
    words.removeDuplicates();
    return words;
}

class PapyrusHighlighter : public QSyntaxHighlighter
{
public:
    PapyrusHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent)
    {
        KeywordFormat.setForeground(Qt::blue);
        KeywordFormat.setFontWeight(QFont::Bold);

        ControlFlowFormat.setForeground(QColor(0, 0, 160));
        ControlFlowFormat.setFontWeight(QFont::Bold);

        TypeFormat.setForeground(Qt::darkCyan);
        TypeFormat.setFontWeight(QFont::Bold);

        StringFormat.setForeground(Qt::darkGreen);

        CommentFormat.setForeground(Qt::darkGray);
        CommentFormat.setFontItalic(true);

        NumberFormat.setForeground(Qt::darkYellow);

        keywordPatterns << "ScriptName" << "EndScript" << "Function" << "EndFunction"
                       << "Event" << "EndEvent" << "Property" << "EndProperty" << "Variable"
                       << "EndVariable" << "Bool" << "Int" << "Float" << "String" << "Form"
                       << "FormList" << "ObjectReference" << "Actor" << "Quest" << "Package"
                       << "Spell" << "EffectShader" << "Keyword" << "True" << "False"
                       << "None" << "Return" << "If" << "Else" << "ElseIf" << "EndIf"
                       << "While" << "EndWhile" << "For" << "EndFor" << "Var" << "Const"
                       << "Auto" << "Static" << "Global" << "Private" << "Native" << "Required"
                       << "MustImplement" << "ShowInObjectMenu" << "AllowPlayerDialogue"
                       << "BlockBegin" << "BlockEnd";

        typePatterns << "Form" << "FormList" << "ObjectReference" << "Actor"
                    << "Quest" << "Package" << "Spell" << "EffectShader" << "Keyword"
                    << "ActorBase" << "Weapon" << "Armor" << "AlchemyItem" << "Ingredient"
                    << "MiscItem" << "Book" << "Note" << "Apparatus" << "Ammo" << "Food"
                    << "Key" << "Tool" << "SoulGem" << "ScrollItem"
                    << "Enchantment" << "MagicEffect" << "Activator" << "Tree" << "Flora" << "Projectile"
                    << "Shader" << "Texture" << "Sound" << "Music"
                    << "AmbientSound" << "Water" << "Light" << "Cell" << "WorldSpace"
                    << "InteriorCell" << "ExteriorCell" << "NavMesh" << "Package"
                    << "Quest" << "QuestStage" << "QuestObjective" << "QuestTopic"
                    << "QuestTopicInfo" << "DialogueTopic" << "DialogueTopicInfo"
                    << "Conversation" << "Topic" << "Info";

        controlFlowPattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        controlFlowPattern.setPattern("\\b(if|elseif|else|endif|while|endwhile|for|endfor)\\b");
    }

protected:
    void highlightBlock(const QString& text) override
    {
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf("//");
        }

        while (startIndex >= 0) {
            int endIndex = text.indexOf("\n", startIndex);
            int commentLength;
            if (endIndex == -1) {
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + 1;
            }
            setFormat(startIndex, commentLength, CommentFormat);
            startIndex = text.indexOf("//", startIndex + commentLength);
        }

        setCurrentBlockState(0);
        startIndex = 0;
        while (startIndex < text.length()) {
            int quoteIndex = text.indexOf("\"", startIndex);
            if (quoteIndex == -1) break;
            int stringEnd = text.indexOf("\"", quoteIndex + 1);
            if (stringEnd == -1) {
                setFormat(quoteIndex, text.length() - quoteIndex, StringFormat);
                break;
            }
            setFormat(quoteIndex, stringEnd - quoteIndex + 1, StringFormat);
            startIndex = stringEnd + 1;
        }

        QRegularExpressionMatchIterator it = controlFlowPattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), ControlFlowFormat);
        }

        foreach (const QString& pattern, keywordPatterns) {
            int pos = 0;
            while ((pos = text.indexOf(pattern, pos)) != -1) {
                bool isWordStart = (pos == 0 || !text.at(pos - 1).isLetterOrNumber());
                bool isWordEnd = (pos + pattern.length() >= text.length() || !text.at(pos + pattern.length()).isLetterOrNumber());

                if (isWordStart && isWordEnd) {
                    setFormat(pos, pattern.length(), KeywordFormat);
                }
                pos += pattern.length();
            }
        }

        foreach (const QString& pattern, typePatterns) {
            int pos = 0;
            while ((pos = text.indexOf(pattern, pos)) != -1) {
                bool isWordStart = (pos == 0 || !text.at(pos - 1).isLetterOrNumber());
                bool isWordEnd = (pos + pattern.length() >= text.length() || !text.at(pos + pattern.length()).isLetterOrNumber());

                if (isWordStart && isWordEnd) {
                    setFormat(pos, pattern.length(), TypeFormat);
                }
                pos += pattern.length();
            }
        }
    }

private:
    QTextCharFormat KeywordFormat;
    QTextCharFormat ControlFlowFormat;
    QTextCharFormat TypeFormat;
    QTextCharFormat StringFormat;
    QTextCharFormat CommentFormat;
    QTextCharFormat NumberFormat;

    QStringList keywordPatterns;
    QStringList typePatterns;
    QRegularExpression controlFlowPattern;
};

ScriptEditorWidget::ScriptEditorWidget(QWidget* parent) :
    QWidget(parent),
    m_textEdit(nullptr),
    highlighter(nullptr),
    lineNumberWidget(nullptr),
    compiler(nullptr),
    completer(nullptr),
    modified(false),
    updatingFolds(false),
    m_referenceBrowser(nullptr),
    m_toggleBrowserBtn(nullptr),
    m_browserContainer(nullptr),
    m_consoleOutput(nullptr),
    m_splitter(nullptr),
    m_toggleConsoleBtn(nullptr),
    m_consoleContainer(nullptr),
    m_typeChecker(nullptr),
    m_toolTipHelper(nullptr)
{
    LOG_DEBUG("ScriptEditorWidget created");

    m_typeChecker = new PapyrusTypeChecker();

    compiler = new PapyrusCompiler(this);
    connect(compiler, &PapyrusCompiler::compilationStarted, this, &ScriptEditorWidget::compilationStarted);
    connect(compiler, &PapyrusCompiler::compilationFinished, this, [this](bool success) {
        logToConsole(QString("Compilation finished: %1").arg(success ? "success" : "failed"));
        if (!success) {
            auto errors = compiler->getLastErrors();
            for (const auto& err : errors) {
                QString severity;
                switch (err.severity) {
                    case CompilerError::Severity::Error: severity = "ERROR"; break;
                    case CompilerError::Severity::Warning: severity = "WARNING"; break;
                    case CompilerError::Severity::Fatal: severity = "FATAL"; break;
                }
                logErrorToConsole(QString("[%1:%2] %3: %4").arg(err.line).arg(err.column).arg(severity).arg(err.message));
            }
            if (!errors.isEmpty()) {
                gotoError(errors.first());
            }
        }
        emit compilationFinished(success);
    });
    connect(compiler, &PapyrusCompiler::logMessage, this, [this](const QString& msg, bool isError) {
        if (isError) {
            logErrorToConsole(msg.trimmed());
        } else {
            logToConsole(msg.trimmed());
        }
    });
    connect(compiler, &PapyrusCompiler::compilationError, this, [this](const QString& err) {
        logErrorToConsole(err);
    });

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Vertical, this);

    QWidget* editorArea = new QWidget(m_splitter);
    QHBoxLayout* editorLayout = new QHBoxLayout(editorArea);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    m_textEdit = new ScriptTextEdit(editorArea);
    m_textEdit->setTabStopDistance(40);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    editorLayout->addWidget(m_textEdit, 1);

    setupSyntaxHighlighter();
    setupFont();
    setupCompleter();

    lineNumberWidget = new LineNumberWidget(m_textEdit, this);

    connect(m_textEdit, &QPlainTextEdit::blockCountChanged, this, &ScriptEditorWidget::updateLineNumberWidth);
    connect(m_textEdit, &QPlainTextEdit::updateRequest, this, &ScriptEditorWidget::updateLineNumber);
    connect(m_textEdit, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        modified = true;
    });
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        modified = true;
        performCompletion();
        refreshTypeSquiggles();
    });

    connect(m_textEdit->document(), &QTextDocument::contentsChanged, this, &ScriptEditorWidget::updateFoldRegions);

    m_textEdit->setMouseTracking(true);
    m_textEdit->installEventFilter(this);

    m_toggleBrowserBtn = new QPushButton("<<", editorArea);
    m_toggleBrowserBtn->setFixedSize(20, 24);
    m_toggleBrowserBtn->setToolTip("Toggle Reference Browser");
    connect(m_toggleBrowserBtn, &QPushButton::clicked, this, &ScriptEditorWidget::toggleReferenceBrowser);
    editorLayout->addWidget(m_toggleBrowserBtn, 0);

    m_toggleConsoleBtn = new QPushButton("v", editorArea);
    m_toggleConsoleBtn->setFixedSize(20, 24);
    m_toggleConsoleBtn->setToolTip("Toggle Console");
    connect(m_toggleConsoleBtn, &QPushButton::clicked, this, &ScriptEditorWidget::toggleConsole);
    editorLayout->addWidget(m_toggleConsoleBtn, 0);

    m_browserContainer = new QWidget(editorArea);
    QVBoxLayout* browserLayout = new QVBoxLayout(m_browserContainer);
    browserLayout->setContentsMargins(0, 0, 0, 0);
    browserLayout->setSpacing(0);

    m_referenceBrowser = new QTreeWidget(m_browserContainer);
    m_referenceBrowser->setHeaderHidden(true);
    m_referenceBrowser->setRootIsDecorated(true);
    connect(m_referenceBrowser, &QTreeWidget::itemDoubleClicked, this, &ScriptEditorWidget::onBrowserItemDoubleClicked);
    browserLayout->addWidget(m_referenceBrowser);

    m_browserContainer->setFixedWidth(200);
    editorLayout->addWidget(m_browserContainer, 0);

    setupReferenceBrowser();

    m_splitter->addWidget(editorArea);

    setupConsole();

    m_splitter->addWidget(m_consoleContainer);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setSizes({600, 100});

    outerLayout->addWidget(m_splitter);

    updateLineNumberWidth(0);
}

ScriptEditorWidget::~ScriptEditorWidget()
{
    LOG_DEBUG("ScriptEditorWidget destroyed");
    delete highlighter;
    delete lineNumberWidget;
    delete m_typeChecker;
}

int ScriptEditorWidget::lineNumberWidth() const
{
    int digits = 1;
    int max = qMax(1, m_textEdit->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = FoldMarkerWidth + 3 + m_textEdit->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void ScriptEditorWidget::updateLineNumberWidth(int)
{
    m_textEdit->setViewportMargins(lineNumberWidth(), 0, 0, 0);
    if (lineNumberWidget) {
        QRect cr = m_textEdit->contentsRect();
        lineNumberWidget->setGeometry(QRect(cr.left(), cr.top(), lineNumberWidth(), cr.height()));
    }
}

void ScriptEditorWidget::updateLineNumber(const QRect& rect, int dy)
{
    if (dy)
        lineNumberWidget->scroll(0, dy);
    else
        lineNumberWidget->update(0, rect.y(), lineNumberWidget->width(), rect.height());

    if (rect.contains(m_textEdit->viewport()->rect()))
        updateLineNumberWidth(0);
}

void ScriptEditorWidget::lineNumberPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberWidget);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = m_textEdit->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)m_textEdit->blockBoundingGeometry(block).translated(m_textEdit->contentOffset()).top();
    int bottom = top + (int)m_textEdit->blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int fmWidth = FoldMarkerWidth - 2;
            int numX = FoldMarkerWidth;
            int numWidth = lineNumberWidget->width() - FoldMarkerWidth - 4;

            painter.setPen(QColor(120, 120, 120));
            painter.drawText(numX, top, numWidth, m_textEdit->fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));

            BlockData* data = dynamic_cast<BlockData*>(block.userData());
            if (data && data->hasFold && data->endBlockNumber >= 0) {
                QRect fmRect(numX - fmWidth - 1, top, fmWidth, m_textEdit->fontMetrics().height());
                painter.setPen(QColor(100, 100, 100));
                if (data->folded) {
                    painter.drawText(fmRect, Qt::AlignCenter, QStringLiteral("\u25B6"));
                } else {
                    painter.drawText(fmRect, Qt::AlignCenter, QStringLiteral("\u25BC"));
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)m_textEdit->blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void ScriptEditorWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (lineNumberWidget && m_textEdit) {
        QRect cr = m_textEdit->contentsRect();
        lineNumberWidget->setGeometry(QRect(cr.left(), cr.top(), lineNumberWidth(), cr.height()));
    }
}

bool ScriptEditorWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_textEdit) {
        if (event->type() == QEvent::Resize) {
            if (lineNumberWidget) {
                QRect cr = m_textEdit->contentsRect();
                lineNumberWidget->setGeometry(QRect(cr.left(), cr.top(), lineNumberWidth(), cr.height()));
            }
        } else if (event->type() == QEvent::ToolTip) {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            QTextCursor cursor = m_textEdit->cursorForPosition(helpEvent->pos());
            QString word;
            QTextCursor wc = cursor;
            wc.select(QTextCursor::WordUnderCursor);
            word = wc.selectedText();
            if (!word.isEmpty() && m_typeChecker && m_typeChecker->hasVariable(word)) {
                auto ti = m_typeChecker->variableType(word);
                QString typeStr = ti.isArray ? QString("%1[]").arg(ti.name) : ti.name;
                QString tip = QString("%1 : %2").arg(word, typeStr);
                QTextBlock block = cursor.block();
                BlockData* data = dynamic_cast<BlockData*>(block.userData());
                if (data && data->hasTypeError) {
                    tip += QString("\n\u26A0 %1").arg(data->typeErrorMessage);
                }
                QToolTip::showText(helpEvent->globalPos(), tip, m_textEdit);
            } else {
                QToolTip::hideText();
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ScriptEditorWidget::loadScript(const QString& fileName)
{
    LOG_INFO(QString("Loading script: %1").arg(fileName));

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open script file: %1").arg(fileName));
        QMessageBox::critical(this, "Error",
            QString("Failed to open script file:\n%1").arg(fileName));
        return;
    }

    QTextStream stream(&file);
    m_textEdit->setPlainText(stream.readAll());
    file.close();

    currentFileName = fileName;
    modified = false;

    updateFoldRegions();
    refreshTypeSquiggles();

    LOG_INFO("Script loaded successfully");
}

void ScriptEditorWidget::saveScript()
{
    if (currentFileName.isEmpty()) {
        return;
    }

    LOG_INFO(QString("Saving script: %1").arg(currentFileName));

    QFile file(currentFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to save script file: %1").arg(currentFileName));
        QMessageBox::critical(this, "Error",
            QString("Failed to save script file:\n%1").arg(currentFileName));
        return;
    }

    QTextStream stream(&file);
    stream << m_textEdit->toPlainText();
    file.close();

    modified = false;
    LOG_INFO("Script saved successfully");
}

bool ScriptEditorWidget::compile(const QString& outputPath)
{
    LOG_INFO(QString("Compiling script: %1").arg(currentFileName));

    if (currentFileName.isEmpty()) {
        LOG_ERROR("No script file loaded");
        logErrorToConsole("No script file loaded");
        return false;
    }

    QString scriptContent = m_textEdit->toPlainText();

    if (scriptContent.isEmpty()) {
        LOG_WARNING("Script is empty");
        logErrorToConsole("Script is empty");
        return false;
    }

    if (!scriptContent.contains("ScriptName") || !scriptContent.contains("EndScript")) {
        LOG_ERROR("Invalid Papyrus script structure (missing ScriptName/EndScript)");
        logErrorToConsole("Invalid Papyrus script structure (missing ScriptName/EndScript)");
        QMessageBox::critical(this, "Compilation Error",
            "Invalid Papyrus script structure.\n"
            "Script must contain ScriptName and EndScript keywords.");
        return false;
    }

    logToConsole(QString("Pre-compilation validation: %1").arg(currentFileName));
    
    auto preErrors = PapyrusPreValidator::validate(scriptContent);
    
    if (!preErrors.isEmpty()) {
        QString errorSummary;
        for (const auto& error : preErrors) {
            QString errorType;
            switch (error.type) {
                case PapyrusValidationError::Type::SyntaxError:
                    errorType = "Syntax";
                    break;
                case PapyrusValidationError::Type::StructureError:
                    errorType = "Structure";
                    break;
                case PapyrusValidationError::Type::TypeMismatch:
                    errorType = "Type";
                    break;
                case PapyrusValidationError::Type::UndefinedVariable:
                    errorType = "Undefined";
                    break;
                case PapyrusValidationError::Type::UnusedVariable:
                    errorType = "Unused";
                    break;
                default:
                    errorType = "Error";
            }
            errorSummary += QString("[%1] Line %2: %3\n")
                .arg(errorType)
                .arg(error.line)
                .arg(error.message);
        }

        logErrorToConsole(errorSummary);
        QMessageBox::critical(this, "Pre-Compilation Validation",
            QString("Script validation failed:\n\n%1").arg(errorSummary));
        return false;
    }

    logToConsole(QString("Compilation started: %1").arg(currentFileName));

    compiler->setScriptPath(currentFileName);
    compiler->setOutputPath(outputPath);

    if (!compilerOutputDir.isEmpty()) {
        QDir dir(outputPath);
        compiler->addIncludePath(dir.absolutePath());
    }

    bool result = compiler->compile();

    if (!result) {
        QFileInfo fileInfo(outputPath);
        QDir outDir = fileInfo.dir();
        if (!outDir.exists()) outDir.mkpath(".");

        QFile srcFile(currentFileName);
        QFile dstFile(outputPath);
        if (srcFile.open(QIODevice::ReadOnly) && dstFile.open(QIODevice::WriteOnly)) {
            dstFile.write(srcFile.readAll());
            srcFile.close();
            dstFile.close();
        }
    }

    return result;
}

void ScriptEditorWidget::setCompilerPath(const QString& path)
{
    compiler->setCompilerPath(path);
}

void ScriptEditorWidget::setCompilerOutputDir(const QString& path)
{
    compilerOutputDir = path;
    compiler->setOutputPath(path);
}

void ScriptEditorWidget::addCompilerIncludePath(const QString& path)
{
    compiler->addIncludePath(path);
}

void ScriptEditorWidget::gotoError(const CompilerError& error)
{
    if (error.line <= 0) return;

    QTextCursor cursor = m_textEdit->textCursor();
    int targetLine = error.line - 1;

    QTextBlock block = m_textEdit->document()->findBlockByNumber(targetLine);
    if (block.isValid()) {
        cursor.setPosition(block.position());
        m_textEdit->setTextCursor(cursor);
        highlightErrorLine(targetLine);
        m_textEdit->ensureCursorVisible();
        m_textEdit->setFocus();
    }
}

void ScriptEditorWidget::highlightErrorLine(int lineNumber)
{
    QTextBlock block = m_textEdit->document()->findBlockByNumber(lineNumber);
    if (!block.isValid()) return;

    BlockData* data = dynamic_cast<BlockData*>(block.userData());
    if (!data) {
        data = new BlockData();
        block.setUserData(data);
    }
    data->hasError = true;
    data->errorMessage = "Error";

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection sel;
    sel.format.setBackground(QColor(255, 100, 100, 80));
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    sel.cursor = QTextCursor(block);
    sel.cursor.movePosition(QTextCursor::StartOfBlock);
    extraSelections.append(sel);
    m_textEdit->setExtraSelections(extraSelections);
}

void ScriptEditorWidget::setupSyntaxHighlighter()
{
    highlighter = new PapyrusHighlighter(m_textEdit->document());
}

void ScriptEditorWidget::refreshTypeSquiggles()
{
    if (!m_textEdit || !m_typeChecker) return;

    QString content = m_textEdit->toPlainText();
    *m_typeChecker = PapyrusPreValidator::buildTypeChecker(content);

    QList<QTextEdit::ExtraSelection> selections;
    QTextCharFormat fmt;
    fmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    fmt.setUnderlineColor(QColor(190, 80, 200));
    fmt.setForeground(Qt::magenta);

    const auto typedErrors = m_typeChecker->typedErrors();
    for (const auto& te : typedErrors) {
        int lineIdx = te.line - 1;
        if (lineIdx < 0) continue;
        QTextBlock block = m_textEdit->document()->findBlockByNumber(lineIdx);
        if (!block.isValid()) continue;
        BlockData* data = dynamic_cast<BlockData*>(block.userData());
        if (!data) {
            data = new BlockData();
            block.setUserData(data);
        }
        data->hasTypeError = true;
        data->typeErrorMessage = te.message;
        QTextEdit::ExtraSelection sel;
        sel.format = fmt;
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = QTextCursor(block);
        sel.cursor.movePosition(QTextCursor::StartOfBlock);
        selections.append(sel);
    }

    for (QTextBlock b = m_textEdit->document()->begin(); b.isValid(); b = b.next()) {
        BlockData* data = dynamic_cast<BlockData*>(b.userData());
        if (data && !data->hasTypeError) {
            data->hasTypeError = false;
            data->typeErrorMessage.clear();
        }
    }

    if (!selections.isEmpty()) {
        m_textEdit->setExtraSelections(selections);
    }
}

void ScriptEditorWidget::setupFont()
{
    QFont font("Consolas", 10);
    font.setFixedPitch(true);
    m_textEdit->setFont(font);
}

void ScriptEditorWidget::setupCompleter()
{
    completer = new QCompleter(this);
    QStringListModel* model = new QStringListModel(buildCompleterWords(), completer);
    completer->setModel(model);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setMaxVisibleItems(10);
    completer->setWidget(m_textEdit);

    connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, [this](const QString& completion) {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        cursor.insertText(completion);
        m_textEdit->setTextCursor(cursor);
    });
}

void ScriptEditorWidget::setupReferenceBrowser()
{
    auto addCategory = [this](const QString& name, const QStringList& items) {
        QTreeWidgetItem* category = new QTreeWidgetItem(m_referenceBrowser, {name});
        category->setFlags(category->flags() & ~Qt::ItemIsSelectable);
        QFont font = category->font(0);
        font.setBold(true);
        category->setFont(0, font);
        for (const QString& text : items) {
            QTreeWidgetItem* item = new QTreeWidgetItem(category, {text});
            item->setData(0, Qt::UserRole, text);
        }
    };

    addCategory("Functions", {
        "Game.GetPlayer() -> Actor",
        "Game.GetForm(int) -> Form",
        "Debug.Notification(string)",
        "Debug.MessageBox(string)",
        "Utility.Wait(float)",
        "Utility.RandomInt(int, int) -> int",
        "Math.Abs(float) -> float",
        "Actor.GetActorBase() -> ActorBase",
        "ObjectReference.GetPositionX() -> float",
        "ObjectReference.GetPositionY() -> float",
        "ObjectReference.GetPositionZ() -> float",
        "ObjectReference.SetPosition(float, float, float)",
        "ObjectReference.Enable()",
        "ObjectReference.Disable()",
        "ObjectReference.Delete()",
        "Quest.SetStage(int)",
        "Quest.GetStage() -> int",
        "Quest.IsCompleted() -> bool"
    });

    addCategory("Events", {
        "OnInit()",
        "OnActivate(ObjectReference akActionRef)",
        "OnTriggerEnter(ObjectReference akActionRef)",
        "OnTriggerLeave(ObjectReference akActionRef)",
        "OnHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)",
        "OnDeath(Actor akKiller)",
        "OnRead()",
        "OnEquipped(Actor akActor)",
        "OnUnequipped(Actor akActor)",
        "OnItemAdded(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)",
        "OnItemRemoved(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akDestContainer)",
        "OnSleepStart(float afSleepStartTime, float afDesiredSleepEndTime)",
        "OnSleepStop(bool abInterrupted)",
        "OnUpdate()"
    });

    addCategory("Properties", {
        "Auto",
        "AutoReadOnly",
        "Hidden",
        "Conditional"
    });

    m_referenceBrowser->expandAll();
}

void ScriptEditorWidget::setupConsole()
{
    m_consoleContainer = new QWidget(m_splitter);

    QVBoxLayout* consoleLayout = new QVBoxLayout(m_consoleContainer);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    consoleLayout->setSpacing(0);

    m_consoleOutput = new QPlainTextEdit(m_consoleContainer);
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_consoleOutput->setStyleSheet(
        "QPlainTextEdit { background-color: #1a1a1a; color: #00ff00; border: none; }");

    QFont consoleFont("Consolas", 9);
    consoleFont.setFixedPitch(true);
    m_consoleOutput->setFont(consoleFont);

    consoleLayout->addWidget(m_consoleOutput);
}

void ScriptEditorWidget::toggleReferenceBrowser()
{
    bool currentlyVisible = m_browserContainer->isVisible();
    m_browserContainer->setVisible(!currentlyVisible);
    m_toggleBrowserBtn->setText(m_browserContainer->isVisible() ? "<<" : ">>");
}

void ScriptEditorWidget::toggleConsole()
{
    bool currentlyVisible = m_consoleContainer->isVisible();
    m_consoleContainer->setVisible(!currentlyVisible);
    m_toggleConsoleBtn->setText(m_consoleContainer->isVisible() ? "v" : "^");
}

void ScriptEditorWidget::logToConsole(const QString& message)
{
    if (!m_consoleOutput) return;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_consoleOutput->appendHtml(
        QString("<span style='color:#00ff00;'>[%1] %2</span>")
            .arg(timestamp, message.toHtmlEscaped()));
}

void ScriptEditorWidget::logErrorToConsole(const QString& message)
{
    if (!m_consoleOutput) return;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_consoleOutput->appendHtml(
        QString("<span style='color:#ff4444;'>[%1] %2</span>")
            .arg(timestamp, message.toHtmlEscaped()));
}

void ScriptEditorWidget::clearConsole()
{
    if (m_consoleOutput) {
        m_consoleOutput->clear();
    }
}

void ScriptEditorWidget::onBrowserItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item || !item->parent())
        return;

    QString text = item->data(0, Qt::UserRole).toString();
    if (text.isEmpty())
        return;

    QTextCursor cursor = m_textEdit->textCursor();
    cursor.insertText(text);
    m_textEdit->setTextCursor(cursor);
    m_textEdit->setFocus();
}

void ScriptEditorWidget::performCompletion()
{
    if (!completer) return;

    QTextCursor cursor = m_textEdit->textCursor();
    QTextCursor wordCursor = cursor;
    wordCursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    QString prefix = wordCursor.selectedText();

    if (prefix.length() < 2) {
        completer->popup()->hide();
        return;
    }

    QStringList baseWords = buildCompleterWords();

    QString lineText = cursor.block().text().left(cursor.positionInBlock());
    static QRegularExpression assignCtxRe(R"((\w+)\s*=\s*$)");
    auto am = assignCtxRe.match(lineText);
    if (am.hasMatch() && m_typeChecker && m_typeChecker->hasVariable(am.captured(1))) {
        auto varType = m_typeChecker->variableType(am.captured(1));
        QStringList typed;
        const auto& fns = m_typeChecker->functions();
        for (auto it = fns.begin(); it != fns.end(); ++it) {
            if (it.value().returnType.name == varType.name &&
                it.value().returnType.isArray == varType.isArray) {
                typed << it.key();
            }
        }
        if (!typed.isEmpty()) {
            baseWords += typed;
            baseWords.removeDuplicates();
        }
    }

    if (m_typeChecker) {
        const auto& syms = m_typeChecker->symbols();
        for (auto it = syms.begin(); it != syms.end(); ++it) {
            baseWords << it.key();
        }
        baseWords.removeDuplicates();
    }

    QStringListModel* model = qobject_cast<QStringListModel*>(completer->model());
    if (model) {
        model->setStringList(baseWords);
    }

    if (prefix != completer->completionPrefix()) {
        completer->setCompletionPrefix(prefix);
    }

    if (completer->completionCount() > 0) {
        QRect cr = m_textEdit->cursorRect();
        cr.setWidth(completer->popup()->sizeHintForColumn(0)
                    + completer->popup()->verticalScrollBar()->sizeHint().width() + 4);
        completer->complete(cr);
    } else {
        completer->popup()->hide();
    }
}

bool ScriptEditorWidget::findFoldStart(const QString& text, QString& keyword) const
{
    static const QStringList startKeywords = {"ScriptName", "Function", "Event", "Property", "If", "While", "For"};

    int bestPos = -1;
    for (const QString& kw : startKeywords) {
        int pos = 0;
        while ((pos = text.indexOf(kw, pos)) != -1) {
            bool wordStart = (pos == 0 || !text[pos - 1].isLetterOrNumber());
            bool wordEnd = (pos + kw.length() >= text.length() || !text[pos + kw.length()].isLetterOrNumber());
            if (wordStart && wordEnd) {
                if (bestPos < 0 || pos < bestPos) {
                    bestPos = pos;
                    keyword = kw;
                }
                break;
            }
            pos += kw.length();
        }
    }
    return bestPos >= 0;
}

bool ScriptEditorWidget::findFoldEnd(const QString& text, QString& keyword) const
{
    static const QStringList endKeywords = {"EndScript", "EndFunction", "EndEvent", "EndProperty", "EndIf", "EndWhile", "EndFor"};

    int bestPos = -1;
    for (const QString& kw : endKeywords) {
        int pos = 0;
        while ((pos = text.indexOf(kw, pos)) != -1) {
            bool wordStart = (pos == 0 || !text[pos - 1].isLetterOrNumber());
            bool wordEnd = (pos + kw.length() >= text.length() || !text[pos + kw.length()].isLetterOrNumber());
            if (wordStart && wordEnd) {
                if (bestPos < 0 || pos < bestPos) {
                    bestPos = pos;
                    keyword = kw;
                }
                break;
            }
            pos += kw.length();
        }
    }
    return bestPos >= 0;
}

QString ScriptEditorWidget::foldEndToStart(const QString& endKeyword) const
{
    if (endKeyword == "EndScript")   return "ScriptName";
    if (endKeyword == "EndFunction") return "Function";
    if (endKeyword == "EndEvent")    return "Event";
    if (endKeyword == "EndProperty") return "Property";
    if (endKeyword == "EndIf")       return "If";
    if (endKeyword == "EndWhile")    return "While";
    if (endKeyword == "EndFor")      return "For";
    return QString();
}

void ScriptEditorWidget::updateFoldRegions()
{
    if (updatingFolds) return;
    updatingFolds = true;

    QTextDocument* doc = m_textEdit->document();

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        BlockData* data = dynamic_cast<BlockData*>(block.userData());
        if (data) {
            data->hasFold = false;
            data->endBlockNumber = -1;
            data->folded = false;
            data->foldKeyword.clear();
        }
    }

    QStack<QPair<int, QString>> stack;

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QString text = block.text();

        int commentPos = text.indexOf("//");
        if (commentPos >= 0) {
            text = text.left(commentPos);
        }

        QString startKeyword;
        if (findFoldStart(text, startKeyword)) {
            stack.push(qMakePair(block.blockNumber(), startKeyword));
        }

        QString endKeyword;
        if (findFoldEnd(text, endKeyword)) {
            QString matchingStart = foldEndToStart(endKeyword);
            if (!matchingStart.isEmpty()) {
                for (int i = stack.size() - 1; i >= 0; --i) {
                    if (stack[i].second == matchingStart) {
                        int startBlockNum = stack[i].first;
                        stack.remove(i);

                        QTextBlock startBlock = doc->findBlockByNumber(startBlockNum);
                        BlockData* data = dynamic_cast<BlockData*>(startBlock.userData());
                        if (!data) {
                            data = new BlockData();
                            startBlock.setUserData(data);
                        }
                        data->hasFold = true;
                        data->endBlockNumber = block.blockNumber();
                        data->foldKeyword = matchingStart;
                        break;
                    }
                }
            }
        }
    }

    recalcVisibility();
    lineNumberWidget->update();
    updatingFolds = false;
}

void ScriptEditorWidget::recalcVisibility()
{
    QTextBlock block = m_textEdit->document()->begin();
    QStack<int> foldEnds;

    while (block.isValid()) {
        BlockData* data = dynamic_cast<BlockData*>(block.userData());

        while (!foldEnds.isEmpty() && foldEnds.top() < block.blockNumber()) {
            foldEnds.pop();
        }

        bool hidden = !foldEnds.isEmpty();
        block.setVisible(!hidden);

        if (data && data->hasFold && data->folded && data->endBlockNumber >= 0) {
            foldEnds.push(data->endBlockNumber);
        }

        block = block.next();
    }
}

void ScriptEditorWidget::toggleFold(QTextBlock& block)
{
    BlockData* data = dynamic_cast<BlockData*>(block.userData());
    if (!data || !data->hasFold || data->endBlockNumber < 0) return;

    data->folded = !data->folded;

    recalcVisibility();

    lineNumberWidget->update();
    updateLineNumberWidth(0);
}

void LineNumberWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->x() < ScriptEditorWidget::FoldMarkerWidth) {
        QTextBlock block = mTextEdit->firstVisibleBlock();
        int top = (int)mTextEdit->blockBoundingGeometry(block).translated(mTextEdit->contentOffset()).top();
        int bottom = top + (int)mTextEdit->blockBoundingRect(block).height();

        while (block.isValid() && top <= event->y()) {
            if (block.isVisible() && event->y() < bottom) {
                BlockData* data = dynamic_cast<BlockData*>(block.userData());
                if (data && data->hasFold && data->endBlockNumber >= 0) {
                    mEditor->toggleFold(block);
                }
                return;
            }

            block = block.next();
            top = bottom;
            bottom = top + (int)mTextEdit->blockBoundingRect(block).height();
        }
        return;
    }

    QWidget::mousePressEvent(event);
}

QString ScriptTextEdit::leadingWhitespace(const QString& line) const
{
    QString ws;
    for (int i = 0; i < line.length(); ++i) {
        if (line.at(i).isSpace()) {
            ws.append(line.at(i));
        } else {
            break;
        }
    }
    return ws;
}

bool ScriptTextEdit::isControlFlowBlockStart(const QString& line) const
{
    static const QRegularExpression re("\\b(if|while|for)\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(line);
    return m.hasMatch();
}

bool ScriptTextEdit::isControlFlowBlockEnd(const QString& line) const
{
    static const QRegularExpression re("\\b(endif|endwhile|endfor)\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(line);
    return m.hasMatch();
}

void ScriptTextEdit::keyPressEvent(QKeyEvent* event)
{
    const int indentSize = 4;
    const QString indentUnit = QString(" ").repeated(indentSize);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        QTextBlock block = cursor.block();
        QString currentText = block.text();
        QString ws = leadingWhitespace(currentText);

        bool wasBlockStart = isControlFlowBlockStart(currentText);

        QPlainTextEdit::keyPressEvent(event);

        if (wasBlockStart) {
            cursor.insertText(ws + indentUnit);
            setTextCursor(cursor);
        } else {
            cursor.insertText(ws);
            setTextCursor(cursor);
        }
        return;
    }

    if (event->key() == Qt::Key_BraceRight || event->text() == "}") {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Tab) {
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();
            QTextBlock startBlock = document()->findBlock(start);
            QTextBlock endBlock = document()->findBlock(end);
            cursor.beginEditBlock();
            for (QTextBlock b = startBlock; b.isValid() && b.blockNumber() <= endBlock.blockNumber(); b = b.next()) {
                cursor.setPosition(b.position());
                cursor.insertText(indentUnit);
            }
            cursor.endEditBlock();
            setTextCursor(cursor);
        } else {
            cursor.insertText(indentUnit);
            setTextCursor(cursor);
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);

    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    QString text = block.text();

    QString trimmed = text.trimmed();
    if (isControlFlowBlockEnd(trimmed) || trimmed.compare("else", Qt::CaseInsensitive) == 0 ||
        trimmed.compare("elseif", Qt::CaseInsensitive) == 0) {
        QString ws = leadingWhitespace(text);
        if (ws.length() >= indentSize) {
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, indentSize);
            cursor.removeSelectedText();
            cursor.endEditBlock();
        }
    }
}
