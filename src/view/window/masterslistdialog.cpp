#include "masterslistdialog.hpp"
#include "../../model/world/data.hpp"

#include "logger.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

MastersListDialog::MastersListDialog(Data* data, QWidget* parent) :
    QDialog(parent),
    mData(data),
    mastersList(new QListWidget(this)),
    statusLabel(new QLabel(tr("Master Files"), this)),
    addMasterBtn(new QPushButton(tr("Add Master..."), this)),
    removeMasterBtn(new QPushButton(tr("Remove Master"), this))
{
    LOG_DEBUG("MastersListDialog created");
    setWindowTitle(tr("Master Files"));
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(mastersList);

    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(addMasterBtn);
    btnRow->addWidget(removeMasterBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    removeMasterBtn->setEnabled(false);
    connect(mastersList, &QListWidget::currentRowChanged, this, [this](int row) {
        removeMasterBtn->setEnabled(row >= 0);
    });
    connect(addMasterBtn, &QPushButton::clicked, this, &MastersListDialog::onAddMaster);
    connect(removeMasterBtn, &QPushButton::clicked, this, &MastersListDialog::onRemoveMaster);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    populateMasters();
}

MastersListDialog::~MastersListDialog()
{
    LOG_DEBUG("MastersListDialog destroyed");
}

void MastersListDialog::populateMasters()
{
    LOG_DEBUG("Populating masters list");
    mastersList->clear();
    
    if (!mData)
    {
        statusLabel->setText("No data available");
        LOG_WARNING("No data available for masters list");
        return;
    }
    
    const auto masters = mData->getMasters();
    if (masters.isEmpty())
    {
        statusLabel->setText(tr("No master files found"));
        LOG_WARNING("No master files found in current document");
    }
    else
    {
        statusLabel->setText(tr("Found %1 master file(s)").arg(masters.size()));
        LOG_INFO(QString("Found %1 master file(s)").arg(masters.size()));
        
        for (const auto& master : masters)
        {
            mastersList->addItem(QString("%1 (%2 bytes)").arg(master.name).arg(master.size));
        }
    }
}

void MastersListDialog::onAddMaster()
{
    if (!mData) return;

    QString dataDir = mData->getPaths().dataDir.absolutePath();
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Add Master File"), dataDir,
        tr("Plugin Files (*.esm *.esp *.esl);;All Files (*)"));

    if (filePath.isEmpty())
        return;

    QFileInfo fi(filePath);
    const QString fileName = fi.fileName();

    if (mData->hasMaster(fileName))
    {
        QMessageBox::information(this, tr("Add Master"),
            tr("'%1' is already a master file of this plugin.").arg(fileName));
        return;
    }

    mData->addMaster(fileName, static_cast<quint64>(fi.size()));
    populateMasters();
}

void MastersListDialog::onRemoveMaster()
{
    if (!mData) return;

    int row = mastersList->currentRow();
    if (row < 0) return;

    const QVector<MasterData> masters = mData->getMasters();
    if (row >= masters.size()) return;

    const QString masterName = masters[row].name;
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Remove Master"),
        tr("Are you sure you want to remove '%1' from master files?\n\n"
           "Any records referencing FormIDs from this master may become invalid.")
            .arg(masterName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        mData->removeMaster(masterName);
        populateMasters();
    }
}
