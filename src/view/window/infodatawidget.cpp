#include "infodatawidget.hpp"
#include "../libs/files/esm/inforecord.hpp"
#include "../libs/files/audio/fuzparser.hpp"
#include "../libs/components/formcomponents.hpp"
#include "papyruscompiler.hpp"
#include "logger.hpp"

#include <QComboBox>
#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>
#  pragma comment(lib, "winmm.lib")
#endif

namespace openck {

namespace {

const QStringList& conditionFunctions()
{
    static const QStringList f = {
        QStringLiteral("GetIsAliasRef"),
        QStringLiteral("GetStage"),
        QStringLiteral("GetQuestDone"),
        QStringLiteral("GetIsClass"),
        QStringLiteral("GetIsRace"),
        QStringLiteral("GetLevel"),
        QStringLiteral("GetSex"),
        QStringLiteral("GetIsDead"),
        QStringLiteral("GetGlobalValue"),
        QStringLiteral("GetScriptVariable")
    };
    return f;
}

const QStringList& comparisonOperators()
{
    static const QStringList o = {
        QStringLiteral("=="),
        QStringLiteral("!="),
        QStringLiteral(">"),
        QStringLiteral("<"),
        QStringLiteral(">="),
        QStringLiteral("<=")
    };
    return o;
}

} // namespace

InfoDataWidget::InfoDataWidget(void* recordPtr, FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
    , m_condTable(nullptr)
    , m_voiceEdit(nullptr)
    , m_fragmentEdit(nullptr)
{
    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        mainLayout->addWidget(new QLabel(QStringLiteral("No record data available"), this));
        return;
    }

    auto* rec = static_cast<InfoRecord*>(m_recordPtr);

    auto* infoGroup = new QGroupBox(QStringLiteral("Info Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* responseEdit = new QLineEdit(infoGroup);
    responseEdit->setText(rec->responseText);
    infoForm->addRow(QStringLiteral("Response Text:"), responseEdit);
    connect(responseEdit, &QLineEdit::textChanged, this,
        [rec](const QString& t) { rec->responseText = t; });

    auto* targetEdit = new QLineEdit(infoGroup);
    targetEdit->setText(QStringLiteral("0x%1").arg(rec->targetId, 8, 16, QChar('0')));
    infoForm->addRow(QStringLiteral("Target ID:"), targetEdit);
    connect(targetEdit, &QLineEdit::editingFinished, this,
        [rec, targetEdit]() {
            bool ok = false;
            quint32 v = targetEdit->text().toUInt(&ok, 16);
            if (ok) rec->targetId = v;
            else    targetEdit->setText(QStringLiteral("0x%1").arg(rec->targetId, 8, 16, QChar('0')));
        });

    mainLayout->addWidget(infoGroup);

    // --- 9.4 Conditional response editing ---
    auto* condGroup = new QGroupBox(QStringLiteral("Conditions"), this);
    auto* condLayout = new QVBoxLayout(condGroup);

    m_condTable = new QTableWidget(0, 4, condGroup);
    m_condTable->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("Function")
                      << QStringLiteral("Operator")
                      << QStringLiteral("Value")
                      << QStringLiteral("AND/OR"));
    m_condTable->horizontalHeader()->setStretchLastSection(true);
    m_condTable->verticalHeader()->setVisible(false);
    condLayout->addWidget(m_condTable);

    auto* condBtnLayout = new QHBoxLayout();
    auto* addCondBtn = new QPushButton(QStringLiteral("Add"), condGroup);
    auto* removeCondBtn = new QPushButton(QStringLiteral("Remove"), condGroup);
    condBtnLayout->addWidget(addCondBtn);
    condBtnLayout->addWidget(removeCondBtn);
    condBtnLayout->addStretch();
    condLayout->addLayout(condBtnLayout);
    mainLayout->addWidget(condGroup);

    populateConditions();

    connect(addCondBtn, &QPushButton::clicked, this, &InfoDataWidget::onAddCondition);
    connect(removeCondBtn, &QPushButton::clicked, this, &InfoDataWidget::onRemoveCondition);
    connect(m_condTable, &QTableWidget::cellChanged, this, &InfoDataWidget::onConditionChanged);

    // --- 9.5 Voice file association ---
    auto* voiceGroup = new QGroupBox(QStringLiteral("Voice File"), this);
    auto* voiceLayout = new QVBoxLayout(voiceGroup);

    auto* voiceRow = new QHBoxLayout();
    m_voiceEdit = new QLineEdit(voiceGroup);
    m_voiceEdit->setText(rec->voiceFile);
    voiceRow->addWidget(m_voiceEdit, 1);
    auto* browseBtn = new QPushButton(QStringLiteral("Browse..."), voiceGroup);
    auto* playBtn = new QPushButton(QStringLiteral("Play"), voiceGroup);
    auto* recordBtn = new QPushButton(QStringLiteral("Record"), voiceGroup);
    recordBtn->setEnabled(false); // stub
    voiceRow->addWidget(browseBtn);
    voiceRow->addWidget(playBtn);
    voiceRow->addWidget(recordBtn);
    voiceLayout->addLayout(voiceRow);

    mainLayout->addWidget(voiceGroup);

    connect(m_voiceEdit, &QLineEdit::textChanged, this,
        [rec](const QString& t) { rec->voiceFile = t; });
    connect(browseBtn, &QPushButton::clicked, this, &InfoDataWidget::onBrowseVoiceFile);
    connect(playBtn, &QPushButton::clicked, this, &InfoDataWidget::onPlayVoiceFile);

    // --- Script fragment support ---
    auto* fragGroup = new QGroupBox(QStringLiteral("Script Fragment"), this);
    auto* fragLayout = new QVBoxLayout(fragGroup);
    m_fragmentEdit = new QPlainTextEdit(fragGroup);
    m_fragmentEdit->setPlainText(rec->scriptFragment);
    m_fragmentEdit->setPlaceholderText(QStringLiteral("Papyrus fragment executed when this response is selected..."));
    fragLayout->addWidget(m_fragmentEdit);

    auto* fragBtnRow = new QHBoxLayout();
    auto* compileBtn = new QPushButton(QStringLiteral("Compile Fragment"), fragGroup);
    fragBtnRow->addWidget(compileBtn);
    fragBtnRow->addStretch();
    fragLayout->addLayout(fragBtnRow);

    mainLayout->addWidget(fragGroup);

    connect(m_fragmentEdit, &QPlainTextEdit::textChanged, this,
        [this, rec]() { rec->scriptFragment = m_fragmentEdit->toPlainText(); });
    connect(compileBtn, &QPushButton::clicked, this, &InfoDataWidget::onCompileFragment);
}

InfoDataWidget::~InfoDataWidget() = default;

void InfoDataWidget::populateConditions()
{
    if (!m_recordPtr || !m_condTable) return;
    auto* rec = static_cast<InfoRecord*>(m_recordPtr);

    m_condTable->blockSignals(true);
    m_condTable->setRowCount(0);

    for (int i = 0; i < rec->conditions.size(); ++i)
    {
        const auto& c = rec->conditions.at(i);

        m_condTable->insertRow(i);

        auto* fnCombo = new QComboBox(m_condTable);
        fnCombo->addItems(conditionFunctions());
        int fnIdx = conditionFunctions().indexOf(c.function);
        if (fnIdx >= 0) fnCombo->setCurrentIndex(fnIdx);
        m_condTable->setCellWidget(i, 0, fnCombo);
        connect(fnCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, i]() { onConditionChanged(); });

        auto* opCombo = new QComboBox(m_condTable);
        opCombo->addItems(comparisonOperators());
        int opIdx = comparisonOperators().indexOf(c.comparison);
        if (opIdx >= 0) opCombo->setCurrentIndex(opIdx);
        m_condTable->setCellWidget(i, 1, opCombo);
        connect(opCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, i]() { onConditionChanged(); });

        auto* valItem = new QTableWidgetItem(c.value.toString());
        m_condTable->setItem(i, 2, valItem);

        auto* logicCombo = new QComboBox(m_condTable);
        logicCombo->addItem(QStringLiteral("AND"));
        logicCombo->addItem(QStringLiteral("OR"));
        logicCombo->setCurrentIndex(c.useAND ? 0 : 1);
        m_condTable->setCellWidget(i, 3, logicCombo);
        connect(logicCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, i]() { onConditionChanged(); });
    }

    m_condTable->blockSignals(false);
}

