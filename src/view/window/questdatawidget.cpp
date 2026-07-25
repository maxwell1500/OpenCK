#include "questdatawidget.hpp"
#include "../libs/files/esm/questrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

QuestDataWidget::QuestDataWidget(void* recordPtr, FormComponents* components,
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

    auto* rec = static_cast<QuestRecord*>(m_recordPtr);

    auto* infoGroup = new QGroupBox(QStringLiteral("Quest Info"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* descEdit = new QLineEdit(infoGroup);
    descEdit->setText(rec->questDesc);
    infoForm->addRow(QStringLiteral("Description:"), descEdit);

    auto* typeSpin = new QSpinBox(infoGroup);
    typeSpin->setRange(0, INT_MAX);
    typeSpin->setValue(static_cast<int>(rec->questType));
    infoForm->addRow(QStringLiteral("Quest Type:"), typeSpin);

    mainLayout->addWidget(infoGroup);

    auto* stageGroup = new QGroupBox(QStringLiteral("Quest Stages"), this);
    auto* stageLayout = new QVBoxLayout(stageGroup);
    auto* stageList = new QListWidget(stageGroup);
    for (int i = 0; i < rec->stageIds.size() && i < rec->stageDescriptions.size(); ++i)
    {
        stageList->addItem(QStringLiteral("%1: %2")
            .arg(rec->stageIds[i])
            .arg(rec->stageDescriptions[i]));
    }
    stageLayout->addWidget(stageList);
    mainLayout->addWidget(stageGroup);
}

QuestDataWidget::~QuestDataWidget() = default;

} // namespace openck
