#include "dialeditor.hpp"

#include <QTableView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QTableWidget>
#include <QGroupBox>

#include "../../libs/files/esm/Dialrecord.hpp"
#include "../../libs/files/esm/Inforecord.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "logger.hpp"

DialEditor::DialEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      record(nullptr),
      isNew(false)
{
    setupUI();
}

DialEditor::~DialEditor()
{
}

void DialEditor::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Editor ID:"));
    editorIdEdit = new QLineEdit();
    topLayout->addWidget(editorIdEdit, 1);
    mainLayout->addLayout(topLayout);

    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Topic Name:"));
    nameEdit = new QLineEdit();
    nameLayout->addWidget(nameEdit, 1);
    mainLayout->addLayout(nameLayout);

    auto* topicsLayout = new QHBoxLayout();
    topicsLayout->addWidget(new QLabel("Responses:"));
    addTopicButton = new QPushButton("Add Response");
    removeTopicButton = new QPushButton("Remove Response");
    topicsLayout->addStretch();
    topicsLayout->addWidget(addTopicButton);
    topicsLayout->addWidget(removeTopicButton);
    mainLayout->addLayout(topicsLayout);

    topicListView = new QTableView();
    topicListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    topicListView->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(topicListView, 1);

    auto* detailLayout = new QVBoxLayout();
    detailLayout->addWidget(new QLabel("Response Details:"));

    auto* responseLayout = new QHBoxLayout();
    responseLayout->addWidget(new QLabel("Response Text:"));
    responseTextEdit = new QTextEdit();
    responseTextEdit->setMaximumHeight(100);
    responseLayout->addWidget(responseTextEdit, 1);
    detailLayout->addLayout(responseLayout);

    auto* emotionLayout = new QHBoxLayout();
    emotionLayout->addWidget(new QLabel("Emotion Type:"));
    emotionTypeCombo = new QComboBox();
    emotionTypeCombo->addItem("Neutral", 0);
    emotionTypeCombo->addItem("Happy", 1);
    emotionTypeCombo->addItem("Sad", 2);
    emotionTypeCombo->addItem("Angry", 3);
    emotionTypeCombo->addItem("Fear", 4);
    emotionLayout->addWidget(emotionTypeCombo);
    detailLayout->addLayout(emotionLayout);

    auto* targetLayout = new QHBoxLayout();
    targetLayout->addWidget(new QLabel("Target Info ID:"));
    targetIdEdit = new QLineEdit();
    targetLayout->addWidget(targetIdEdit, 1);
    detailLayout->addLayout(targetLayout);

    auto* voiceLayout = new QHBoxLayout();
    voiceLayout->addWidget(new QLabel("Voice File:"));
    voiceFileEdit = new QLineEdit();
    voiceLayout->addWidget(voiceFileEdit, 1);
    browseVoiceFileBtn = new QPushButton("Browse...");
    voiceLayout->addWidget(browseVoiceFileBtn);
    detailLayout->addLayout(voiceLayout);

    auto* conditionGroup = new QGroupBox("Response Conditions");
    auto* conditionLayout = new QVBoxLayout();
    conditionTable = new QTableWidget();
    conditionTable->setColumnCount(3);
    conditionTable->setHorizontalHeaderLabels({"Condition ID", "Type", "Value"});
    conditionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    conditionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    conditionLayout->addWidget(conditionTable);
    auto* conditionButtonLayout = new QHBoxLayout();
    addConditionBtn = new QPushButton("Add Condition");
    removeConditionBtn = new QPushButton("Remove Condition");
    conditionButtonLayout->addWidget(addConditionBtn);
    conditionButtonLayout->addWidget(removeConditionBtn);
    conditionLayout->addLayout(conditionButtonLayout);
    conditionGroup->setLayout(conditionLayout);
    detailLayout->addWidget(conditionGroup);

    descriptionEdit = new QTextEdit();
    descriptionEdit->setReadOnly(true);
    descriptionEdit->setMaximumHeight(60);
    detailLayout->addWidget(new QLabel("Info:"));
    detailLayout->addWidget(descriptionEdit);

    mainLayout->addLayout(detailLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialEditor::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(addTopicButton, &QPushButton::clicked, this, &DialEditor::onAddTopic);
    connect(removeTopicButton, &QPushButton::clicked, this, &DialEditor::onRemoveTopic);
    connect(topicListView, &QTableView::clicked, this, &DialEditor::onTopicSelected);
    connect(responseTextEdit, &QTextEdit::textChanged, this, &DialEditor::updateResponseDetails);
    connect(emotionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialEditor::updateResponseDetails);
    connect(targetIdEdit, &QLineEdit::textChanged, this, &DialEditor::updateResponseDetails);
    connect(browseVoiceFileBtn, &QPushButton::clicked, this, &DialEditor::onBrowseVoiceFile);
    connect(addConditionBtn, &QPushButton::clicked, this, &DialEditor::onAddCondition);
    connect(removeConditionBtn, &QPushButton::clicked, this, &DialEditor::onRemoveCondition);
}