void InfoDataWidget::syncConditionsToRecord()
{
    if (!m_recordPtr || !m_condTable) return;
    auto* rec = static_cast<InfoRecord*>(m_recordPtr);

    rec->conditions.clear();
    for (int i = 0; i < m_condTable->rowCount(); ++i)
    {
        DialogueCondition c;

        if (auto* fn = qobject_cast<QComboBox*>(m_condTable->cellWidget(i, 0)))
            c.function = fn->currentText();
        if (auto* op = qobject_cast<QComboBox*>(m_condTable->cellWidget(i, 1)))
            c.comparison = op->currentText();
        if (auto* valItem = m_condTable->item(i, 2))
        {
            QString v = valItem->text().trimmed();
            bool ok = false;
            int asInt = v.toInt(&ok);
            if (ok) c.value = asInt;
            else
            {
                bool fok = false;
                double asFloat = v.toDouble(&fok);
                if (fok) c.value = asFloat;
                else     c.value = v;
            }
        }
        if (auto* logic = qobject_cast<QComboBox*>(m_condTable->cellWidget(i, 3)))
            c.useAND = (logic->currentIndex() == 0);

        rec->conditions.append(c);
    }

    // Keep legacy conditionIds vector in sync (size-only mirror so save() still works).
    if (rec->conditionIds.size() != rec->conditions.size())
        rec->conditionIds.resize(rec->conditions.size());
}

