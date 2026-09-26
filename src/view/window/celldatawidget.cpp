#include "celldatawidget.hpp"
#include "../libs/files/esm/cellrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../widgets/recordfieldparse.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

namespace {
// CellRecord stores cellX and cellY as quint32. The spin boxes used to accept
// -99999..99999, so a negative coordinate became a huge unsigned value on the
// way into the record. Interior cell coordinates are non-negative anyway.
constexpr int kCellCoordMax = 99999;
}

CellDataWidget::CellDataWidget(void* recordPtr, FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        mainLayout->addWidget(new QLabel(QStringLiteral("No record data available"), this));
        return;
    }

    auto* cellGroup = new QGroupBox(QStringLiteral("Cell Data"), this);
    auto* cellForm = new QFormLayout(cellGroup);

    auto* xSpin = new QSpinBox(cellGroup);
    xSpin->setObjectName(QStringLiteral("cellX"));
    xSpin->setRange(0, kCellCoordMax);
    cellForm->addRow(QStringLiteral("Cell X:"), xSpin);

    auto* ySpin = new QSpinBox(cellGroup);
    ySpin->setObjectName(QStringLiteral("cellY"));
    ySpin->setRange(0, kCellCoordMax);
    cellForm->addRow(QStringLiteral("Cell Y:"), ySpin);

    auto* ownerEdit = new QLineEdit(cellGroup);
    ownerEdit->setObjectName(QStringLiteral("owner"));
    ownerEdit->setPlaceholderText(QStringLiteral("Owner Form ID (hex)"));
    cellForm->addRow(QStringLiteral("Owner:"), ownerEdit);

    auto* lockSpin = new QSpinBox(cellGroup);
    lockSpin->setObjectName(QStringLiteral("lockLevel"));
    lockSpin->setRange(0, 100);
    cellForm->addRow(QStringLiteral("Lock Level:"), lockSpin);

    mainLayout->addWidget(cellGroup);

    loadSession();
}

CellDataWidget::~CellDataWidget() = default;

void CellDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<CellRecord*>(m_recordPtr);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("cellX")))
        spin->setValue(static_cast<int>(rec->cellX));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("cellY")))
        spin->setValue(static_cast<int>(rec->cellY));
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("owner")))
        edit->setText(formatFormId(rec->owner));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("lockLevel")))
        spin->setValue(static_cast<int>(rec->lockLevel));
}

bool CellDataWidget::validateSession(QString* error)
{
    auto* edit = findChild<QLineEdit*>(QStringLiteral("owner"));
    if (!edit) return true;
    quint32 ignored = 0;
    if (!parseFormId(edit->text(), ignored))
    {
        if (error)
            *error = QStringLiteral("Owner is not a valid FormID: \"%1\".").arg(edit->text());
        return false;
    }
    return true;
}

void CellDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<CellRecord*>(m_recordPtr);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("cellX")))
        rec->cellX = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("cellY")))
        rec->cellY = static_cast<quint32>(spin->value());
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("owner")))
    {
        quint32 id = 0;
        if (parseFormId(edit->text(), id))
            rec->owner = id;
    }
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("lockLevel")))
        rec->lockLevel = static_cast<quint32>(spin->value());
}

} // namespace openck
