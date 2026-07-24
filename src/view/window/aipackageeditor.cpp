#include "aipackageeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/ckid.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/packagerecord.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "pack_editor.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

AIPackageEditor::AIPackageEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mAddPackageButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedPack(nullptr)
{
    LOG_INFO("AIPackageEditor created");
    setupUI();
    loadPackages();
}

AIPackageEditor::~AIPackageEditor()
{
}

void AIPackageEditor::setupUI()
{
    setWindowTitle("AI Packages Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Package" << "Type" << "Details");
    mTree->setColumnWidth(0, 300);
    mTree->setColumnWidth(1, 80);
    mTree->setColumnWidth(2, 400);
    mTree->setAlternatingRowColors(true);
    mTree->setRootIsDecorated(true);
    splitter->addWidget(mTree);

    mDetailEdit = new QTextEdit();
    mDetailEdit->setReadOnly(true);
    mDetailEdit->setFontPointSize(10);
    splitter->addWidget(mDetailEdit);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    auto* buttonBar = new QHBoxLayout();
    mAddPackageButton = new QPushButton("Add Package");
    buttonBar->addWidget(mAddPackageButton);

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonBar->addWidget(mEditButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mSaveButton = new QPushButton("Save Changes");
    buttonBar->addWidget(mSaveButton);

    mainLayout->addLayout(buttonBar);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    connect(mTree, &QTreeWidget::itemClicked, this, &AIPackageEditor::onNodeSelected);
    connect(mAddPackageButton, &QPushButton::clicked, this, &AIPackageEditor::onAddPackage);
    connect(mEditButton, &QPushButton::clicked, this, &AIPackageEditor::onEditPackage);
    connect(mDeleteButton, &QPushButton::clicked, this, &AIPackageEditor::onDeletePackage);
    connect(mSaveButton, &QPushButton::clicked, this, &AIPackageEditor::onSave);
}

void AIPackageEditor::loadPackages()
{
    mTree->clear();
    mSelectedPack = nullptr;

    auto& packCollection = mData->getPackCollection();
    QVector<QString> packIds = packCollection.getIds(false);

    for (const QString& packId : packIds) {
        int idx = packCollection.getIndex(packId);
        if (idx < 0) continue;

        PackageRecord& pack = packCollection.getRecord(idx).get();
        QTreeWidgetItem* packItem = new QTreeWidgetItem(mTree);
        packItem->setText(0, pack.editorId.isEmpty() ? QString("Package_%1").arg(pack.formId, 8, 16, QChar('0')).toUpper() : pack.editorId);
        packItem->setText(1, "PACKAGE");
        packItem->setText(2, QString("Type: %1 | Targets: %2 | Params: %3")
            .arg(pack.packageType)
            .arg(pack.targetIds.size())
            .arg(pack.parameters.size()));
        packItem->setData(0, Qt::UserRole, QVariant::fromValue<PackageRecord*>(&pack));
    }

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 packages").arg(packIds.size()));
    LOG_INFO(QString("Loaded %1 packages").arg(packIds.size()));
}

void AIPackageEditor::refreshTree()
{
    loadPackages();
}

void AIPackageEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);

    QString type = item->text(1);

    if (type == "PACKAGE") {
        PackageRecord* pack = static_cast<PackageRecord*>(item->data(0, Qt::UserRole).value<PackageRecord*>());
        if (pack) {
            mSelectedPack = pack;
            showPackageDetails(pack);
        }
    }
}

void AIPackageEditor::showPackageDetails(const PackageRecord* pack)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(pack->editorId.isEmpty() ? QString("Package_%1").arg(pack->formId, 8, 16, QChar('0')).toUpper() : pack->editorId);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(pack->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Package Type:</b> %1</p>").arg(pack->packageType);
    text += QString("<p><b>Target Type:</b> %1</p>").arg(pack->targetType);
    text += QString("<p><b>Flags:</b> 0x%1</p>").arg(pack->flags, 8, 16, QChar('0')).toUpper();

    text += "<h3>Target IDs</h3>";
    if (pack->targetIds.isEmpty()) {
        text += "<p>(none)</p>";
    } else {
        text += "<ul>";
        for (quint32 targetId : pack->targetIds) {
            text += QString("<li>0x%1</li>").arg(targetId, 8, 16, QChar('0')).toUpper();
        }
        text += "</ul>";
    }

    text += "<h3>Parameters</h3>";
    if (pack->parameters.isEmpty()) {
        text += "<p>(none)</p>";
    } else {
        text += "<ul>";
        for (quint32 param : pack->parameters) {
            text += QString("<li>%1</li>").arg(param);
        }
        text += "</ul>";
    }

    mDetailEdit->setHtml(text);
}

