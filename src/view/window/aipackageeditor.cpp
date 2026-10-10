#include "aipackageeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/ckid.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/addrecordcommand.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../libs/files/esm/packagesemantics.hpp"
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
#include <utility>

AIPackageEditor::AIPackageEditor(Data* data, std::function<bool()> saveCallback,
                                 QWidget* parent)
    : QDialog(parent),
      mData(data),
      mSaveCallback(std::move(saveCallback)),
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

        // Show the semantics the editor actually exposes: the package family
        // and whether the schedule gates it.
        bool decoded = false;
        const openck::PackageData data = openck::decodePackageData(pack, &decoded);
        const QString kindName =
            decoded ? openck::packageKindName(openck::packageKindFromU32(data.type))
                     : QStringLiteral("?");
        QString schedule;
        if (decoded && openck::scheduleConstraintSet(data.schedule))
        {
            const QVector<openck::ScheduleCheck> checks =
                openck::scheduleChecks(data.schedule);
            for (const auto& check : checks)
            {
                if (!schedule.isEmpty()) schedule += QLatin1String(", ");
                schedule += check.name;
            }
        }
        else
        {
            schedule = QObject::tr("always");
        }

        packItem->setText(1, kindName);
        packItem->setText(2, pack.targetIds.isEmpty()
                                ? QObject::tr("%1 | no targets").arg(schedule)
                                : QObject::tr("%1 | %2 target(s)")
                                      .arg(schedule)
                                      .arg(pack.targetIds.size()));
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

    // A package row is identified by its attached record, not by a column
    // string, so the details column stays free to show semantics.
    const QVariant packPtr = item->data(0, Qt::UserRole);
    if (!packPtr.canConvert<PackageRecord*>())
    {
        return;
    }

    PackageRecord* pack = packPtr.value<PackageRecord*>();
    if (pack)
    {
        mSelectedPack = pack;
        showPackageDetails(pack);
    }
}

void AIPackageEditor::showPackageDetails(const PackageRecord* pack)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(pack->editorId.isEmpty() ? QString("Package_%1").arg(pack->formId, 8, 16, QChar('0')).toUpper() : pack->editorId);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(pack->formId, 8, 16, QChar('0')).toUpper();

    bool decoded = false;
    const openck::PackageData data = openck::decodePackageData(*pack, &decoded);
    if (decoded)
    {
        const openck::PackageKind kind = openck::packageKindFromU32(data.type);
        text += QString("<p><b>Kind:</b> %1 (type %2)</p>")
                    .arg(openck::packageKindName(kind))
                    .arg(data.type);
        text += QString("<p><b>Flags:</b> 0x%1 &nbsp;&nbsp;Perform all: %2</p>")
                    .arg(data.flags, 8, 16, QChar('0'))
                    .arg(data.doAll ? QStringLiteral("yes") : QStringLiteral("no"));

        text += "<h3>Schedule</h3>";
        const QVector<openck::ScheduleCheck> checks =
            openck::scheduleChecks(data.schedule);
        if (checks.isEmpty())
        {
            text += "<p>Runs at any time.</p>";
        }
        else
        {
            text += "<ul>";
            for (const auto& check : checks)
            {
                text += QString("<li>%1</li>").arg(check.name);
            }
            text += "</ul>";
        }

        const QVector<openck::PackageIssue> issues =
            openck::validatePackageData(*pack, data);
        if (!issues.isEmpty())
        {
            text += "<h3>Validation</h3><ul>";
            for (const auto& issue : issues)
            {
                const QString color = issue.severity == openck::PackageIssueSeverity::Error
                                          ? QStringLiteral("#c62828")
                                          : QStringLiteral("#f57f17");
                text += QString("<li style='color:%1'>%2</li>").arg(color, issue.message);
            }
            text += "</ul>";
        }
    }
    else
    {
        text += QString("<p><b>Package Type:</b> %1</p>").arg(pack->packageType);
        text += "<p><i>No PKDT payload: the record stores no decodable "
                "procedure data.</i></p>";
    }

    text += QString("<p><b>Target Type:</b> %1</p>").arg(pack->targetType);
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
    newPack.blank();
    newPack.initComponents();
    newPack.editorId = editorId;
    try
    {
        newPack.formId = mData->createNewRecord(CkId::Type_Pack_, editorId);
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, tr("Add Package"),
            tr("Could not allocate a form ID: %1").arg(QString::fromUtf8(e.what())));
        return;
    }

    auto& collection = mData->getPackCollection();
    if (collection.searchId(editorId) >= 0)
    {
        QMessageBox::warning(this, tr("Add Package"),
            tr("A package named '%1' already exists.").arg(editorId));
        return;
    }
    auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Pack_));
    if (!table || !mData->getUndoStack())
    {
        QMessageBox::critical(this, tr("Add Package"), tr("The package could not be created."));
        return;
    }
    Record<PackageRecord> record(State_ModifiedOnly, nullptr, &newPack);
    mData->getUndoStack()->push(new AddRecordCommand(table, &collection,
        collection.getAppendIndex(editorId, CkId::Type_Pack_), record,
        QStringLiteral("Add package: %1").arg(editorId)));
    LOG_INFO(QString("Added package '%1'").arg(editorId));
    mStatusLabel->setText(QString("Added package '%1'").arg(editorId));
    refreshTree();
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

    const QVariant packPtr = item->data(0, Qt::UserRole);
    if (!packPtr.canConvert<PackageRecord*>())
    {
        return;
    }

    const PackageRecord* pack = packPtr.value<PackageRecord*>();
    if (!pack) return;

    auto reply = QMessageBox::question(this, "Delete Package",
        QString("Are you sure you want to delete package '%1'?\n\nThis action cannot be undone.")
            .arg(pack->editorId),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        QString packId = pack->editorId;
        auto& coll = mData->getPackCollection();
        bool removed = coll.removeRecordWithUndo(packId, mData->getUndoStack());
        if (!removed)
            mData->removeRecord(CkId::Type_Pack_, packId);
        LOG_INFO(QString("Deleted package '%1'").arg(packId));
        mSelectedPack = nullptr;
        refreshTree();
    }
}

void AIPackageEditor::onSave()
{
    if (!mSaveCallback || !mSaveCallback())
    {
        QMessageBox::warning(this, tr("Save"), tr("The active document could not be saved."));
        return;
    }
    mStatusLabel->setText(tr("Active document saved."));
}
