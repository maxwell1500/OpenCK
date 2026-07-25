#include "racedatawidget.hpp"
#include "../libs/files/esm/racerecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

RaceDataWidget::RaceDataWidget(void* recordPtr, FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        auto* lbl = new QLabel(QStringLiteral("No record data available"), this);
        mainLayout->addWidget(lbl);
        return;
    }

    auto* rec = static_cast<RaceRecord*>(m_recordPtr);

    auto* dataGroup = new QGroupBox(QStringLiteral("Race Data"), this);
    auto* dataForm = new QFormLayout(dataGroup);

    auto* flagsSpin = new QSpinBox(dataGroup);
    flagsSpin->setRange(0, INT_MAX);
    flagsSpin->setValue(static_cast<int>(rec->raceFlags));
    dataForm->addRow(QStringLiteral("Race Flags:"), flagsSpin);

    mainLayout->addWidget(dataGroup);

    auto* listGroup = new QGroupBox(QStringLiteral("NPC Variables"), this);
    auto* listLayout = new QVBoxLayout(listGroup);
    auto* varList = new QListWidget(listGroup);
    for (quint32 id : rec->npcVariables)
        varList->addItem(QStringLiteral("0x%1").arg(id, 8, 16, QChar('0')));
    listLayout->addWidget(varList);
    auto* varBtnLayout = new QHBoxLayout();
    auto* addVarBtn = new QPushButton(QStringLiteral("Add"), listGroup);
    auto* rmVarBtn = new QPushButton(QStringLiteral("Remove"), listGroup);
    varBtnLayout->addWidget(addVarBtn);
    varBtnLayout->addWidget(rmVarBtn);
    varBtnLayout->addStretch();
    listLayout->addLayout(varBtnLayout);
    mainLayout->addWidget(listGroup);

    QObject::connect(addVarBtn, &QPushButton::clicked, this, [varList]() {
        varList->addItem(QStringLiteral("0x00000000"));
    });
    QObject::connect(rmVarBtn, &QPushButton::clicked, this, [varList]() {
        auto items = varList->selectedItems();
        for (auto* item : items) delete item;
    });
}

RaceDataWidget::~RaceDataWidget() = default;

} // namespace openck
