#include "viewmediator.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

class NewPluginDialog : public QDialog
{
public:
    NewPluginDialog(const QString& filePath, const QString& dataPath, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("New Plugin"));
        setMinimumWidth(560);

        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout();
        auto* fileLabel = new QLabel(filePath, this);
        fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        form->addRow(QStringLiteral("File:"), fileLabel);

        mGame = new QComboBox(this);
        mGame->addItem(QStringLiteral("Morrowind"), int(GameFormat::Game::Morrowind));
        mGame->addItem(QStringLiteral("Oblivion"), int(GameFormat::Game::Oblivion));
        mGame->addItem(QStringLiteral("Skyrim"), int(GameFormat::Game::Skyrim));
        mGame->addItem(QStringLiteral("Fallout 4"), int(GameFormat::Game::Fallout4));
        mGame->addItem(QStringLiteral("Starfield"), int(GameFormat::Game::Starfield));
        mGame->setCurrentIndex(4);
        form->addRow(QStringLiteral("Game:"), mGame);

        mFileType = new QComboBox(this);
        mFileType->addItem(QStringLiteral("Plugin (.esp)"), false);
        mFileType->addItem(QStringLiteral("Light master (.esl)"), true);
        form->addRow(QStringLiteral("Type:"), mFileType);

        mAuthor = new QLineEdit(this);
        mAuthor->setPlaceholderText(QStringLiteral("Optional author name"));
        form->addRow(QStringLiteral("Author:"), mAuthor);

        mNextId = new QSpinBox(this);
        mNextId->setRange(0x800, 0xFFFFF);
        mNextId->setValue(0x800);
        form->addRow(QStringLiteral("Next local FormID:"), mNextId);
        layout->addLayout(form);

        auto* masterTitle = new QLabel(QStringLiteral("Active master order"), this);
        layout->addWidget(masterTitle);
        mMasters = new QListWidget(this);
        mMasters->setMinimumHeight(150);
        layout->addWidget(mMasters);

        auto* masterButtons = new QHBoxLayout();
        auto* addMaster = new QPushButton(QStringLiteral("Add..."), this);
        auto* removeMaster = new QPushButton(QStringLiteral("Remove"), this);
        auto* moveUp = new QPushButton(QStringLiteral("Up"), this);
        auto* moveDown = new QPushButton(QStringLiteral("Down"), this);
        masterButtons->addWidget(addMaster);
        masterButtons->addWidget(removeMaster);
        masterButtons->addStretch();
        masterButtons->addWidget(moveUp);
        masterButtons->addWidget(moveDown);
        layout->addLayout(masterButtons);

        connect(addMaster, &QPushButton::clicked, this, [this, dataPath] {
            const QString path = QFileDialog::getOpenFileName(this,
                QStringLiteral("Select active master"), dataPath,
                QStringLiteral("Bethesda master files (*.esm *.esl *.esp);;All files (*)"));
            if (path.isEmpty()) return;
            mMasters->addItem(QFileInfo(path).fileName());
        });
        connect(removeMaster, &QPushButton::clicked, this, [this] {
            delete mMasters->takeItem(mMasters->currentRow());
        });
        connect(moveUp, &QPushButton::clicked, this, [this] {
            const int row = mMasters->currentRow();
            if (row <= 0) return;
            QListWidgetItem* item = mMasters->takeItem(row);
            mMasters->insertItem(row - 1, item);
            mMasters->setCurrentRow(row - 1);
        });
        connect(moveDown, &QPushButton::clicked, this, [this] {
            const int row = mMasters->currentRow();
            if (row < 0 || row >= mMasters->count() - 1) return;
            QListWidgetItem* item = mMasters->takeItem(row);
            mMasters->insertItem(row + 1, item);
            mMasters->setCurrentRow(row + 1);
        });

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    NewPluginOptions options() const
    {
        NewPluginOptions result;
        result.game = static_cast<GameFormat::Game>(mGame->currentData().toInt());
        result.author = mAuthor->text().trimmed();
        result.nextObjectId = static_cast<quint32>(mNextId->value());
        result.lightMaster = mFileType->currentData().toBool();
        for (int i = 0; i < mMasters->count(); ++i)
            result.masters.append(MasterData(mMasters->item(i)->text()));
        return result;
    }

private:
    QComboBox* mGame = nullptr;
    QComboBox* mFileType = nullptr;
    QLineEdit* mAuthor = nullptr;
    QSpinBox* mNextId = nullptr;
    QListWidget* mMasters = nullptr;
};

}

