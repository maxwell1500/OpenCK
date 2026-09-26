#include "locationdatawidget.hpp"

#include "../libs/files/esm/locationrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

LocationDataWidget::LocationDataWidget(void* recordPtr,
                                       openck::FormComponents* components,
                                       QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Location Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* nameEdit = new QLineEdit(infoGroup);
    nameEdit->setObjectName(QStringLiteral("name"));
    nameEdit->setPlaceholderText(QStringLiteral("Location Name"));

    auto* parentSpin = new QSpinBox(infoGroup);
    parentSpin->setObjectName(QStringLiteral("parentId"));
    parentSpin->setRange(0, INT_MAX);
    auto* xSpin = new QSpinBox(infoGroup);
    xSpin->setObjectName(QStringLiteral("x"));
    xSpin->setRange(0, INT_MAX);
    auto* ySpin = new QSpinBox(infoGroup);
    ySpin->setObjectName(QStringLiteral("y"));
    ySpin->setRange(0, INT_MAX);
    auto* zSpin = new QSpinBox(infoGroup);
    zSpin->setObjectName(QStringLiteral("z"));
    zSpin->setRange(0, INT_MAX);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Location Name:"), nameEdit);
    infoForm->addRow(QStringLiteral("Parent ID:"), parentSpin);
    infoForm->addRow(QStringLiteral("X:"), xSpin);
    infoForm->addRow(QStringLiteral("Y:"), ySpin);
    infoForm->addRow(QStringLiteral("Z:"), zSpin);
    mainLayout->addWidget(infoGroup);

    // Linked references (XNAM groups + LNAM targets), read-only listing.
    auto* linksGroup = new QGroupBox(QStringLiteral("Linked References"), this);
    auto* linksLayout = new QVBoxLayout(linksGroup);
    auto* linksView = new QPlainTextEdit(linksGroup);
    linksView->setObjectName(QStringLiteral("linkedRefs"));
    linksView->setReadOnly(true);
    linksView->setMaximumHeight(120);
    linksLayout->addWidget(linksView);
    mainLayout->addWidget(linksGroup);

    loadSession();
}

LocationDataWidget::~LocationDataWidget() = default;

void LocationDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<LocationRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        edit->setText(rec->editorId);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("name")))
        edit->setText(rec->locationName);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("parentId")))
        spin->setValue(static_cast<int>(rec->parentId));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("x")))
        spin->setValue(static_cast<int>(rec->x));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("y")))
        spin->setValue(static_cast<int>(rec->y));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("z")))
        spin->setValue(static_cast<int>(rec->z));

    // The linked-reference listing is a read-only view of the XNAM groups; it
    // is never written back, because the group structure is owned by the
    // component layer.
    if (auto* view = findChild<QPlainTextEdit*>(QStringLiteral("linkedRefs")))
    {
        QString text;
        for (const LocationRecord::LinkedRef& group : rec->linkedRefs)
        {
            if (!text.isEmpty())
                text += QStringLiteral("\n");
            text += QStringLiteral("RefType %1:").arg(group.refTypeId, 8, 16, QLatin1Char('0'));
            for (quint32 linkedId : group.linkedIds)
                text += QStringLiteral(" %1").arg(linkedId, 8, 16, QLatin1Char('0'));
        }
        if (text.isEmpty())
            text = QStringLiteral("(none)");
        view->setPlainText(text);
    }
}

bool LocationDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    return true;
}

void LocationDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<LocationRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        rec->editorId = edit->text();
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("name")))
        rec->locationName = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("parentId")))
        rec->parentId = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("x")))
        rec->x = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("y")))
        rec->y = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("z")))
        rec->z = static_cast<quint32>(spin->value());
    // linkedRefs is intentionally not written back; see loadSession().
}
