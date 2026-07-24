#include "dialogueeditorwidget.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/columns.hpp"
#include "../../libs/files/esm/dialrecord.hpp"
#include "../../libs/files/esm/inforecord.hpp"

#include "logger.hpp"

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>

DialogueEditorWidget::DialogueEditorWidget(Data* data, QWidget* parent) :
    QWidget(parent),
    mData(data),
    treeWidget(nullptr),
    conditionEditor(nullptr),
    topicEdit(nullptr),
    editConditionButton(nullptr),
    addInfoButton(nullptr),
    removeInfoButton(nullptr)
{
    LOG_DEBUG("DialogueEditorWidget created");
    setupUI();
}

DialogueEditorWidget::~DialogueEditorWidget()
{
    LOG_DEBUG("DialogueEditorWidget destroyed");
}

void DialogueEditorWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* topicLayout = new QHBoxLayout();
    topicLayout->addWidget(new QLabel("Topic:", this));
    topicEdit = new QLineEdit(this);
    topicEdit->setPlaceholderText("Enter dialogue topic...");
    topicLayout->addWidget(topicEdit);
    mainLayout->addLayout(topicLayout);

    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels(QStringList() << "Topic" << "Type" << "Conditions");
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(treeWidget);

    auto* conditionLayout = new QHBoxLayout();
    conditionLayout->addWidget(new QLabel("Conditions:", this));
    conditionEditor = new QTextEdit(this);
    conditionEditor->setMaximumHeight(100);
    conditionLayout->addWidget(conditionEditor);
    mainLayout->addLayout(conditionLayout);

    auto* buttonLayout = new QHBoxLayout();
    editConditionButton = new QPushButton("Edit Condition", this);
    addInfoButton = new QPushButton("Add Info", this);
    removeInfoButton = new QPushButton("Remove", this);

    buttonLayout->addWidget(editConditionButton);
    buttonLayout->addWidget(addInfoButton);
    buttonLayout->addWidget(removeInfoButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(treeWidget, &QTreeWidget::currentItemChanged, this, &DialogueEditorWidget::onTreeSelectionChanged);
    connect(editConditionButton, &QPushButton::clicked, this, &DialogueEditorWidget::onEditCondition);
    connect(addInfoButton, &QPushButton::clicked, this, &DialogueEditorWidget::onAddInfo);
    connect(removeInfoButton, &QPushButton::clicked, this, &DialogueEditorWidget::onRemoveInfo);
}

void DialogueEditorWidget::loadDialogue(const QString& dialId)
{
    LOG_INFO(QString("Loading dialogue: %1").arg(dialId));
    currentDialId = dialId;

    if (!mData) {
        LOG_ERROR("No data available");
        return;
    }

    const auto& dialCollection = mData->getDialCollection();
    int dialIndex = dialCollection.searchId(dialId);
    if (dialIndex == -1) {
        LOG_WARNING(QString("Dial record not found: %1").arg(dialId));
        return;
    }

    const auto& dialRecord = dialCollection.getRecord(dialIndex);
    topicEdit->setText(dialRecord.get().topicName);

    populateTree();

    LOG_INFO("Dialogue loaded successfully");
}

void DialogueEditorWidget::saveDialogue()
{
    LOG_INFO(QString("Saving dialogue: %1").arg(currentDialId));

    if (!mData || currentDialId.isEmpty()) {
        LOG_ERROR("Cannot save - no data or dial ID");
        return;
    }

    auto& dialCollection = mData->getDialCollection();
    int dialIndex = dialCollection.searchId(currentDialId);
    if (dialIndex == -1) {
        LOG_ERROR("Dial record not found");
        return;
    }

    DialRecord& dial = dialCollection.getRecord(dialIndex).get();
    
    dial.topicName = topicEdit->text();
    
    LOG_INFO(QString("Dialogue '%1' saved successfully").arg(currentDialId));
}

void DialogueEditorWidget::populateTree()
{
    if (!mData) return;

    treeWidget->clear();

    const auto& infoCollection = mData->getInfoCollection();
    int count = 0;

    for (int i = 0; i < infoCollection.size(); i++) {
        const auto& infoRecord = infoCollection.getRecord(i);
        auto* item = new QTreeWidgetItem(treeWidget);
        item->setText(0, infoRecord.get().editorId);
        item->setText(1, "Info");
        item->setText(2, "");
        count++;
    }

    LOG_INFO(QString("Loaded %1 info nodes").arg(count));
}

void DialogueEditorWidget::updateEditor()
{
    if (!mData || currentDialId.isEmpty()) return;
    
    auto& dialCollection = mData->getDialCollection();
    int dialIndex = dialCollection.searchId(currentDialId);
    if (dialIndex >= 0)
    {
        const DialRecord& dial = dialCollection.getRecord(dialIndex).get();
        topicEdit->setText(dial.topicName);
    }
}

void DialogueEditorWidget::onTreeSelectionChanged()
{
    LOG_DEBUG("Tree selection changed");
    auto* item = treeWidget->currentItem();
    if (item) {
        conditionEditor->setPlainText(item->text(2));
    }
}

void DialogueEditorWidget::onEditCondition()
{
    LOG_DEBUG("Edit condition clicked");
    auto* item = treeWidget->currentItem();
    if (item) {
        item->setText(2, conditionEditor->toPlainText());
    }
}

void DialogueEditorWidget::onAddInfo()
{
    LOG_DEBUG("Add info clicked");
    
    if (!mData) return;

    auto& infoCollection = mData->getInfoCollection();
    
    InfoRecord newInfo;
    newInfo.editorId = QString("NewInfo_%1").arg(infoCollection.size());
    newInfo.formId = 0;
    newInfo.responseText = "New dialogue response";
    newInfo.flags = 0;
    
    infoCollection.add(newInfo);
    populateTree();
    
    LOG_INFO(QString("Added new info record: %1").arg(newInfo.editorId));
}

void DialogueEditorWidget::onRemoveInfo()
{
    LOG_DEBUG("Remove info clicked");
    
    if (!mData) return;

    auto* item = treeWidget->currentItem();
    if (!item) return;

    int rowIndex = treeWidget->indexOfTopLevelItem(item);
    if (rowIndex < 0) return;
    
    QString editorId = item->text(0);
    
    auto& infoCollection = mData->getInfoCollection();
    int infoIndex = infoCollection.searchId(editorId);
    
    if (infoIndex >= 0)
    {
        infoCollection.removeRows(infoIndex, 1);
        populateTree();
        LOG_INFO(QString("Removed info record: %1").arg(editorId));
    }
}
