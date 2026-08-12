#include "factdatawidget.hpp"
#include "../libs/files/esm/factrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

FactDataWidget::FactDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Faction Data"), this);
    auto* form = new QFormLayout(group);

    auto* nameEdit = new QLineEdit(group);
    nameEdit->setPlaceholderText(QStringLiteral("Faction name"));

    auto* descEdit = new QLineEdit(group);
    descEdit->setPlaceholderText(QStringLiteral("Description"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setRange(0, INT_MAX);

    auto* iconPathEdit = new QLineEdit(group);
    iconPathEdit->setPlaceholderText(QStringLiteral("Icon path"));

    form->addRow(QStringLiteral("Faction Name:"), nameEdit);
    form->addRow(QStringLiteral("Description:"), descEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    form->addRow(QStringLiteral("Icon Path:"), iconPathEdit);
    mainLayout->addWidget(group);

    auto* ranksGroup = new QGroupBox(QStringLiteral("Ranks"), this);
    auto* ranksLayout = new QVBoxLayout(ranksGroup);
    auto* ranksList = new QListWidget(ranksGroup);
    ranksLayout->addWidget(ranksList);
    mainLayout->addWidget(ranksGroup);

    auto* relationsGroup = new QGroupBox(QStringLiteral("Relations"), this);
    auto* relationsLayout = new QVBoxLayout(relationsGroup);
    auto* relationsList = new QListWidget(relationsGroup);
    relationsLayout->addWidget(relationsList);
    mainLayout->addWidget(relationsGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<FactRecord*>(m_recordPtr);
        nameEdit->setText(rec->factionName);
        descEdit->setText(rec->description);
        flagsSpin->setValue(static_cast<int>(rec->flags));
        iconPathEdit->setText(rec->iconPath);
        for (const QString& rank : rec->ranks)
            ranksList->addItem(rank);
        for (quint32 relation : rec->relations)
            relationsList->addItem(QStringLiteral("0x%1").arg(relation, 8, 16, QChar('0')));
    }
}

FactDataWidget::~FactDataWidget() = default;

} // namespace openck
