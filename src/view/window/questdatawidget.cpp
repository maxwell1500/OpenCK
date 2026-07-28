#include "questdatawidget.hpp"
#include "../libs/files/esm/questrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "fieldvalidators.hpp"
#include "papyruscompiler.hpp"
#include "logger.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>

namespace openck {

namespace {

const NAME kFragmentSubName = 'QFRA';

QVector<QString> loadFragments(const QuestRecord* rec)
{
    QVector<QString> frags;
    for (const auto& raw : rec->rawSubRecords)
    {
        if (raw.name == kFragmentSubName)
        {
            QDataStream stream(raw.data);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            stream >> count;
            for (quint32 i = 0; i < count; ++i)
            {
                quint32 len = 0;
                stream >> len;
                QByteArray bytes = raw.data.mid(stream.device()->pos(), static_cast<int>(len));
                stream.skipRawData(static_cast<int>(len));
                frags.append(QString::fromUtf8(bytes));
            }
            return frags;
        }
    }
    return frags;
}

void storeFragments(QuestRecord* rec, const QVector<QString>& frags)
{
    for (int i = rec->rawSubRecords.size() - 1; i >= 0; --i)
    {
        if (rec->rawSubRecords[i].name == kFragmentSubName)
            rec->rawSubRecords.removeAt(i);
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << (quint32)frags.size();
    for (const QString& f : frags)
    {
        QByteArray bytes = f.toUtf8();
        stream << (quint32)bytes.size();
        stream.writeRawData(bytes.constData(), bytes.size());
    }
    RawSubRecord raw;
    raw.name = kFragmentSubName;
    raw.data = data;
    rec->rawSubRecords.append(raw);
}

quint32 nextStageIndex(const QuestRecord* rec)
{
    quint32 maxIdx = 0;
    bool any = false;
    for (quint32 id : rec->stageIds)
    {
        if (!any || id > maxIdx) { maxIdx = id; any = true; }
    }
    return any ? maxIdx + 1 : 1;
}

} // namespace

QuestDataWidget::QuestDataWidget(void* recordPtr, FormComponents* components,
                                 QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
    , m_stageTable(nullptr)
    , m_indexSpin(nullptr)
    , m_doneFlagCheck(nullptr)
    , m_repeatFlagCheck(nullptr)
    , m_descEdit(nullptr)
    , m_objectiveEdit(nullptr)
    , m_fragmentEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_selectedRow(-1)
    , m_syncing(false)
{
    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        mainLayout->addWidget(new QLabel(QStringLiteral("No record data available"), this));
        return;
    }

    auto* rec = static_cast<QuestRecord*>(m_recordPtr);

    auto* infoGroup = new QGroupBox(QStringLiteral("Quest Info"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* nameEdit = new QLineEdit(infoGroup);
    nameEdit->setText(rec->questName);
    infoForm->addRow(QStringLiteral("Quest Name:"), nameEdit);
    connect(nameEdit, &QLineEdit::textChanged, this,
        [rec](const QString& t) { rec->questName = t; });

    auto* descEdit = new QLineEdit(infoGroup);
    descEdit->setText(rec->questDesc);
    infoForm->addRow(QStringLiteral("Description:"), descEdit);
    connect(descEdit, &QLineEdit::textChanged, this,
        [rec](const QString& t) { rec->questDesc = t; });

    auto* typeSpin = new QSpinBox(infoGroup);
    typeSpin->setRange(0, INT_MAX);
    typeSpin->setValue(static_cast<int>(rec->questType));
    infoForm->addRow(QStringLiteral("Quest Type:"), typeSpin);
    connect(typeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
        [rec](int v) { rec->questType = static_cast<quint32>(v); });

    mainLayout->addWidget(infoGroup);

    // --- Stage list ---
    auto* stageGroup = new QGroupBox(QStringLiteral("Quest Stages"), this);
    auto* stageLayout = new QVBoxLayout(stageGroup);

    m_stageTable = new QTableWidget(0, 3, stageGroup);
    m_stageTable->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("Index")
                      << QStringLiteral("Flags")
                      << QStringLiteral("Description"));
    m_stageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_stageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_stageTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_stageTable->horizontalHeader()->setStretchLastSection(true);
    m_stageTable->verticalHeader()->setVisible(false);
    m_stageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stageTable->setSelectionMode(QAbstractItemView::SingleSelection);
    stageLayout->addWidget(m_stageTable);

    auto* stageBtnRow = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("Add Stage"), stageGroup);
    auto* removeBtn = new QPushButton(QStringLiteral("Remove Stage"), stageGroup);
    stageBtnRow->addWidget(addBtn);
    stageBtnRow->addWidget(removeBtn);
    stageBtnRow->addStretch();
    stageLayout->addLayout(stageBtnRow);

    mainLayout->addWidget(stageGroup);

    // --- Stage detail panel ---
    auto* detailGroup = new QGroupBox(QStringLiteral("Stage Detail"), this);
    auto* detailLayout = new QVBoxLayout(detailGroup);

    auto* detailForm = new QFormLayout();
    m_indexSpin = new QSpinBox(detailGroup);
    m_indexSpin->setRange(0, 9999);
    detailForm->addRow(QStringLiteral("Index:"), m_indexSpin);

    auto* flagRow = new QHBoxLayout();
    m_doneFlagCheck = new QCheckBox(QStringLiteral("Done"), detailGroup);
    m_repeatFlagCheck = new QCheckBox(QStringLiteral("Repeat"), detailGroup);
    flagRow->addWidget(m_doneFlagCheck);
    flagRow->addWidget(m_repeatFlagCheck);
    flagRow->addStretch();
    detailForm->addRow(QStringLiteral("Flags:"), flagRow);

    detailLayout->addLayout(detailForm);

    detailLayout->addWidget(new QLabel(QStringLiteral("Description:")));
    m_descEdit = new QPlainTextEdit(detailGroup);
    m_descEdit->setMaximumHeight(80);
    detailLayout->addWidget(m_descEdit);

    detailLayout->addWidget(new QLabel(QStringLiteral("Objective Link (FormID):")));
    m_objectiveEdit = new QLineEdit(detailGroup);
    m_objectiveEdit->setPlaceholderText(QStringLiteral("0x00000000 (or blank)"));
    setHexFormIdValidator(m_objectiveEdit, this);
    detailLayout->addWidget(m_objectiveEdit);

    detailLayout->addWidget(new QLabel(QStringLiteral("Script Fragment (Papyrus):")));
    m_fragmentEdit = new QPlainTextEdit(detailGroup);
    m_fragmentEdit->setMaximumHeight(120);
    m_fragmentEdit->setPlaceholderText(
        QStringLiteral("Papyrus fragment executed when this stage is set..."));
    detailLayout->addWidget(m_fragmentEdit);

    auto* fragBtnRow = new QHBoxLayout();
    auto* compileBtn = new QPushButton(QStringLiteral("Compile Fragment"), detailGroup);
    fragBtnRow->addWidget(compileBtn);
    fragBtnRow->addStretch();
    detailLayout->addLayout(fragBtnRow);

    mainLayout->addWidget(detailGroup);

    m_statusLabel = new QLabel(QStringLiteral("Ready"), this);
    mainLayout->addWidget(m_statusLabel);

    populateStageTable();

    connect(addBtn, &QPushButton::clicked, this, &QuestDataWidget::onAddStage);
    connect(removeBtn, &QPushButton::clicked, this, &QuestDataWidget::onRemoveStage);
    connect(m_stageTable, &QTableWidget::itemSelectionChanged,
            this, &QuestDataWidget::onStageSelectionChanged);
    connect(m_stageTable, &QTableWidget::cellChanged,
            this, &QuestDataWidget::onStageCellChanged);
    connect(m_indexSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(m_doneFlagCheck, &QCheckBox::toggled, this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(m_repeatFlagCheck, &QCheckBox::toggled, this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(m_descEdit, &QPlainTextEdit::textChanged, this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(m_objectiveEdit, &QLineEdit::textChanged, this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(m_fragmentEdit, &QPlainTextEdit::textChanged, this,
            [this]() { if (!m_syncing) onDetailChanged(); });
    connect(compileBtn, &QPushButton::clicked, this, &QuestDataWidget::onCompileFragment);

    if (m_stageTable->rowCount() > 0)
    {
        m_stageTable->selectRow(0);
    }
    else
    {
        clearDetailPanel();
    }
}

QuestDataWidget::~QuestDataWidget() = default;

void QuestDataWidget::populateStageTable()
{
    if (!m_recordPtr || !m_stageTable) return;
    auto* rec = static_cast<QuestRecord*>(m_recordPtr);

    m_syncing = true;
    m_stageTable->setRowCount(0);

    int n = rec->stageIds.size();
    m_stageTable->setRowCount(n);
    for (int i = 0; i < n; ++i)
    {
        auto* idxItem = new QTableWidgetItem(QString::number(rec->stageIds[i]));
        idxItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        m_stageTable->setItem(i, 0, idxItem);

        // Flags: simple display. The queststageeditor stores SFLG in rawSubRecords;
        // we show a placeholder bitfield label derived from SFLG if present.
        quint32 flags = 0;
        for (const auto& raw : rec->rawSubRecords)
        {
            if (raw.name == 'SFLG')
            {
                QDataStream s(raw.data);
                s.setByteOrder(QDataStream::LittleEndian);
                quint32 count = 0;
                s >> count;
                if (static_cast<int>(count) > i)
                {
                    quint32 tmp = 0;
                    s.skipRawData(static_cast<int>(i * 4));
                    s >> tmp;
                    flags = tmp;
                }
                break;
            }
        }
        QStringList flagNames;
        if (flags & 0x01) flagNames << QStringLiteral("Done");
        if (flags & 0x02) flagNames << QStringLiteral("Repeat");
        auto* flagItem = new QTableWidgetItem(flagNames.join(QStringLiteral(", ")));
        flagItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_stageTable->setItem(i, 1, flagItem);

        QString desc = (i < rec->stageDescriptions.size()) ? rec->stageDescriptions[i] : QString();
        auto* descItem = new QTableWidgetItem(desc);
        descItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        m_stageTable->setItem(i, 2, descItem);
    }
    m_syncing = false;
}

void QuestDataWidget::clearDetailPanel()
{
    m_syncing = true;
    m_indexSpin->setValue(0);
    m_doneFlagCheck->setChecked(false);
    m_repeatFlagCheck->setChecked(false);
    m_descEdit->clear();
    m_objectiveEdit->clear();
    m_fragmentEdit->clear();
    m_syncing = false;
}

void QuestDataWidget::loadStageDetail(int row)
{
    if (!m_recordPtr || row < 0 || row >= m_stageTable->rowCount())
    {
        clearDetailPanel();
        m_selectedRow = -1;
        return;
    }

    auto* rec = static_cast<QuestRecord*>(m_recordPtr);
    m_selectedRow = row;

    m_syncing = true;
    m_indexSpin->setValue(static_cast<int>(rec->stageIds[row]));

    // Flags from SFLG raw subrecord
    quint32 flags = 0;
    for (const auto& raw : rec->rawSubRecords)
    {
        if (raw.name == 'SFLG')
        {
            QDataStream s(raw.data);
            s.setByteOrder(QDataStream::LittleEndian);
            quint32 count = 0;
            s >> count;
            if (static_cast<int>(count) > row)
            {
                s.skipRawData(static_cast<int>(row * 4));
                s >> flags;
            }
            break;
        }
    }
    m_doneFlagCheck->setChecked(flags & 0x01);
    m_repeatFlagCheck->setChecked(flags & 0x02);

    QString desc = (row < rec->stageDescriptions.size()) ? rec->stageDescriptions[row] : QString();
    m_descEdit->setPlainText(desc);

    // Objective link: find objectiveId matching this stage's stageId, if any
    quint32 stageId = rec->stageIds[row];
    quint32 objectiveFormId = 0;
    bool hasObjective = false;
    for (quint32 oid : rec->objectiveIds)
    {
        if (oid == stageId) { hasObjective = true; break; }
    }
    Q_UNUSED(objectiveFormId);
    m_objectiveEdit->setText(hasObjective
        ? QStringLiteral("0x%1").arg(stageId, 8, 16, QChar('0')).toUpper()
        : QString());

    // Script fragment from QFRA raw subrecord
    QVector<QString> frags = loadFragments(rec);
    QString frag = (row < frags.size()) ? frags[row] : QString();
    m_fragmentEdit->setPlainText(frag);

    m_syncing = false;
}

void QuestDataWidget::onStageSelectionChanged()
{
    if (m_syncing) return;
    QList<QTableWidgetItem*> sel = m_stageTable->selectedItems();
    if (sel.isEmpty())
    {
        clearDetailPanel();
        m_selectedRow = -1;
        return;
    }
    loadStageDetail(sel.first()->row());
}

void QuestDataWidget::onStageCellChanged(int row, int column)
{
    if (m_syncing || !m_recordPtr) return;
    auto* rec = static_cast<QuestRecord*>(m_recordPtr);
    if (row < 0 || row >= rec->stageIds.size()) return;

    if (column == 0)
    {
        auto* item = m_stageTable->item(row, 0);
        if (!item) return;
        bool ok = false;
        quint32 v = item->text().toUInt(&ok);
        if (ok) rec->stageIds[row] = v;
    }
    else if (column == 2)
    {
        auto* item = m_stageTable->item(row, 2);
        if (!item) return;
        if (row >= rec->stageDescriptions.size())
            rec->stageDescriptions.resize(row + 1);
        rec->stageDescriptions[row] = item->text();
        if (row == m_selectedRow)
        {
            m_syncing = true;
            m_descEdit->setPlainText(item->text());
            m_syncing = false;
        }
    }
}

void QuestDataWidget::syncDetailToRecord()
{
    if (!m_recordPtr || m_selectedRow < 0 || m_selectedRow >= m_stageTable->rowCount())
        return;

    auto* rec = static_cast<QuestRecord*>(m_recordPtr);
    int row = m_selectedRow;

    // Index
    rec->stageIds[row] = static_cast<quint32>(m_indexSpin->value());
    m_syncing = true;
    m_stageTable->item(row, 0)->setText(QString::number(rec->stageIds[row]));
    m_syncing = false;

    // Description
    QString desc = m_descEdit->toPlainText();
    if (row >= rec->stageDescriptions.size())
        rec->stageDescriptions.resize(row + 1);
    rec->stageDescriptions[row] = desc;
    m_syncing = true;
    m_stageTable->item(row, 2)->setText(desc);
    m_syncing = false;

    // Flags -> SFLG raw subrecord
    quint32 flags = (m_doneFlagCheck->isChecked() ? 0x01 : 0u)
                  | (m_repeatFlagCheck->isChecked() ? 0x02 : 0u);
    {
        QVector<quint32> flagVec;
        bool found = false;
        for (int i = rec->rawSubRecords.size() - 1; i >= 0; --i)
        {
            if (rec->rawSubRecords[i].name == 'SFLG')
            {
                QDataStream s(rec->rawSubRecords[i].data);
                s.setByteOrder(QDataStream::LittleEndian);
                quint32 count = 0;
                s >> count;
                flagVec.resize(qMax(static_cast<int>(count), row + 1));
                for (quint32 j = 0; j < count; ++j) s >> flagVec[j];
                rec->rawSubRecords.removeAt(i);
                found = true;
                break;
            }
        }
        if (!found) flagVec.resize(row + 1);
        if (flagVec.size() <= row) flagVec.resize(row + 1);
        flagVec[row] = flags;

        QByteArray data;
        QDataStream s(&data, QIODevice::WriteOnly);
        s.setByteOrder(QDataStream::LittleEndian);
        s << (quint32)flagVec.size();
        for (quint32 f : flagVec) s << f;
        RawSubRecord raw;
        raw.name = 'SFLG';
        raw.data = data;
        rec->rawSubRecords.append(raw);
    }

    // Update flags column display
    QStringList flagNames;
    if (flags & 0x01) flagNames << QStringLiteral("Done");
    if (flags & 0x02) flagNames << QStringLiteral("Repeat");
    m_syncing = true;
    m_stageTable->item(row, 1)->setText(flagNames.join(QStringLiteral(", ")));
    m_syncing = false;

    // Objective link: if non-empty, ensure stageId is in objectiveIds; else remove it
    QString objText = m_objectiveEdit->text().trimmed();
    quint32 stageId = rec->stageIds[row];
    if (objText.isEmpty())
    {
        rec->objectiveIds.removeAll(stageId);
    }
    else
    {
        if (!rec->objectiveIds.contains(stageId))
            rec->objectiveIds.append(stageId);
    }

    // Script fragment
    QVector<QString> frags = loadFragments(rec);
    if (frags.size() <= row) frags.resize(row + 1);
    frags[row] = m_fragmentEdit->toPlainText();
    storeFragments(rec, frags);

    if (m_statusLabel)
        m_statusLabel->setText(QStringLiteral("Stage %1 updated.").arg(row));
}

void QuestDataWidget::onDetailChanged()
{
    syncDetailToRecord();
}

void QuestDataWidget::onAddStage()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<QuestRecord*>(m_recordPtr);

    quint32 newIndex = nextStageIndex(rec);
    rec->stageIds.append(newIndex);
    rec->stageDescriptions.append(QString());

    populateStageTable();
    int newRow = m_stageTable->rowCount() - 1;
    if (newRow >= 0) m_stageTable->selectRow(newRow);

    LOG_INFO(QString("QuestDataWidget: added stage index %1").arg(newIndex));
    if (m_statusLabel)
        m_statusLabel->setText(QStringLiteral("Added stage %1.").arg(newIndex));
}

void QuestDataWidget::onRemoveStage()
{
    if (!m_recordPtr || !m_stageTable) return;
    int row = m_stageTable->currentRow();
    if (row < 0) return;

    auto reply = QMessageBox::question(this, QStringLiteral("Remove Stage"),
        QStringLiteral("Remove stage at row %1?").arg(row),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    auto* rec = static_cast<QuestRecord*>(m_recordPtr);
    if (row < rec->stageIds.size()) rec->stageIds.removeAt(row);
    if (row < rec->stageDescriptions.size()) rec->stageDescriptions.removeAt(row);

    // Remove matching objective
    if (row < rec->stageIds.size() || !rec->stageIds.isEmpty())
    {
        quint32 stageId = (row < rec->stageIds.size()) ? rec->stageIds[row] : 0;
        rec->objectiveIds.removeAll(stageId);
    }

    // Remove corresponding fragment
    QVector<QString> frags = loadFragments(rec);
    if (row < frags.size()) frags.removeAt(row);
    storeFragments(rec, frags);

    populateStageTable();
    if (m_stageTable->rowCount() > 0)
        m_stageTable->selectRow(qMin(row, m_stageTable->rowCount() - 1));
    else
        clearDetailPanel();

    LOG_INFO(QString("QuestDataWidget: removed stage row %1").arg(row));
    if (m_statusLabel)
        m_statusLabel->setText(QStringLiteral("Removed stage."));
}

void QuestDataWidget::onCompileFragment()
{
    if (!m_fragmentEdit) return;
    QString fragment = m_fragmentEdit->toPlainText().trimmed();
    if (fragment.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Compile Fragment"),
            QStringLiteral("Fragment is empty."));
        return;
    }

    QString tempDir = QDir::tempPath() + QStringLiteral("/openck_quest_fragment");
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

    LOG_INFO(QString("Compiling quest stage fragment: %1").arg(scriptPath));

    bool ok = compiler.compile();
    if (ok)
    {
        QMessageBox::information(this, QStringLiteral("Compile Fragment"),
            QStringLiteral("Fragment compiled successfully."));
        if (m_statusLabel)
            m_statusLabel->setText(QStringLiteral("Fragment compiled."));
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