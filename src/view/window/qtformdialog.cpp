#include "qtformdialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../widgets/formcomponentwidget.hpp"

namespace openck {

namespace {

// Universal components shown on the "Basic" tab of the form dialog,
// mirroring the real CK's basic-form fields.
bool isBasicComponent(const QString& className)
{
    return className == QStringLiteral("TESFullName")
        || className == QStringLiteral("TESModel")
        || className == QStringLiteral("TESTexture")
        || className == QStringLiteral("TESHealth")
        || className == QStringLiteral("TESValue")
        || className == QStringLiteral("TESWeight")
        || className == QStringLiteral("TESDescription");
}

bool isKeywordComponent(const QString& className)
{
    return className == QStringLiteral("BGSKeywordForm");
}

} // namespace

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

    std::vector<Component*> basic;
    std::vector<Component*> specialized;
    std::vector<Component*> keywords;
    if (m_components)
    {
        const std::vector<Component*>::size_type count = m_components->size();
        for (auto& c : m_components->all())
        {
            const QString cls = c->className();
            if (isKeywordComponent(cls))
                keywords.push_back(c.get());
            else if (isBasicComponent(cls))
                basic.push_back(c.get());
            else
                specialized.push_back(c.get());
        }
    }

    auto addGridTab = [this](const QString& label, const std::vector<Component*>& comps,
                             EditorPropertyGrid** outGrid) -> QWidget* {
        auto* tab = new QWidget(m_tabs);
        auto* tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 0, 0, 0);

        auto* scroll = new QScrollArea(tab);
        scroll->setWidgetResizable(true);
        auto* grid = new EditorPropertyGrid(scroll);
        scroll->setWidget(grid);
        tabLayout->addWidget(scroll, 1);

        m_tabs->addTab(tab, label);
        *outGrid = grid;
        return tab;
    };

    addGridTab(tr("Basic"), basic, &m_basicGrid);

    if (!specialized.empty())
        addGridTab(tr("Components"), specialized, &m_componentsGrid);

    if (!keywords.empty())
        addGridTab(tr("Keywords"), keywords, &m_keywordsGrid);

    m_dataTab = new QWidget(m_tabs);
    m_dataTabLayout = new QVBoxLayout(m_dataTab);
    m_dataTabLayout->setContentsMargins(0, 0, 0, 0);
    m_tabs->addTab(m_dataTab, tr("Data"));

    m_layout->addWidget(m_tabs, 1);

    if (m_basicGrid) m_basicGrid->setComponents(basic);
    if (m_componentsGrid) m_componentsGrid->setComponents(specialized);
    if (m_keywordsGrid) m_keywordsGrid->setComponents(keywords);

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
    if (m_basicGrid) m_basicGrid->apply();
    if (m_componentsGrid) m_componentsGrid->apply();
    if (m_keywordsGrid) m_keywordsGrid->apply();
}

void QtFormDialog::onOk()
{
    onApply();
    accept();
}

} // namespace openck
