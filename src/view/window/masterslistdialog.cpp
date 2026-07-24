#include "masterslistdialog.hpp"
#include "../../model/world/data.hpp"

#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

MastersListDialog::MastersListDialog(Data* data, QWidget* parent) :
    QDialog(parent),
    mData(data),
    mastersList(new QListWidget(this)),
    statusLabel(new QLabel("Master Files", this))
{
    LOG_DEBUG("MastersListDialog created");
    setWindowTitle("Master Files");
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(mastersList);
    
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
    
    // Get master files from the data
    const auto& metaData = mData->getMetaData().getRecords();
    if (metaData.isEmpty())
    {
        statusLabel->setText("No master files found");
        LOG_WARNING("No master files found in current document");
    }
    else
    {
        statusLabel->setText(QString("Found %1 master file(s)").arg(metaData.size()));
        LOG_INFO(QString("Found %1 master file(s)").arg(metaData.size()));
        
        for (const auto& master : metaData)
        {
            mastersList->addItem(master.get().editorId);
        }
    }
}
