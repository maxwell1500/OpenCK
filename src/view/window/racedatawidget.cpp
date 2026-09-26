#include "racedatawidget.hpp"
#include "../libs/files/esm/racerecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../widgets/recordfieldparse.hpp"

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
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    if (!m_recordPtr)
    {
        auto* lbl = new QLabel(QStringLiteral("No record data available"), this);
        mainLayout->addWidget(lbl);
        return;
    }

    auto* dataGroup = new QGroupBox(QStringLiteral("Race Data"), this);
    auto* dataForm = new QFormLayout(dataGroup);

    auto* flagsSpin = new QSpinBox(dataGroup);
    flagsSpin->setObjectName(QStringLiteral("raceFlags"));
    flagsSpin->setRange(0, INT_MAX);
    dataForm->addRow(QStringLiteral("Race Flags:"), flagsSpin);

    mainLayout->addWidget(dataGroup);

    auto* listGroup = new QGroupBox(QStringLiteral("NPC Variables"), this);
    auto* listLayout = new QVBoxLayout(listGroup);
    auto* varList = new QListWidget(listGroup);
    varList->setObjectName(QStringLiteral("npcVariables"));
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
        varList->addItem(formatFormId(0));
    });
    QObject::connect(rmVarBtn, &QPushButton::clicked, this, [varList]() {
        auto items = varList->selectedItems();
        for (auto* item : items) delete item;
    });

    loadSession();
}

RaceDataWidget::~RaceDataWidget() = default;

void RaceDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<RaceRecord*>(m_recordPtr);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("raceFlags")))
        spin->setValue(static_cast<int>(rec->raceFlags));
    if (auto* list = findChild<QListWidget*>(QStringLiteral("npcVariables")))
    {
        list->clear();
        for (quint32 id : rec->npcVariables)
            list->addItem(formatFormId(id));
    }
}

bool RaceDataWidget::validateSession(QString* error)
{
    auto* list = findChild<QListWidget*>(QStringLiteral("npcVariables"));
    if (!list) return true;
    for (int row = 0; row < list->count(); ++row)
    {
        quint32 ignored = 0;
        if (!parseFormId(list->item(row)->text(), ignored))
        {
            if (error)
                *error = QStringLiteral("NPC Variable %1 is not a valid FormID: \"%2\".")
                             .arg(row + 1).arg(list->item(row)->text());
            return false;
        }
    }
    return true;
}

void RaceDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<RaceRecord*>(m_recordPtr);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("raceFlags")))
        rec->raceFlags = static_cast<quint32>(spin->value());
    if (auto* list = findChild<QListWidget*>(QStringLiteral("npcVariables")))
    {
        QVector<quint32> ids;
        ids.reserve(list->count());
        for (int row = 0; row < list->count(); ++row)
        {
            quint32 id = 0;
            // validateSession() already rejected anything unparseable, so a
            // failure here can only mean the list changed underneath us; skip
            // rather than write a bogus id.
            if (parseFormId(list->item(row)->text(), id))
                ids.append(id);
        }
        rec->npcVariables = ids;
    }
}

} // namespace openck
