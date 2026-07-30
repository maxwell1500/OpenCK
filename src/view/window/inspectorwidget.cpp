#include "inspectorwidget.hpp"

#include "../widgets/editorpropertygrid.hpp"
#include "../../libs/components/formcomponents.hpp"
#include "../../libs/components/component.hpp"

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>

InspectorWidget::InspectorWidget(QWidget* parent)
    : QWidget(parent)
    , m_scrollArea(nullptr)
    , m_grid(nullptr)
    , m_titleLabel(nullptr)
    , m_subRecordsTable(nullptr)
    , m_currentComponents(nullptr)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_titleLabel = new QLabel(tr("No record selected"), this);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    layout->addWidget(m_titleLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_grid = new openck::EditorPropertyGrid(m_scrollArea);
    m_scrollArea->setWidget(m_grid);
    layout->addWidget(m_scrollArea, 1);

    m_subRecordsTable = new QTableWidget(this);
    m_subRecordsTable->setColumnCount(2);
    m_subRecordsTable->setHorizontalHeaderLabels({"Field", "Value"});
    m_subRecordsTable->horizontalHeader()->setStretchLastSection(true);
    m_subRecordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_subRecordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_subRecordsTable->setMaximumHeight(150);
    m_subRecordsTable->setVisible(false);
    layout->addWidget(m_subRecordsTable);
}

InspectorWidget::~InspectorWidget() = default;

void InspectorWidget::onRecordSelected(int categoryId, int recordIndex, const QString& editorId)
{
    Q_UNUSED(categoryId);
    Q_UNUSED(recordIndex);

    m_titleLabel->setText(tr("Record: %1").arg(editorId));
    m_subRecordsTable->setVisible(false);
}

void InspectorWidget::clear()
{
    m_currentComponents = nullptr;
    m_currentFormIdKey.clear();
    m_titleLabel->setText(tr("No record selected"));
    m_grid->setComponents({});
    m_subRecordsTable->setVisible(false);
}

void InspectorWidget::showComponents(openck::FormComponents* components, const QString& title)
{
    m_currentComponents = components;
    m_currentFormIdKey = title;
    m_titleLabel->setText(title);

    if (components)
    {
        std::vector<Component*> componentPtrs;
        componentPtrs.reserve(components->size());
        for (const auto& c : components->all())
        {
            componentPtrs.push_back(c.get());
        }
        m_grid->setComponents(componentPtrs);
    }
    else
    {
        m_grid->setComponents({});
    }

    m_subRecordsTable->setVisible(false);
}