void InfoDataWidget::onAddCondition()
{
    if (!m_condTable) return;
    int row = m_condTable->rowCount();
    m_condTable->blockSignals(true);
    m_condTable->insertRow(row);

    auto* fnCombo = new QComboBox(m_condTable);
    fnCombo->addItems(conditionFunctions());
    m_condTable->setCellWidget(row, 0, fnCombo);
    connect(fnCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this]() { onConditionChanged(); });

    auto* opCombo = new QComboBox(m_condTable);
    opCombo->addItems(comparisonOperators());
    m_condTable->setCellWidget(row, 1, opCombo);
    connect(opCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this]() { onConditionChanged(); });

    m_condTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("0")));

    auto* logicCombo = new QComboBox(m_condTable);
    logicCombo->addItem(QStringLiteral("AND"));
    logicCombo->addItem(QStringLiteral("OR"));
    m_condTable->setCellWidget(row, 3, logicCombo);
    connect(logicCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this]() { onConditionChanged(); });

    m_condTable->blockSignals(false);
    syncConditionsToRecord();
}

void InfoDataWidget::onRemoveCondition()
{
    if (!m_condTable) return;
    int row = m_condTable->currentRow();
    if (row < 0) return;
    m_condTable->removeRow(row);
    syncConditionsToRecord();
}

void InfoDataWidget::onConditionChanged()
{
    syncConditionsToRecord();
}

void InfoDataWidget::onBrowseVoiceFile()
{
    if (!m_voiceEdit) return;
    QString startDir = m_voiceEdit->text();
    if (startDir.isEmpty()) startDir = QDir::homePath();

    QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Voice File"), startDir,
        QStringLiteral("Voice Files (*.wav *.ogg *.mp3);;All Files (*)"));
    if (!path.isEmpty())
        m_voiceEdit->setText(path);
}