ViewMediator::ViewMediator(DocumentMediator& docMed) : 
    docMed(docMed)
{   
    w.reset(new MainWindow());

    connect(w.get(), &MainWindow::actionData_triggered, this, &ViewMediator::showDataDialog);
    connect(w.get(), &MainWindow::actionSave_triggered, this, &ViewMediator::showSaveDialog);
    connect(w.get(), &MainWindow::actionNewPlugin_triggered, this, &ViewMediator::showNewPluginDialog);
    connect(w.get(), &MainWindow::actionSaveAs_triggered, this, &ViewMediator::showSaveAsDialog);
    connect(w.get(), &MainWindow::actionClosePlugin_triggered, this, &ViewMediator::closeCurrentPlugin);
    connect(w.get(), &MainWindow::actionSettings_triggered, this, &ViewMediator::showGmstDialog);

    connect(&docMed, &DocumentMediator::loadRequest, 
        &loader, &LoaderView::add);
    
    connect(&docMed, &DocumentMediator::loadingStopped, 
        &loader, &LoaderView::loadingStopped);

    connect(&docMed, &DocumentMediator::nextStage,
        &loader, &LoaderView::nextStage);

    connect(&docMed, &DocumentMediator::nextRecord,
        &loader, &LoaderView::nextRecord);

    connect(&loader, &LoaderView::close,
        &docMed, &DocumentMediator::removeDocument);

    connect(&docMed, &DocumentMediator::loadingStopped, this, [this](Document*, bool, const QString&) {
        Document* current = this->docMed.getCurrentDocument();
        if (current)
        {
            w->setDocument(current);
            w->setData(&current->getData());
        }
    });

    w->show();
}

ViewMediator::~ViewMediator()
{
}

void ViewMediator::setUpDataDialog(const QString& path)
{
    dataPath = path;
}

void ViewMediator::showDataDialog()
{
    dataDlg.reset(new DataDialog());
    dataDlg->setWindowFlags(dataDlg->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dataDlg->setUp(dataPath);

    connect(dataDlg.get(), &DataDialog::addDocument, this, &ViewMediator::dataDialogAccepted);

    // If the data table encountered errors (unreadable ESM files, missing
    // directory, etc.), surface them once the dialog is visible. Previously
    // these were shown via blocking msgBoxCritical() calls from inside
    // DataTable::DataTable, which ran BEFORE dataDlg->exec() and could leave
    // an orphan modal box on screen that the user couldn't see.
    QStringList loadErrors;
    if (auto* model = dataDlg->getDataTable())
    {
        loadErrors = model->getLoadErrors();
    }

    if (loadErrors.isEmpty())
    {
        dataDlg->exec();
    }
    else
    {
        // Show the dialog first, then a single modal summary on top of it
        // so the user can see the context.
        dataDlg->show();
        QString combined = loadErrors.join('\n');
        if (loadErrors.size() > 10)
        {
            combined += QString("\n... and %1 more (see log for full list)").arg(loadErrors.size() - 10);
        }
        QMessageBox::warning(dataDlg.get(), "Plugin Load Issues", combined);
        dataDlg->exec();
    }
}

void ViewMediator::dataDialogAccepted(const QStringList& files, const QString& savePath, bool isNew)
{
    emit addDocument(files, savePath, isNew);
}

void ViewMediator::showSaveDialog()
{
    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setModal(true);
    saveDialog.setDirectory(dataPath);

    emit saveDocument(saveDialog.getSaveFileName(
        nullptr, "Save Plugin File", "", "Elder Scrolls Plugin fies (*.esp)")
    );
}

void ViewMediator::showNewPluginDialog()
{
    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setModal(true);
    saveDialog.setDirectory(dataPath);

    const QString fileName = saveDialog.getSaveFileName(
        nullptr, tr("New Plugin File"), QString(),
        tr("Bethesda plugin files (*.esp *.esl);;All files (*)"));
    if (fileName.isEmpty()) return;

    NewPluginDialog dialog(fileName, dataPath, w.get());
    if (dialog.exec() != QDialog::Accepted) return;

    docMed.clearFiles();
    docMed.addDocument(QStringList(), fileName, true, dialog.options());
}

void ViewMediator::showSaveAsDialog()
{
    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setModal(true);
    saveDialog.setDirectory(dataPath);

    Document* current = docMed.getCurrentDocument();
    QString defaultName = current ? current->getSavePath() : dataPath;

    emit saveDocument(saveDialog.getSaveFileName(
        nullptr, "Save Plugin As", defaultName, "Elder Scrolls Plugin files (*.esp)")
    );
}

void ViewMediator::closeCurrentPlugin()
{
    Document* current = docMed.getCurrentDocument();
    if (current)
    {
        docMed.removeDocument(current);
        w->setDocument(nullptr);
        w->setData(nullptr);
    }
    else
    {
        QMessageBox::information(nullptr, "Close Plugin", "No plugin is currently loaded.");
    }
}

void ViewMediator::showGmstDialog()
{
    Document* document = docMed.getCurrentDocument();

    if (document)
    {
        gmstDlg.reset(new GmstDialog());
        gmstDlg->setUp(document);
        gmstDlg->exec();
    }
    else
    {
        QMessageBox::information(
            nullptr,
            "Game Settings",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data, then you can view and edit game settings."
        );
    }
}


