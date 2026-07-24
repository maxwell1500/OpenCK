#include "viewmediator.hpp"

#include <QFileDialog>
#include <QMessageBox>

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

    QString fileName = saveDialog.getSaveFileName(
        nullptr, "New Plugin File", "", "Elder Scrolls Plugin files (*.esp)");
    
    if (!fileName.isEmpty())
    {
        emit addDocument(QStringList(), fileName, true);
    }
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


