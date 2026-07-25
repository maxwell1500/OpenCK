#include "celldatawidget.hpp"
#include "../libs/files/esm/cellrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

CellDataWidget::CellDataWidget(void* recordPtr, FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        mainLayout->addWidget(new QLabel(QStringLiteral("No record data available"), this));
        return;
    }

    auto* rec = static_cast<CellRecord*>(m_recordPtr);

    auto* cellGroup = new QGroupBox(QStringLiteral("Cell Data"), this);
    auto* cellForm = new QFormLayout(cellGroup);

    auto* xSpin = new QSpinBox(cellGroup);
    xSpin->setRange(-99999, 99999);
    xSpin->setValue(static_cast<int>(rec->cellX));
    cellForm->addRow(QStringLiteral("Cell X:"), xSpin);

    auto* ySpin = new QSpinBox(cellGroup);
    ySpin->setRange(-99999, 99999);
    ySpin->setValue(static_cast<int>(rec->cellY));
    cellForm->addRow(QStringLiteral("Cell Y:"), ySpin);

    auto* ownerEdit = new QLineEdit(cellGroup);
    ownerEdit->setText(QStringLiteral("0x%1").arg(rec->owner, 8, 16, QChar('0')));
    cellForm->addRow(QStringLiteral("Owner:"), ownerEdit);

    auto* lockSpin = new QSpinBox(cellGroup);
    lockSpin->setRange(0, 100);
    lockSpin->setValue(static_cast<int>(rec->lockLevel));
    cellForm->addRow(QStringLiteral("Lock Level:"), lockSpin);

    mainLayout->addWidget(cellGroup);
}

CellDataWidget::~CellDataWidget() = default;

} // namespace openck
