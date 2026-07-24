#include "facteditor.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "logger.hpp"
#include "../../../libs/files/esm/factrecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>

FactEditor::FactEditor(Data* data, FactRecord* fact, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mFact(fact)
{
    setWindowTitle("Faction Editor");
    resize(500, 400);
    setupUI();
    loadFromFact();
}

void FactEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* infoGroup = new QGroupBox("Basic Information");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

    QHBoxLayout* editorIdLayout = new QHBoxLayout();
    editorIdLayout->addWidget(new QLabel("Editor ID:"));
    mEditorIdEdit = new QLineEdit();
    editorIdLayout->addWidget(mEditorIdEdit);
    infoLayout->addLayout(editorIdLayout);

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    mNameEdit = new QLineEdit();
    nameLayout->addWidget(mNameEdit);
    infoLayout->addLayout(nameLayout);

    QHBoxLayout* iconLayout = new QHBoxLayout();
    iconLayout->addWidget(new QLabel("Icon Path:"));
    mIconPathEdit = new QLineEdit();
    iconLayout->addWidget(mIconPathEdit);
    infoLayout->addLayout(iconLayout);

    QHBoxLayout* descLayout = new QHBoxLayout();
    descLayout->addWidget(new QLabel("Description:"));
    mDescriptionEdit = new QPlainTextEdit();
    mDescriptionEdit->setMaximumHeight(60);
    descLayout->addWidget(mDescriptionEdit);
    infoLayout->addLayout(descLayout);

    mainLayout->addWidget(infoGroup);

    QGroupBox* ranksGroup = new QGroupBox("Faction Ranks");
    QVBoxLayout* ranksLayout = new QVBoxLayout(ranksGroup);

    mRanksList = new QListWidget();
    ranksLayout->addWidget(mRanksList);

    QHBoxLayout* rankButtonsLayout = new QHBoxLayout();
    mAddRankButton = new QPushButton("Add Rank");
    mRemoveRankButton = new QPushButton("Remove Rank");
    rankButtonsLayout->addWidget(mAddRankButton);
    rankButtonsLayout->addWidget(mRemoveRankButton);
    ranksLayout->addLayout(rankButtonsLayout);

    mainLayout->addWidget(ranksGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    mSaveButton = new QPushButton("Save");
    mCancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(mSaveButton);
    buttonLayout->addWidget(mCancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(mSaveButton, &QPushButton::clicked, this, &FactEditor::saveRecord);
    connect(mCancelButton, &QPushButton::clicked, this, &FactEditor::reject);
    connect(mAddRankButton, &QPushButton::clicked, this, &FactEditor::addRank);
    connect(mRemoveRankButton, &QPushButton::clicked, this, [this]() {
        delete mRanksList->takeItem(mRanksList->currentRow());
    });
}

void FactEditor::loadFromFact()
{
    if (!mFact) return;

    mEditorIdEdit->setText(mFact->editorId);
    mNameEdit->setText(mFact->factionName);
    mDescriptionEdit->setPlainText(mFact->description);
    mIconPathEdit->setText(mFact->iconPath);

    for (const auto& rank : mFact->ranks)
    {
        mRanksList->addItem(rank);
    }
}

void FactEditor::addRank()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Add Rank", "Enter rank name:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty())
    {
        mRanksList->addItem(text);
    }
}

void FactEditor::saveRecord()
{
    if (!mFact) return;

    if (!validateFact())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateFact(*mFact, mData);
        QStringList errorMessages;
        for (const auto& r : results) {
            if (r.severity == ColumnValidator::Severity::Error) {
                errorMessages << QString("%1: %2").arg(r.field, r.message);
            }
        }
        if (!errorMessages.isEmpty()) {
            QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
            return;
        }
    }

    mFact->editorId = mEditorIdEdit->text();
    mFact->factionName = mNameEdit->text();
    mFact->description = mDescriptionEdit->toPlainText();
    mFact->iconPath = mIconPathEdit->text();

    mFact->ranks.clear();
    for (int i = 0; i < mRanksList->count(); i++)
    {
        mFact->ranks.append(mRanksList->item(i)->text());
    }

    LOG_INFO(QString("Faction '%1' updated").arg(mFact->editorId));
    accept();
}

bool FactEditor::validateFact()
{
    if (mEditorIdEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Faction must have an EditorID.");
        return false;
    }

    auto& collection = mData->getFactCollection();
    int idx = collection.searchId(mEditorIdEdit->text());
    if (idx >= 0 && collection.getRecord(idx).get().formId != mFact->formId)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("EditorID '%1' already exists on another faction.").arg(mEditorIdEdit->text()));
        return false;
    }

    if (mNameEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Faction must have a name.");
        return false;
    }

    if (mFact->ranks.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Faction must have at least one rank.");
        return false;
    }

    return true;
}
