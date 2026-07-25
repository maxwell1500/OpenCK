#include "qtformdialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "../widgets/formcomponentwidget.hpp"

namespace openck {

QtFormDialog::QtFormDialog(const QString& formIdKey, FormComponents* components,
                           QWidget* parent)
    : QDialog(parent)
    , m_formIdKey(formIdKey)
    , m_components(components)
{
    setWindowTitle(QStringLiteral("Form — %1").arg(formIdKey));
    resize(640, 480);

    m_layout = new QVBoxLayout(this);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_grid = new EditorPropertyGrid(scroll);
    scroll->setWidget(m_grid);
    m_layout->addWidget(scroll, 1);

    if (m_components)
    {
        std::vector<Component*> componentPtrs;
        componentPtrs.reserve(m_components->size());
        for (const auto& c : m_components->all())
        {
            componentPtrs.push_back(c.get());
        }
        m_grid->setComponents(componentPtrs);
    }

    auto* buttons = new QDialogButtonBox(this);
    auto* applyBtn = buttons->addButton(QStringLiteral("Apply"),
        QDialogButtonBox::ApplyRole);
    auto* okBtn = buttons->addButton(QDialogButtonBox::Ok);
    auto* cancelBtn = buttons->addButton(QDialogButtonBox::Cancel);
    m_layout->addWidget(buttons);

    connect(applyBtn, &QPushButton::clicked, this, &QtFormDialog::onApply);
    connect(okBtn, &QPushButton::clicked, this, &QtFormDialog::onOk);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QtFormDialog::~QtFormDialog() = default;

void QtFormDialog::setCustomWidget(QWidget* widget)
{
    if (m_customWidget)
    {
        m_layout->removeWidget(m_customWidget);
        m_customWidget->deleteLater();
    }
    m_customWidget = widget;
    if (widget)
    {
        m_layout->insertWidget(m_layout->count() - 1, widget);
    }
}

void QtFormDialog::onApply()
{
    if (m_grid) m_grid->apply();
}

void QtFormDialog::onOk()
{
    if (m_grid) m_grid->apply();
    accept();
}

} // namespace openck
