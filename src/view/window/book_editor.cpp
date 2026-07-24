#include "book_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

BookEditor::BookEditor(Data* data, BookRecord* book, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mBook(book),
      mEditorIdEdit(nullptr),
      mIconPathEdit(nullptr),
      mModelPathEdit(nullptr),
      mPageCountSpin(nullptr),
      mPagesEdit(nullptr)
{
    setupUI();
    loadFromBook();
}

void BookEditor::setupUI()
{
    setWindowTitle("Book Editor");
    setMinimumSize(400, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Book Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mModelPathEdit = new QLineEdit();
    infoLayout->addRow("Model Path:", mModelPathEdit);

    mPageCountSpin = new QSpinBox();
    mPageCountSpin->setRange(0, 99999);
    infoLayout->addRow("Page Count:", mPageCountSpin);

    mPagesEdit = new QPlainTextEdit();
    mPagesEdit->setMaximumHeight(100);
    infoLayout->addRow("Pages:", mPagesEdit);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &BookEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void BookEditor::loadFromBook()
{
    mEditorIdEdit->setText(mBook->editorId);
    mIconPathEdit->setText(mBook->iconPath);
    mModelPathEdit->setText(mBook->modelPath);
    mPageCountSpin->setValue(mBook->pageCount);
    mPagesEdit->setPlainText(mBook->pages);
}

bool BookEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getBookCollection().searchId(editorId) >= 0)
    {
        if (editorId != mBook->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A book with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void BookEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateBook(*mBook, mData);
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

    mBook->editorId = mEditorIdEdit->text();
    mBook->iconPath = mIconPathEdit->text();
    mBook->modelPath = mModelPathEdit->text();
    mBook->pageCount = mPageCountSpin->value();
    mBook->pages = mPagesEdit->toPlainText();
    accept();
}
