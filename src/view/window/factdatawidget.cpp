#include "factdatawidget.hpp"
#include "../libs/files/esm/factrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../widgets/recordfieldparse.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

FactDataWidget::FactDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Faction Data"), this);
    auto* form = new QFormLayout(group);

    auto* nameEdit = new QLineEdit(group);
    nameEdit->setObjectName(QStringLiteral("factionName"));
    nameEdit->setPlaceholderText(QStringLiteral("Faction name"));

    auto* descEdit = new QLineEdit(group);
    descEdit->setObjectName(QStringLiteral("description"));
    descEdit->setPlaceholderText(QStringLiteral("Description"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, INT_MAX);

    auto* iconPathEdit = new QLineEdit(group);
    iconPathEdit->setObjectName(QStringLiteral("iconPath"));
    iconPathEdit->setPlaceholderText(QStringLiteral("Icon path"));

    form->addRow(QStringLiteral("Faction Name:"), nameEdit);
    form->addRow(QStringLiteral("Description:"), descEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    form->addRow(QStringLiteral("Icon Path:"), iconPathEdit);
    mainLayout->addWidget(group);

    auto* ranksGroup = new QGroupBox(QStringLiteral("Ranks"), this);
    auto* ranksLayout = new QVBoxLayout(ranksGroup);
    auto* ranksList = new QListWidget(ranksGroup);
    ranksList->setObjectName(QStringLiteral("ranks"));
    ranksList->setToolTip(QStringLiteral("Rank titles, in order"));
    ranksLayout->addWidget(ranksList);
    auto* ranksBtnLayout = new QHBoxLayout();
    auto* addRankBtn = new QPushButton(QStringLiteral("Add"), ranksGroup);
    auto* removeRankBtn = new QPushButton(QStringLiteral("Remove"), ranksGroup);
    ranksBtnLayout->addWidget(addRankBtn);
    ranksBtnLayout->addWidget(removeRankBtn);
    ranksLayout->addLayout(ranksBtnLayout);
    mainLayout->addWidget(ranksGroup);

    auto* relationsGroup = new QGroupBox(QStringLiteral("Relations"), this);
    auto* relationsLayout = new QVBoxLayout(relationsGroup);
    auto* relationsList = new QListWidget(relationsGroup);
    relationsList->setObjectName(QStringLiteral("relations"));
    relationsLayout->addWidget(relationsList);
    auto* relationsBtnLayout = new QHBoxLayout();
    auto* addRelationBtn = new QPushButton(QStringLiteral("Add"), relationsGroup);
    auto* removeRelationBtn = new QPushButton(QStringLiteral("Remove"), relationsGroup);
    relationsBtnLayout->addWidget(addRelationBtn);
    relationsBtnLayout->addWidget(removeRelationBtn);
    relationsLayout->addLayout(relationsBtnLayout);
    mainLayout->addWidget(relationsGroup);

    QObject::connect(addRankBtn, &QPushButton::clicked, this, [ranksList]() {
        ranksList->addItem(QString());
    });
    QObject::connect(removeRankBtn, &QPushButton::clicked, this, [ranksList]() {
        auto items = ranksList->selectedItems();
        for (auto* item : items) delete item;
    });
    QObject::connect(addRelationBtn, &QPushButton::clicked, this, [relationsList]() {
        relationsList->addItem(formatFormId(0));
    });
    QObject::connect(removeRelationBtn, &QPushButton::clicked, this, [relationsList]() {
        auto items = relationsList->selectedItems();
        for (auto* item : items) delete item;
    });

    loadSession();
}

FactDataWidget::~FactDataWidget() = default;

void FactDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<FactRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("factionName")))
        edit->setText(rec->factionName);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("description")))
        edit->setText(rec->description);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("iconPath")))
        edit->setText(rec->iconPath);
    if (auto* list = findChild<QListWidget*>(QStringLiteral("ranks")))
    {
        list->clear();
        for (const QString& rank : rec->ranks)
            list->addItem(rank);
    }
    if (auto* list = findChild<QListWidget*>(QStringLiteral("relations")))
    {
        list->clear();
        for (quint32 relation : rec->relations)
            list->addItem(formatFormId(relation));
    }
}

bool FactDataWidget::validateSession(QString* error)
{
    // Only the relations list can be malformed; ranks are free-text titles.
    auto* list = findChild<QListWidget*>(QStringLiteral("relations"));
    if (!list) return true;
    for (int row = 0; row < list->count(); ++row)
    {
        quint32 ignored = 0;
        if (!parseFormId(list->item(row)->text(), ignored))
        {
            if (error)
                *error = QStringLiteral("Relation %1 is not a valid FormID: \"%2\".")
                             .arg(row + 1).arg(list->item(row)->text());
            return false;
        }
    }
    return true;
}

void FactDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<FactRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("factionName")))
        rec->factionName = edit->text();
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("description")))
        rec->description = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint32>(spin->value());
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("iconPath")))
        rec->iconPath = edit->text();
    if (auto* list = findChild<QListWidget*>(QStringLiteral("ranks")))
    {
        QVector<QString> ranks;
        ranks.reserve(list->count());
        for (int row = 0; row < list->count(); ++row)
            ranks.append(list->item(row)->text());
        rec->ranks = ranks;
    }
    if (auto* list = findChild<QListWidget*>(QStringLiteral("relations")))
    {
        QVector<quint32> relations;
        relations.reserve(list->count());
        for (int row = 0; row < list->count(); ++row)
        {
            quint32 id = 0;
            if (parseFormId(list->item(row)->text(), id))
                relations.append(id);
        }
        rec->relations = relations;
    }
}

} // namespace openck