void DialEditor::loadRecord(DialRecord* record)
{
    this->record = record;
    isNew = false;

    if (record) {
        editorIdEdit->setText(record->editorId);
        nameEdit->setText(record->topicName);
        loadTopics();
        loadConditions();
    }
}

bool DialEditor::validate()
{
    QString editorId = editorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    return true;
}

bool DialEditor::saveRecord()
{
    if (!validate())
    {
        return false;
    }

    if (!record && !isNew) {
        return false;
    }

    if (!record) {
        record = new DialRecord();
        isNew = false;
    }

    {
        auto results = ColumnValidator::validateDial(*record, mData);
        QStringList errorMessages;
        for (const auto& r : results) {
            if (r.severity == ColumnValidator::Severity::Error) {
                errorMessages << QString("%1: %2").arg(r.field, r.message);
            }
        }
        if (!errorMessages.isEmpty()) {
            QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
            return false;
        }
    }

    record->editorId = editorIdEdit->text();
    record->topicName = nameEdit->text();

    saveTopics();
    saveConditions();

    LOG_INFO(QString("Saved dialogue: %1").arg(record->editorId));
    return true;
}

void DialEditor::loadTopics()
{
    if (!record) {
        return;
    }

    auto* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"Response ID", "Type"});

    for (int i = 0; i < record->responseIds.size(); i++) {
        model->setItem(i, 0, new QStandardItem(QString::number(record->responseIds[i])));
        model->setItem(i, 1, new QStandardItem("Response"));
    }

    for (int i = 0; i < record->conditionIds.size(); i++) {
        int row = model->rowCount();
        model->setItem(row, 0, new QStandardItem(QString::number(record->conditionIds[i])));
        model->setItem(row, 1, new QStandardItem("Condition"));
    }

    for (int i = 0; i < record->animationIds.size(); i++) {
        int row = model->rowCount();
        model->setItem(row, 0, new QStandardItem(QString::number(record->animationIds[i])));
        model->setItem(row, 1, new QStandardItem("Animation"));
    }

    for (int i = 0; i < record->emotionIds.size(); i++) {
        int row = model->rowCount();
        model->setItem(row, 0, new QStandardItem(QString::number(record->emotionIds[i])));
        model->setItem(row, 1, new QStandardItem("Emotion"));
    }

    topicListView->setModel(model);
}

void DialEditor::saveTopics()
{
    if (!record) {
        return;
    }

    record->responseIds.clear();
    record->conditionIds.clear();
    record->animationIds.clear();
    record->emotionIds.clear();

    auto* model = qobject_cast<QStandardItemModel*>(topicListView->model());
    if (!model) {
        return;
    }

    for (int row = 0; row < model->rowCount(); row++) {
        QString type = model->item(row, 1)->text();
        quint32 id = 0;
        {
            bool ok = false;
            id = model->item(row, 0)->text().toUInt(&ok);
            if (!ok) id = 0;
        }
        
        if (type == "Response") {
            record->responseIds.append(id);
        } else if (type == "Condition") {
            record->conditionIds.append(id);
        } else if (type == "Animation") {
            record->animationIds.append(id);
        } else if (type == "Emotion") {
            record->emotionIds.append(id);
        }
    }
}

void DialEditor::onAddTopic()
{
    if (!record) {
        return;
    }

    record->responseIds.append(0);
    loadTopics();
    LOG_INFO("Added response");
}

void DialEditor::onRemoveTopic()
{
    if (!record) {
        return;
    }

    QModelIndex current = topicListView->currentIndex();
    if (current.isValid()) {
        auto* model = qobject_cast<QStandardItemModel*>(topicListView->model());
        if (model) {
            model->removeRow(current.row());
            LOG_INFO("Removed response");
        }
    }
}

