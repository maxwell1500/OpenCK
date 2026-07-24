#include "infoeditor.hpp"

#include <QTableView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "../../libs/files/esm/Inforecord.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "logger.hpp"

InfoEditor::InfoEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      record(nullptr),
      isNew(false)
{
    setupUI();
}

InfoEditor::~InfoEditor()
{
}

void InfoEditor::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* idLayout = new QHBoxLayout();
    idLayout->addWidget(new QLabel("Editor ID:"));
    editorIdEdit = new QLineEdit();
    idLayout->addWidget(editorIdEdit, 1);
    mainLayout->addLayout(idLayout);

    auto* responseLayout = new QHBoxLayout();
    responseLayout->addWidget(new QLabel("Response Text:"));
    responseTextEdit = new QTextEdit();
    responseLayout->addLayout(responseLayout, 1);
    mainLayout->addLayout(responseLayout);

    auto* scriptLayout = new QHBoxLayout();
    scriptLayout->addWidget(new QLabel("Scripts:"));
    scriptEdit = new QTextEdit();
    scriptLayout->addLayout(scriptLayout, 1);
    mainLayout->addLayout(scriptLayout);

    addResponseButton = new QPushButton("Add Script");
    removeResponseButton = new QPushButton("Remove Script");
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addResponseButton);
    buttonLayout->addWidget(removeResponseButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    responseListView = new QTableView();
    responseListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    responseListView->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(responseListView, 1);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &InfoEditor::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(addResponseButton, &QPushButton::clicked, this, &InfoEditor::onAddResponse);
    connect(removeResponseButton, &QPushButton::clicked, this, &InfoEditor::onRemoveResponse);
    connect(responseListView, &QTableView::clicked, this, &InfoEditor::onResponseSelected);
}

void InfoEditor::loadRecord(InfoRecord* record)
{
    this->record = record;
    isNew = false;

    if (record) {
        editorIdEdit->setText(record->editorId);
        responseTextEdit->setPlainText(record->responseText);
        scriptEdit->setPlainText("");
        for (int i = 0; i < record->scriptIds.size(); i++) {
            scriptEdit->append(QString::number(record->scriptIds[i]));
        }
        loadResponses();
    }
}

bool InfoEditor::validate()
{
    QString editorId = editorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    return true;
}

bool InfoEditor::saveRecord()
{
    if (!validate())
    {
        return false;
    }

    auto results = ColumnValidator::validateInfo(*record, mData);
    QStringList errorMessages;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            errorMessages << QString("%1: %2").arg(r.field, r.message);
        }
    }
    if (!errorMessages.isEmpty())
    {
        QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
        return false;
    }

    if (!record && !isNew) {
        return false;
    }

    if (!record) {
        record = new InfoRecord();
        isNew = false;
    }

    record->editorId = editorIdEdit->text();
    record->responseText = responseTextEdit->toPlainText();

    // Parse script IDs from text edit
    record->scriptIds.clear();
    QStringList scriptLines = scriptEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    for (const auto& line : scriptLines) {
        bool ok = false;
        quint32 id = line.toUInt(&ok);
        if (ok) record->scriptIds.append(id);
    }

    saveResponses();

    LOG_INFO(QString("Saved dialogue info: %1").arg(record->editorId));
    return true;
}

void InfoEditor::loadResponses()
{
    if (!record) {
        return;
    }

    auto* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"ID", "Type"});

    for (int i = 0; i < record->conditionIds.size(); i++) {
        model->setItem(i, 0, new QStandardItem(QString::number(record->conditionIds[i])));
        model->setItem(i, 1, new QStandardItem("Condition"));
    }

    responseListView->setModel(model);
}

void InfoEditor::saveResponses()
{
    if (!record) {
        return;
    }

    record->conditionIds.clear();

    auto* model = qobject_cast<QStandardItemModel*>(responseListView->model());
    if (!model) {
        return;
    }

    for (int row = 0; row < model->rowCount(); row++) {
        bool ok = false;
        quint32 id = model->item(row, 0)->text().toUInt(&ok);
        if (ok) record->conditionIds.append(id);
    }
}

void InfoEditor::onAddResponse()
{
    if (!record) {
        return;
    }

    if (scriptEdit->toPlainText().isEmpty()) {
        scriptEdit->append("0");
    } else {
        scriptEdit->append(QString::number(record->scriptIds.last() + 1));
    }
    loadResponses();
    LOG_INFO("Added response condition");
}

void InfoEditor::onRemoveResponse()
{
    if (!record) {
        return;
    }

    QModelIndex current = responseListView->currentIndex();
    if (current.isValid()) {
        auto* model = qobject_cast<QStandardItemModel*>(responseListView->model());
        if (model) {
            model->removeRow(current.row());
            LOG_INFO("Removed response condition");
        }
    }
}

void InfoEditor::onResponseSelected(QModelIndex current)
{
    Q_UNUSED(current)
}

void InfoEditor::onSave()
{
    if (saveRecord()) {
        accept();
    }
}