void AIPackageEditor::onAddPackage()
{
    bool ok = false;
    QString editorId = QInputDialog::getText(this, "Add Package",
        "Enter Editor ID:", QLineEdit::Normal, "", &ok);

    if (!ok || editorId.isEmpty()) return;

    PackageRecord newPack;
    newPack.editorId = editorId;
    newPack.formId = 0; // Will be assigned by Data class
    newPack.flags = 0;
    newPack.packageType = 0;
    newPack.targetType = 0;

    if (mData->addPack(newPack)) {
        LOG_INFO(QString("Added package '%1'").arg(editorId));
        mStatusLabel->setText(QString("Added package '%1'").arg(editorId));
        refreshTree();
    } else {
        QMessageBox::critical(this, "Error", "Failed to add package.");
    }
}

void AIPackageEditor::onEditPackage()
{
    if (!mSelectedPack) return;

    PackageRecord originalState = *mSelectedPack;
    PackageRecord editedState = originalState;
    PackEditor editor(mData, &editedState, this);
    if (editor.exec() == QDialog::Accepted) {
        auto& coll = mData->getPackCollection();
        int idx = coll.searchId(editedState.editorId);
        if (idx >= 0 && mData->getUndoStack()) {
            EditRecordCommand<PackageRecord>* cmd = new EditRecordCommand<PackageRecord>(&coll, idx, originalState, editedState,
                "Edit package: " + editedState.editorId);
            if (cmd->hasChanged()) {
                mData->getUndoStack()->push(cmd);
            } else {
                delete cmd;
            }
        }
        LOG_INFO(QString("Updated package '%1'").arg(editedState.editorId));
        refreshTree();
    }
}

void AIPackageEditor::onDeletePackage()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    QString type = item->text(1);

    if (type == "PACKAGE") {
        const PackageRecord* pack = static_cast<const PackageRecord*>(item->data(0, Qt::UserRole).value<const PackageRecord*>());
        if (!pack) return;

        auto reply = QMessageBox::question(this, "Delete Package",
            QString("Are you sure you want to delete package '%1'?\n\nThis action cannot be undone.")
                .arg(pack->editorId),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mData->removeRecord(CkId::Type_Pack_, pack->editorId);
            LOG_INFO(QString("Deleted package '%1'").arg(pack->editorId));
            refreshTree();
        }
    }
}

void AIPackageEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save AI Packages", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty()) return;

    ESMWriter writer;
    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", QString("Cannot open file: %1").arg(filePath));
        return;
    }

    const auto& metaData = mData->getMetaData().getRecords();
    for (const auto& record : metaData)
    {
        writer.addMaster(record.get().editorId);
    }

    writer.setVersion(1.0f);

    int totalPackages = 0;
    int totalTargets = 0;
    int totalParameters = 0;

    const auto& packRecords = mData->getPackCollection().getRecords();
    for (const auto& record : packRecords)
    {
        if (record.state == State_Modified || record.state == State_ModifiedOnly)
        {
            RecHeader recHeader;
            recHeader.id = record.get().formId;
            writer.startRecord('PACK', recHeader);
            record.get().save(writer);
            writer.endRecord();
            totalPackages++;
            totalTargets += record.get().targetIds.size();
            totalParameters += record.get().parameters.size();
        }
    }

    writer.close();
    saveFile.close();

    LOG_INFO(QString("Saved AI packages to %1").arg(filePath));
    LOG_INFO(QString("Packages: %1, Targets: %2, Parameters: %3")
        .arg(totalPackages).arg(totalTargets).arg(totalParameters));

    QMessageBox::information(this, "Saved",
        QString("AI packages data exported.\n\n"
                "Packages: %1\n"
                "Targets: %2\n"
                "Parameters: %3")
            .arg(totalPackages)
            .arg(totalTargets)
            .arg(totalParameters));
}