void DialEditor::onTopicSelected(QModelIndex current)
{
    if (!current.isValid() || !record) {
        descriptionEdit->clear();
        responseTextEdit->clear();
        targetIdEdit->clear();
        voiceFileEdit->clear();
        emotionTypeCombo->setCurrentIndex(0);
        conditionTable->setRowCount(0);
        return;
    }

    auto* model = qobject_cast<QStandardItemModel*>(topicListView->model());
    if (!model) return;

    int row = current.row();
    QString id = model->item(row, 0)->text();
    QString type = model->item(row, 1)->text();

    quint32 formId = 0;
    {
        bool ok = false;
        formId = id.toUInt(&ok);
        if (!ok) formId = 0;
    }

    if (type == "Response" && mData) {
        auto& infoCollection = mData->getInfoCollection();
        for (int i = 0; i < infoCollection.size(); i++) {
            const InfoRecord& info = infoCollection.getRecord(i).get();
            if (info.formId == formId) {
                responseTextEdit->setPlainText(info.responseText);
                targetIdEdit->setText(QString::number(info.targetId));
                voiceFileEdit->setText(info.voiceFile);
                int emotionIdx = static_cast<int>(info.flags & 0x7);
                if (emotionIdx >= 0 && emotionIdx < emotionTypeCombo->count())
                    emotionTypeCombo->setCurrentIndex(emotionIdx);
                descriptionEdit->setText(QString("Editor ID: %1\nForm ID: %2")
                    .arg(info.editorId).arg(info.formId, 8, 16, QChar('0')));
                
                loadConditions();
                return;
            }
        }
        responseTextEdit->clear();
        targetIdEdit->clear();
        voiceFileEdit->clear();
        emotionTypeCombo->setCurrentIndex(0);
        conditionTable->setRowCount(0);
        descriptionEdit->setText(QString("Response not found in Info collection.\nForm ID: %1")
            .arg(formId, 8, 16, QChar('0')));
    } else if (type == "Condition") {
        descriptionEdit->setText(QString("Type: Condition\nID: %1\nConditions: %2\nTopic: %3")
            .arg(id).arg(record->conditionIds.size()).arg(record->topicName));
    } else if (type == "Animation") {
        descriptionEdit->setText(QString("Type: Animation\nID: %1\nAnimations: %2\nTopic: %3")
            .arg(id).arg(record->animationIds.size()).arg(record->topicName));
    } else if (type == "Emotion") {
        descriptionEdit->setText(QString("Type: Emotion\nID: %1\nEmotions: %2\nTopic: %3")
            .arg(id).arg(record->emotionIds.size()).arg(record->topicName));
    }
}

void DialEditor::onSave()
{
    if (saveRecord()) {
        accept();
    }
}

void DialEditor::updateResponseDetails()
{
    QModelIndex current = topicListView->currentIndex();
    if (!current.isValid() || !record || !mData) return;

    auto* model = qobject_cast<QStandardItemModel*>(topicListView->model());
    if (!model) return;

    int row = current.row();
    QString type = model->item(row, 1)->text();
    
    if (type != "Response") return;

    quint32 formId = 0;
    {
        bool ok = false;
        formId = model->item(row, 0)->text().toUInt(&ok);
        if (!ok) formId = 0;
    }
    
    auto& infoCollection = mData->getInfoCollection();
    for (int i = 0; i < infoCollection.size(); i++) {
        Record<InfoRecord>& rec = infoCollection.getRecord(i);
        InfoRecord* info = &rec.get();
        if (info && info->formId == formId) {
            info->responseText = responseTextEdit->toPlainText();
            {
                bool ok = false;
                info->targetId = targetIdEdit->text().toUInt(&ok);
            }
            info->voiceFile = voiceFileEdit->text();
            info->flags = (info->flags & ~0x7) | (emotionTypeCombo->currentData().toInt() & 0x7);
            LOG_INFO(QString("Updated response details for form %1").arg(formId, 8, 16, QChar('0')));
            return;
        }
    }
}

void DialEditor::onBrowseVoiceFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Voice File"), "", tr("Audio Files (*.wav *.mp3 *.ogg);;All Files (*)"));
    if (!fileName.isEmpty()) {
        voiceFileEdit->setText(fileName);
    }
}

void DialEditor::onAddCondition()
{
    int row = conditionTable->rowCount();
    conditionTable->insertRow(row);
    conditionTable->setItem(row, 0, new QTableWidgetItem("0x00000000"));
    conditionTable->setItem(row, 1, new QTableWidgetItem("Variable"));
    conditionTable->setItem(row, 2, new QTableWidgetItem("0"));
}

void DialEditor::onRemoveCondition()
{
    int currentRow = conditionTable->currentRow();
    if (currentRow >= 0) {
        conditionTable->removeRow(currentRow);
    }
}

void DialEditor::loadConditions()
{
    if (!record) return;

    conditionTable->setRowCount(0);
    
    for (const auto& conditionId : record->conditionIds) {
        int row = conditionTable->rowCount();
        conditionTable->insertRow(row);
        conditionTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(conditionId, 8, 16, QChar('0')).toUpper()));
        conditionTable->setItem(row, 1, new QTableWidgetItem("Condition"));
        conditionTable->setItem(row, 2, new QTableWidgetItem(""));
    }
}

void DialEditor::saveConditions()
{
    if (!record) return;

    record->conditionIds.clear();
    
    for (int row = 0; row < conditionTable->rowCount(); row++) {
        QString idText = conditionTable->item(row, 0)->text();
        quint32 id = idText.toUInt(nullptr, 16);
        if (id > 0) {
            record->conditionIds.append(id);
        }
    }
}
