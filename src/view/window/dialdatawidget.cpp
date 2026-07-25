#include "dialdatawidget.hpp"
#include "../libs/files/esm/dialrecord.hpp"
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

DialDataWidget::DialDataWidget(void* recordPtr, FormComponents* components,
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

    auto* rec = static_cast<DialRecord*>(m_recordPtr);

    auto* dialGroup = new QGroupBox(QStringLiteral("Dialogue Topic"), this);
    auto* dialForm = new QFormLayout(dialGroup);

    auto* topicEdit = new QLineEdit(dialGroup);
    topicEdit->setText(rec->topicName);
    dialForm->addRow(QStringLiteral("Topic Name:"), topicEdit);

    mainLayout->addWidget(dialGroup);

    auto* respGroup = new QGroupBox(QStringLiteral("Response IDs"), this);
    auto* respLayout = new QVBoxLayout(respGroup);
    auto* respList = new QListWidget(respGroup);
    for (quint32 id : rec->responseIds)
    {
        respList->addItem(QStringLiteral("0x%1").arg(id, 8, 16, QChar('0')));
    }
    respLayout->addWidget(respList);
    auto* respBtnLayout = new QHBoxLayout();
    auto* addRespBtn = new QPushButton(QStringLiteral("Add"), respGroup);
    auto* removeRespBtn = new QPushButton(QStringLiteral("Remove"), respGroup);
    respBtnLayout->addWidget(addRespBtn);
    respBtnLayout->addWidget(removeRespBtn);
    respLayout->addLayout(respBtnLayout);
    mainLayout->addWidget(respGroup);
}

DialDataWidget::~DialDataWidget() = default;

} // namespace openck
