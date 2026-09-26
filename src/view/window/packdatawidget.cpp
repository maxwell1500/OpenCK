#include "packdatawidget.hpp"

#include "../libs/files/esm/Packagerecord.hpp"
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

PackDataWidget::PackDataWidget(void* recordPtr,
                               openck::FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Package Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* typeSpin = new QSpinBox(infoGroup);
    typeSpin->setObjectName(QStringLiteral("packageType"));
    typeSpin->setRange(0, INT_MAX);
    auto* targetTypeSpin = new QSpinBox(infoGroup);
    targetTypeSpin->setObjectName(QStringLiteral("targetType"));
    targetTypeSpin->setRange(0, INT_MAX);
    auto* flagsSpin = new QSpinBox(infoGroup);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, INT_MAX);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Package Type:"), typeSpin);
    infoForm->addRow(QStringLiteral("Target Type:"), targetTypeSpin);
    infoForm->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(infoGroup);

    auto* targetsGroup = new QGroupBox(QStringLiteral("Targets"), this);
    auto* targetsLayout = new QVBoxLayout(targetsGroup);
    auto* targetsList = new QListWidget(targetsGroup);
    targetsList->setObjectName(QStringLiteral("targetIds"));
    targetsLayout->addWidget(targetsList);
    auto* targetsBtnLayout = new QHBoxLayout();
    auto* addTargetBtn = new QPushButton(QStringLiteral("Add"), targetsGroup);
    auto* removeTargetBtn = new QPushButton(QStringLiteral("Remove"), targetsGroup);
    targetsBtnLayout->addWidget(addTargetBtn);
    targetsBtnLayout->addWidget(removeTargetBtn);
    targetsLayout->addLayout(targetsBtnLayout);
    mainLayout->addWidget(targetsGroup);

    QObject::connect(addTargetBtn, &QPushButton::clicked, this, [targetsList]() {
        targetsList->addItem(openck::formatFormId(0));
    });
    QObject::connect(removeTargetBtn, &QPushButton::clicked, this, [targetsList]() {
        auto items = targetsList->selectedItems();
        for (auto* item : items) delete item;
    });

    loadSession();
}

PackDataWidget::~PackDataWidget() = default;

void PackDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<PackageRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        edit->setText(rec->editorId);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("packageType")))
        spin->setValue(static_cast<int>(rec->packageType));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("targetType")))
        spin->setValue(static_cast<int>(rec->targetType));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
    if (auto* list = findChild<QListWidget*>(QStringLiteral("targetIds")))
    {
        list->clear();
        for (quint32 id : rec->targetIds)
            list->addItem(openck::formatFormId(id));
    }
}

bool PackDataWidget::validateSession(QString* error)
{
    auto* list = findChild<QListWidget*>(QStringLiteral("targetIds"));
    if (!list) return true;
    for (int row = 0; row < list->count(); ++row)
    {
        quint32 ignored = 0;
        if (!openck::parseFormId(list->item(row)->text(), ignored))
        {
            if (error)
                *error = QStringLiteral("Target %1 is not a valid FormID: \"%2\".")
                             .arg(row + 1).arg(list->item(row)->text());
            return false;
        }
    }
    return true;
}

void PackDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<PackageRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        rec->editorId = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("packageType")))
        rec->packageType = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("targetType")))
        rec->targetType = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint32>(spin->value());
    if (auto* list = findChild<QListWidget*>(QStringLiteral("targetIds")))
    {
        QVector<quint32> ids;
        ids.reserve(list->count());
        for (int row = 0; row < list->count(); ++row)
        {
            quint32 id = 0;
            if (openck::parseFormId(list->item(row)->text(), id))
                ids.append(id);
        }
        rec->targetIds = ids;
    }
}
