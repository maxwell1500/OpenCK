#include "dialdatawidget.hpp"
#include "../libs/files/esm/dialrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../widgets/recordfieldparse.hpp"

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
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        mainLayout->addWidget(new QLabel(QStringLiteral("No record data available"), this));
        return;
    }

    auto* dialGroup = new QGroupBox(QStringLiteral("Dialogue Topic"), this);
    auto* dialForm = new QFormLayout(dialGroup);

    auto* topicEdit = new QLineEdit(dialGroup);
    topicEdit->setObjectName(QStringLiteral("topicName"));
    dialForm->addRow(QStringLiteral("Topic Name:"), topicEdit);

    mainLayout->addWidget(dialGroup);

    auto* respGroup = new QGroupBox(QStringLiteral("Response IDs"), this);
    auto* respLayout = new QVBoxLayout(respGroup);
    auto* respList = new QListWidget(respGroup);
    respList->setObjectName(QStringLiteral("responseIds"));
    respLayout->addWidget(respList);
    auto* respBtnLayout = new QHBoxLayout();
    auto* addRespBtn = new QPushButton(QStringLiteral("Add"), respGroup);
    auto* removeRespBtn = new QPushButton(QStringLiteral("Remove"), respGroup);
    respBtnLayout->addWidget(addRespBtn);
    respBtnLayout->addWidget(removeRespBtn);
    respLayout->addLayout(respBtnLayout);
    mainLayout->addWidget(respGroup);

    QObject::connect(addRespBtn, &QPushButton::clicked, this, [respList]() {
        respList->addItem(formatFormId(0));
    });
    QObject::connect(removeRespBtn, &QPushButton::clicked, this, [respList]() {
        auto items = respList->selectedItems();
        for (auto* item : items) delete item;
    });

    loadSession();
}

DialDataWidget::~DialDataWidget() = default;

void DialDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<DialRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("topicName")))
        edit->setText(rec->topicName);
    if (auto* list = findChild<QListWidget*>(QStringLiteral("responseIds")))
    {
        list->clear();
        for (quint32 id : rec->responseIds)
            list->addItem(formatFormId(id));
    }
}

bool DialDataWidget::validateSession(QString* error)
{
    auto* list = findChild<QListWidget*>(QStringLiteral("responseIds"));
    if (!list) return true;
    for (int row = 0; row < list->count(); ++row)
    {
        quint32 ignored = 0;
        if (!parseFormId(list->item(row)->text(), ignored))
        {
            if (error)
                *error = QStringLiteral("Response %1 is not a valid FormID: \"%2\".")
                             .arg(row + 1).arg(list->item(row)->text());
            return false;
        }
    }
    return true;
}

void DialDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<DialRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("topicName")))
        rec->topicName = edit->text();
    if (auto* list = findChild<QListWidget*>(QStringLiteral("responseIds")))
    {
        QVector<quint32> ids;
        ids.reserve(list->count());
        for (int row = 0; row < list->count(); ++row)
        {
            quint32 id = 0;
            if (parseFormId(list->item(row)->text(), id))
                ids.append(id);
        }
        rec->responseIds = ids;
    }
}

} // namespace openck
