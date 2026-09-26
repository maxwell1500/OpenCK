#include "qtformdialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <utility>

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
                           QWidget* parent,
                           std::function<void(const FormComponents&)> commit,
                           Data* data,
                           std::unique_ptr<RecordEditSession> session)
    : QDialog(parent)
    , m_formIdKey(formIdKey)
    , m_sourceComponents(components)
    , m_data(data)
    , m_commit(std::move(commit))
    , m_session(std::move(session))
{
    if (m_session)
    {
        // The session owns the working copy, so the grid and any custom data
        // widget edit the same record and commit() sees both sets of changes.
        m_components = m_session->workingComponents();
    }
    else
    {
        m_ownedWorkingComponents = components ? *components : FormComponents();
        m_components = &m_ownedWorkingComponents;
    }
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
        auto* grid = new EditorPropertyGrid(scroll, m_data);
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
    connect(cancelBtn, &QPushButton::clicked, this, &QtFormDialog::reject);
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
    m_customSession = widget ? dynamic_cast<FormDataWidget*>(widget) : nullptr;
    if (widget)
    {
        m_dataTabLayout->addWidget(widget);
        if (m_customSession)
            m_customSession->loadSession();
    }
}

void QtFormDialog::onApply()
{
    if (m_basicGrid) m_basicGrid->apply();
    if (m_componentsGrid) m_componentsGrid->apply();
    if (m_keywordsGrid) m_keywordsGrid->apply();
    if (m_customSession)
    {
        QString error;
        if (!m_customSession->validateSession(&error))
        {
            QMessageBox::warning(this, tr("Invalid Edit"),
                error.isEmpty() ? tr("This record cannot be saved as entered.")
                                : error);
            return;
        }
        m_customSession->applySession();
    }
    commitChanges();
}

bool QtFormDialog::commitChanges()
{
    if (m_session)
    {
        // The session already holds the pre-edit copy, so it can decide
        // whether anything changed and push a single undo command for both
        // the grid and the custom widget.
        return m_session->commit();
    }
    if (!m_sourceComponents || m_ownedWorkingComponents == *m_sourceComponents)
        return false;
    if (m_commit)
        m_commit(m_ownedWorkingComponents);
    else
        *m_sourceComponents = m_ownedWorkingComponents;
    return true;
}

void QtFormDialog::onOk()
{
    // commitChanges() returning false only means the working set was left
    // untouched, which is not a failure — OK must still close the dialog.
    onApply();
    accept();
}

void QtFormDialog::reject()
{
    // Cancelling throws away the working copy, so a custom widget can edit its
    // record freely without any risk of the edit surviving.
    if (m_session)
        m_session->discard();
    QDialog::reject();
}

} // namespace openck
