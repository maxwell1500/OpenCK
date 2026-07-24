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

    auto* layout = new QVBoxLayout(this);

    // The property grid scrolls so it can grow as the user adds
    // records with many components without the dialog becoming
    // unmanageable.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_grid = new EditorPropertyGrid(scroll);
    scroll->setWidget(m_grid);
    layout->addWidget(scroll, 1);

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
    layout->addWidget(buttons);

    connect(applyBtn, &QPushButton::clicked, this, &QtFormDialog::onApply);
    connect(okBtn, &QPushButton::clicked, this, &QtFormDialog::onOk);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QtFormDialog::~QtFormDialog() = default;

void QtFormDialog::onApply()
{
    // Pull any pending edits back into the components. The editor
    // widgets already push values into storage via their signal
    // connections, so this method is mostly a no-op today; it
    // exists so explicit Apply behaves the same way as the user
    // pressing OK.
    if (m_grid) m_grid->apply();
}

void QtFormDialog::onOk()
{
    if (m_grid) m_grid->apply();
    accept();
}

} // namespace openck
