#include "infodatawidget.hpp"
#include "../libs/files/esm/inforecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace openck {

InfoDataWidget::InfoDataWidget(void* recordPtr, FormComponents* components,
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

    auto* rec = static_cast<InfoRecord*>(m_recordPtr);

    auto* infoGroup = new QGroupBox(QStringLiteral("Info Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* responseEdit = new QLineEdit(infoGroup);
    responseEdit->setText(rec->responseText);
    infoForm->addRow(QStringLiteral("Response Text:"), responseEdit);

    auto* targetEdit = new QLineEdit(infoGroup);
    targetEdit->setText(QStringLiteral("0x%1").arg(rec->targetId, 8, 16, QChar('0')));
    infoForm->addRow(QStringLiteral("Target ID:"), targetEdit);

    mainLayout->addWidget(infoGroup);

    auto* condGroup = new QGroupBox(QStringLiteral("Condition IDs"), this);
    auto* condLayout = new QVBoxLayout(condGroup);
    auto* condList = new QListWidget(condGroup);
    for (quint32 id : rec->conditionIds)
    {
        condList->addItem(QStringLiteral("0x%1").arg(id, 8, 16, QChar('0')));
    }
    condLayout->addWidget(condList);
    auto* condBtnLayout = new QHBoxLayout();
    auto* addCondBtn = new QPushButton(QStringLiteral("Add"), condGroup);
    auto* removeCondBtn = new QPushButton(QStringLiteral("Remove"), condGroup);
    condBtnLayout->addWidget(addCondBtn);
    condBtnLayout->addWidget(removeCondBtn);
    condLayout->addLayout(condBtnLayout);
    mainLayout->addWidget(condGroup);
}

InfoDataWidget::~InfoDataWidget() = default;

} // namespace openck