void InfoDataWidget::onPlayVoiceFile()
{
    if (!m_voiceEdit) return;
    QString path = m_voiceEdit->text();
    if (path.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Play Voice File"),
            QStringLiteral("No voice file is set."));
        return;
    }
    if (!QFile::exists(path))
    {
        QMessageBox::warning(this, QStringLiteral("Play Voice File"),
            QStringLiteral("File does not exist:\n%1").arg(path));
        return;
    }

#ifdef _WIN32
    QString playPath = path;
    QString tempFuzWav;

    // .fuz containers embed the audio in an "XWAV" chunk. If that chunk is
    // a plain WAV (RIFF), extract it to a temp file for playback.
    if (path.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive))
    {
        FuzParser fuz;
        if (FuzParser::loadFile(path, fuz) && fuz.hasAudio()
            && fuz.audioData.startsWith("RIFF"))
        {
            tempFuzWav = QDir::tempPath() + QStringLiteral("/openck_voice_")
                + QString::number(QDateTime::currentMSecsSinceEpoch())
                + QStringLiteral(".wav");
            QFile wavFile(tempFuzWav);
            if (wavFile.open(QIODevice::WriteOnly))
            {
                wavFile.write(fuz.audioData);
                wavFile.close();
                playPath = tempFuzWav;
            }
        }
    }

    // Win32 PlaySound is async (SND_ASYNC) and supports .wav directly.
    std::wstring w = playPath.toStdWString();
    BOOL ok = PlaySoundW(w.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    if (!ok)
    {
        QMessageBox::warning(this, QStringLiteral("Play Voice File"),
            QStringLiteral("Could not play voice file (Win32 PlaySound failed)."));
    }
#else
    QMessageBox::information(this, QStringLiteral("Play Voice File"),
        QStringLiteral("Voice playback is not supported on this platform."));
#endif
}

void InfoDataWidget::onCompileFragment()
{
    if (!m_fragmentEdit) return;
    QString fragment = m_fragmentEdit->toPlainText().trimmed();
    if (fragment.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Compile Fragment"),
            QStringLiteral("Fragment is empty."));
        return;
    }

    // Write fragment to a temp .psc so the PapyrusCompiler pipeline can pick it up.
    QString tempDir = QDir::tempPath() + QStringLiteral("/openck_fragment");
    QDir().mkpath(tempDir);
    QString scriptPath = tempDir + QStringLiteral("/fragment.psc");
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::critical(this, QStringLiteral("Compile Fragment"),
            QStringLiteral("Could not write temp script:\n%1").arg(scriptPath));
        return;
    }
    QTextStream ts(&scriptFile);
    ts << fragment;
    scriptFile.close();

    PapyrusCompiler compiler;
    QString compilerPath = PapyrusCompiler::detectCompilerPath();
    if (!compilerPath.isEmpty())
        compiler.setCompilerPath(compilerPath);
    compiler.setScriptPath(scriptPath);
    compiler.setOutputPath(tempDir);

    LOG_INFO(QString("Compiling dialogue fragment: %1").arg(scriptPath));

    bool ok = compiler.compile();
    if (ok)
    {
        QMessageBox::information(this, QStringLiteral("Compile Fragment"),
            QStringLiteral("Fragment compiled successfully."));
    }
    else
    {
        QString detail;
        for (const auto& e : compiler.getLastErrors())
            detail += QString("[%1] %2:%3: %4\n")
                .arg(e.severity == CompilerError::Severity::Fatal ? QStringLiteral("fatal")
                     : e.severity == CompilerError::Severity::Error ? QStringLiteral("error")
                     : QStringLiteral("warning"))
                .arg(e.file)
                .arg(e.line)
                .arg(e.message);
        if (detail.isEmpty()) detail = QStringLiteral("Compilation failed (no detail).");
        QMessageBox::warning(this, QStringLiteral("Compile Fragment"), detail);
    }
}

} // namespace openck