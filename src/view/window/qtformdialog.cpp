#include "qtformdialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
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
    setModal(false);

    m_layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);

    auto* propertiesTab = new QWidget(m_tabs);
    auto* propertiesLayout = new QVBoxLayout(propertiesTab);
    propertiesLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(propertiesTab);
    scroll->setWidgetResizable(true);
    m_grid = new EditorPropertyGrid(scroll);
    scroll->setWidget(m_grid);
    propertiesLayout->addWidget(scroll, 1);

    m_tabs->addTab(propertiesTab, tr("Properties"));

    m_dataTab = new QWidget(m_tabs);
    m_dataTabLayout = new QVBoxLayout(m_dataTab);
    m_dataTabLayout->setContentsMargins(0, 0, 0, 0);
    m_tabs->addTab(m_dataTab, tr("Data"));

    m_layout->addWidget(m_tabs, 1);

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
        m_dataTabLayout->removeWidget(m_customWidget);
        m_customWidget->deleteLater();
    }
    m_customWidget = widget;
    if (widget)
    {
        m_dataTabLayout->addWidget(widget);
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
