#include "formpickerwidget.hpp"

#include <QComboBox>
#include <QHeaderView>

#include <algorithm>

FormPickerWidget::FormPickerWidget(const QVector<FormPickerEntry>& entries,
                                   quint32 currentFormId, QWidget* parent)
    : QWidget(parent)
    , m_entries(entries)
    , m_value(currentFormId)
{
    auto* layout = new QVBoxLayout(this);
    auto* filters = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("Search Editor ID or FormID"));
    m_type = new QComboBox(this);
    filters->addWidget(m_search);
    filters->addWidget(m_type);
    layout->addLayout(filters);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Editor ID"), QStringLiteral("Type"),
        QStringLiteral("FormID")});
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setSelectionMode(QTableWidget::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table);

    auto* navigation = new QHBoxLayout();
    auto* previous = new QPushButton(QStringLiteral("Previous"), this);
    auto* next = new QPushButton(QStringLiteral("Next"), this);
    navigation->addWidget(previous);
    navigation->addWidget(next);
    navigation->addStretch();
    m_status = new QLabel(this);
    navigation->addWidget(m_status);
    layout->addLayout(navigation);

    QSet<QString> types;
    for (const FormPickerEntry& entry : m_entries)
        if (!entry.typeName.isEmpty()) types.insert(entry.typeName);
    QStringList sortedTypes = QStringList(types.values());
    sortedTypes.sort();
    m_type->addItem(QStringLiteral("All types"), QString());
    for (const QString& type : sortedTypes)
        m_type->addItem(type, type);

    connect(m_search, &QLineEdit::textChanged, this, [this](const QString&) { rebuild(); });
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this](int) { rebuild(); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const int row = m_table->currentRow();
        if (row >= 0) selectRow(row);
    });
    connect(previous, &QPushButton::clicked, this, [this] { selectNext(-1); });
    connect(next, &QPushButton::clicked, this, [this] { selectNext(1); });

    rebuild();
    setValue(currentFormId);
}

void FormPickerWidget::setEntries(const QVector<FormPickerEntry>& entries)
{
    m_entries = entries;
    const QString selectedType = m_type->currentData().toString();
    m_type->clear();
    QSet<QString> types;
    for (const FormPickerEntry& entry : m_entries)
        if (!entry.typeName.isEmpty()) types.insert(entry.typeName);
    QStringList sortedTypes = QStringList(types.values());
    sortedTypes.sort();
    m_type->addItem(QStringLiteral("All types"), QString());
    int typeIndex = 0;
    for (const QString& type : sortedTypes)
    {
        m_type->addItem(type, type);
        if (type == selectedType) typeIndex = m_type->count() - 1;
    }
    m_type->setCurrentIndex(typeIndex);
    rebuild();
}

void FormPickerWidget::setValue(quint32 formId)
{
    m_value = formId;
    rebuild();
}

void FormPickerWidget::selectRow(int row)
{
    if (row < 0 || row >= m_table->rowCount()) return;
    m_table->selectRow(row);
    m_value = m_table->item(row, 0)->data(Qt::UserRole).toUInt();
}

void FormPickerWidget::selectNext(int direction)
{
    if (m_table->rowCount() == 0) return;
    int row = m_table->currentRow() + direction;
    if (row < 0) row = m_table->rowCount() - 1;
    if (row >= m_table->rowCount()) row = 0;
    selectRow(row);
}

void FormPickerWidget::rebuild()
{
    const QString search = m_search->text().trimmed();
    const QString type = m_type->currentData().toString();
    QHash<quint32, int> counts;
    for (const FormPickerEntry& entry : m_entries)
        counts[entry.formId] += 1;

    QVector<FormPickerEntry> filtered;
    for (const FormPickerEntry& entry : m_entries)
    {
        if (!type.isEmpty() && entry.typeName != type) continue;
        if (!search.isEmpty()
            && !entry.editorId.contains(search, Qt::CaseInsensitive)
            && !QString::number(entry.formId, 16).contains(search, Qt::CaseInsensitive))
            continue;
        filtered.append(entry);
    }

    m_table->setRowCount(filtered.size());
    for (int row = 0; row < filtered.size(); ++row)
    {
        const FormPickerEntry& entry = filtered[row];
        auto* editor = new QTableWidgetItem(entry.editorId);
        editor->setData(Qt::UserRole, entry.formId);
        m_table->setItem(row, 0, editor);
        m_table->setItem(row, 1, new QTableWidgetItem(entry.typeName));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QStringLiteral("0x%1").arg(entry.formId, 8, 16, QChar('0'))));
    }

    const auto selected = std::find_if(filtered.cbegin(), filtered.cend(),
        [this](const FormPickerEntry& entry) { return entry.formId == m_value; });
    if (selected != filtered.cend())
        m_table->selectRow(static_cast<int>(selected - filtered.cbegin()));
    if (m_value != 0 && counts.value(m_value, 0) > 1)
        m_status->setText(QStringLiteral("Duplicate FormID: %1 matches").arg(counts.value(m_value)));
    else
        m_status->setText(QStringLiteral("%1 result(s)").arg(filtered.size()));
}
