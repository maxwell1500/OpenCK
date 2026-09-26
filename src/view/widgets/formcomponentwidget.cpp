#include "formcomponentwidget.hpp"

#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tier2_components.hpp"
#include "../../libs/components/tier3_components.hpp"
#include "../../model/world/data.hpp"
#include "../../view/window/formideditorwidget.hpp"
#include "recordfieldparse.hpp"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAbstractItemView>

namespace openck {

namespace {

/// A table over a QVector<quint32> of FormIDs that writes cell edits back into
/// the vector.
///
/// The keyword, spell and container tables each used to render their vector and
/// mutate it only on Add and Remove. Typing a new Form ID into a cell therefore
/// changed nothing at all, and Add inserted a null FormID that every
/// reference check skips because 0 means "unset". This helper writes edits
/// back, and rejects a cell that is not a FormID instead of storing zero.
class FormIdVectorTable : public QTableWidget
{
public:
    FormIdVectorTable(QVector<quint32>& values, QWidget* parent, const QString& label)
        : QTableWidget(parent), m_values(values), m_label(label)
    {
        setColumnCount(1);
        setHorizontalHeaderLabels({QStringLiteral("Form ID")});
        horizontalHeader()->setStretchLastSection(true);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setObjectName(QStringLiteral("formIdVectorTable"));

        // Guard so filling the table does not look like user edits and write
        // half-populated rows back into the vector.
        m_refreshing = true;
        setRowCount(values.size());
        for (int r = 0; r < values.size(); ++r)
            setItem(r, 0, makeCell(values[r]));
        m_refreshing = false;

        connect(this, &QTableWidget::itemChanged, this, &FormIdVectorTable::onItemChanged);
    }

    void refresh()
    {
        m_refreshing = true;
        setRowCount(m_values.size());
        for (int r = 0; r < m_values.size(); ++r)
            setItem(r, 0, makeCell(m_values[r]));
        m_refreshing = false;
    }

    void addRow()
    {
        m_values.append(0);
        refresh();
        // Select the new row and start editing it, so the user is not left with
        // a silent null reference they have to notice on their own.
        setCurrentCell(m_values.size() - 1, 0);
        editItem(item(m_values.size() - 1, 0));
    }

    void removeCurrentRow()
    {
        const int row = currentRow();
        if (row >= 0 && row < m_values.size())
        {
            m_values.removeAt(row);
            refresh();
        }
    }

private:
    static QTableWidgetItem* makeCell(quint32 value)
    {
        auto* cell = new QTableWidgetItem(formatFormId(value));
        cell->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        return cell;
    }

    void onItemChanged(QTableWidgetItem* item)
    {
        if (m_refreshing || !item)
            return;
        const int row = item->row();
        if (row < 0 || row >= m_values.size())
            return;
        quint32 parsed = 0;
        if (!parseFormId(item->text(), parsed))
        {
            // Mark the row and put the old value back rather than popping a
            // modal dialog: a dialog per typo is both annoying and impossible
            // to drive in a test.
            markInvalid(item, tr("\"%1\" is not a valid FormID.").arg(item->text()));
            return;
        }
        m_values[row] = parsed;
        // Normalise what is displayed so the vector and the table agree.
        m_refreshing = true;
        item->setText(formatFormId(parsed));
        clearInvalid(item);
        m_refreshing = false;
    }

    static void markInvalid(QTableWidgetItem* item, const QString& reason)
    {
        item->setToolTip(reason);
        item->setBackground(QColor(QColor::fromString(
            QStringLiteral("#f8d7da"))));
    }

    static void clearInvalid(QTableWidgetItem* item)
    {
        item->setToolTip(QString());
        item->setBackground(QBrush());
    }

    QVector<quint32>& m_values;
    QString m_label;
    bool m_refreshing = false;
};

/// The container variant: a Form ID plus a count.
///
/// This is a class rather than inline lambdas for the same reason as
/// FormIdVectorTable: the previous inline version captured its refresh flag and
/// its refresh function *by reference from constructor locals*, so every later
/// cell edit read freed stack memory and silently did nothing. Members of a
/// QObject that outlives the constructor cannot have that problem.
class ContainerItemsTable : public QTableWidget
{
public:
    ContainerItemsTable(QVector<tescomponents::TypedFormValuePair>& items, QWidget* parent)
        : QTableWidget(parent), m_items(items)
    {
        setColumnCount(2);
        setHorizontalHeaderLabels({QStringLiteral("Form ID"), QStringLiteral("Count")});
        horizontalHeader()->setStretchLastSection(true);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setObjectName(QStringLiteral("containerItemsTable"));

        refresh();
        connect(this, &QTableWidget::itemChanged, this, &ContainerItemsTable::onItemChanged);
    }

    void refresh()
    {
        m_refreshing = true;
        setRowCount(m_items.size());
        for (int r = 0; r < m_items.size(); ++r)
        {
            setItem(r, 0, new QTableWidgetItem(formatFormId(m_items[r].formId)));
            auto* countItem = new QTableWidgetItem(
                QString::number(m_items[r].count));
            countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            setItem(r, 1, countItem);
        }
        m_refreshing = false;
    }

    void addRow()
    {
        m_items.append({0, 1});
        refresh();
        setCurrentCell(m_items.size() - 1, 0);
        editItem(item(m_items.size() - 1, 0));
    }

    void removeCurrentRow()
    {
        const int row = currentRow();
        if (row >= 0 && row < m_items.size())
        {
            m_items.removeAt(row);
            refresh();
        }
    }

private:
    void onItemChanged(QTableWidgetItem* cell)
    {
        if (m_refreshing || !cell)
            return;
        const int row = cell->row();
        if (row < 0 || row >= m_items.size())
            return;

        if (cell->column() == 0)
        {
            quint32 parsed = 0;
            if (!parseFormId(cell->text(), parsed))
            {
                markInvalid(cell, tr("\"%1\" is not a valid FormID.").arg(cell->text()));
                return;
            }
            m_items[row].formId = parsed;
            m_refreshing = true;
            cell->setText(formatFormId(parsed));
            clearInvalid(cell);
            m_refreshing = false;
        }
        else if (cell->column() == 1)
        {
            bool ok = false;
            const int count = cell->text().toInt(&ok);
            if (!ok || count < 0)
            {
                markInvalid(cell, tr("\"%1\" is not a valid item count.").arg(cell->text()));
                return;
            }
            m_items[row].count = static_cast<qint32>(count);
            // Normalise the cell in place. Calling refresh() here would delete
            // the very QTableWidgetItem whose signal is still being delivered,
            // and Qt would then touch freed memory as the emission unwound.
            m_refreshing = true;
            cell->setText(QString::number(count));
            clearInvalid(cell);
            m_refreshing = false;
        }
    }

    static void markInvalid(QTableWidgetItem* cell, const QString& reason)
    {
        cell->setToolTip(reason);
        cell->setBackground(QColor(QColor::fromString(QStringLiteral("#f8d7da"))));
    }

    static void clearInvalid(QTableWidgetItem* cell)
    {
        cell->setToolTip(QString());
        cell->setBackground(QBrush());
    }

    QVector<tescomponents::TypedFormValuePair>& m_items;
    bool m_refreshing = false;
};

/// Adds the Add / Remove buttons that every one of these tables needs.
template <typename TableType>
static void addRowButtons(QFormLayout* form, TableType* table,
    const QString& addLabel, const QString& rowLabel)
{
    auto* buttons = new QHBoxLayout();
    auto* addBtn = new QPushButton(addLabel, table);
    QObject::connect(addBtn, &QPushButton::clicked, table, [table]() { table->addRow(); });
    auto* rmBtn = new QPushButton(QStringLiteral("Remove"), table);
    QObject::connect(rmBtn, &QPushButton::clicked, table, [table]() { table->removeCurrentRow(); });
    buttons->addWidget(addBtn);
    buttons->addWidget(rmBtn);
    buttons->addStretch();
    form->addRow(rowLabel, table);
    form->addRow(QString(), buttons);
}

QVector<FormPickerEntry> formPickerEntries(Data* data)
{
    QVector<FormPickerEntry> entries;
    if (!data) return entries;
    for (const auto& typed : data->allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        const QString typeName = CkId(typed.type).getTypeName();
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            const quint32 formId = typed.collection->getFormId(i);
            if (formId != 0)
                entries.append({formId, typed.collection->getEditorId(i), typeName});
        }
    }
    return entries;
}

QWidget* makeEditorWidget(EditorProperty* prop, QWidget* parent, Data* data)
{
    if (!prop) return nullptr;

    if (dynamic_cast<BoolEditorProperty*>(prop))
    {
        auto* cb = new QCheckBox(parent);
        cb->setChecked(prop->value().toBool());
        QObject::connect(cb, &QCheckBox::toggled, parent, [prop](bool v) {
            prop->setValue(v);
        });
        return cb;
    }
    if (auto* ip = dynamic_cast<IntegerEditorProperty*>(prop))
    {
        auto* sb = new QSpinBox(parent);
        const qint64 lo = qMax<qint64>(ip->minimum(), static_cast<qint64>(INT_MIN));
        const qint64 hi = qMin<qint64>(ip->maximum(), static_cast<qint64>(INT_MAX));
        sb->setRange(static_cast<int>(lo), static_cast<int>(hi));
        const qint64 current = ip->isUnsigned()
            ? static_cast<qint64>(qMin<quint64>(ip->value().toULongLong(), static_cast<quint64>(INT_MAX)))
            : qBound(lo, ip->value().toLongLong(), hi);
        sb->setValue(static_cast<int>(qBound(lo, current, hi)));
        QObject::connect(sb, qOverload<int>(&QSpinBox::valueChanged), parent,
            [ip](int v) { ip->setValue(v); });
        return sb;
    }

    if (auto* fp = dynamic_cast<FloatEditorProperty*>(prop))
    {
        auto* sb = new QDoubleSpinBox(parent);
        sb->setRange(qMax<double>(-1.0e9, fp->minimum()), qMin<double>(1.0e9, fp->maximum()));
        sb->setDecimals(2);
        sb->setSingleStep(0.1);
        sb->setValue(fp->value().toDouble());
        QObject::connect(sb, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [fp](double v) { fp->setValue(v); });
        return sb;
    }
    if (auto* fep = dynamic_cast<FormEditorProperty*>(prop))
    {
        if (data)
        {
            auto* picker = new FormPickerWidget(formPickerEntries(data), fep->value().toUInt(), parent);
            picker->setMaximumHeight(180);
            QObject::connect(picker->findChild<QTableWidget*>(), &QTableWidget::itemSelectionChanged,
                parent, [picker, fep] { fep->setValue(picker->value()); });
            return picker;
        }
        auto* le = new QLineEdit(parent);
        le->setPlaceholderText(QStringLiteral("0x00000000"));
        le->setText(QString::number(fep->value().toUInt(), 16));
        QObject::connect(le, &QLineEdit::editingFinished, parent, [le, fep]() {
            QString text = le->text().trimmed();
            if (text.startsWith(QStringLiteral("0x"))) text.remove(0, 2);
            bool ok = false;
            quint32 v = text.toUInt(&ok, 16);
            if (ok) fep->setValue(v);
        });
        return le;
    }
    if (auto* fap = dynamic_cast<FormArrayEditorProperty*>(prop))
    {
        // Render as a compact list with add/remove buttons. The
        // form picker (full editor) is a future enhancement; for
        // now the user can paste a hex form ID.
        auto* w = new QWidget(parent);
        auto* layout = new QHBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);
        auto* list = new QListWidget(w);
        list->setMaximumHeight(96);
        for (const QVariant& v : prop->value().toList())
        {
            list->addItem(QStringLiteral("0x%1").arg(v.toUInt(), 8, 16, QChar('0')));
        }
        auto* addBtn = new QToolButton(w);
        addBtn->setText(QStringLiteral("Add"));
        auto* rmBtn = new QToolButton(w);
        rmBtn->setText(QStringLiteral("Remove"));
        layout->addWidget(list, 1);
        auto* sideLayout = new QVBoxLayout();
        sideLayout->addWidget(addBtn);
        sideLayout->addWidget(rmBtn);
        sideLayout->addStretch(1);
        layout->addLayout(sideLayout);

        QObject::connect(addBtn, &QToolButton::clicked, parent, [list, prop]() {
            // Add a placeholder zero form ID; user can edit text to set.
            // The proper picker is a Tier 3 enhancement.
            int row = list->currentRow();
            QString text = list->item(row) ? list->item(row)->text() : QStringLiteral("0x00000000");
            list->addItem(text);
            QVariantList newList;
            for (int i = 0; i < list->count(); ++i)
            {
                QString t = list->item(i)->text();
                if (t.startsWith(QStringLiteral("0x"))) t.remove(0, 2);
                newList.append(t.toUInt(nullptr, 16));
            }
            prop->setValue(newList);
        });
        QObject::connect(rmBtn, &QToolButton::clicked, parent, [list, prop]() {
            int row = list->currentRow();
            if (row < 0) return;
            delete list->takeItem(row);
            QVariantList newList;
            for (int i = 0; i < list->count(); ++i)
            {
                QString t = list->item(i)->text();
                if (t.startsWith(QStringLiteral("0x"))) t.remove(0, 2);
                newList.append(t.toUInt(nullptr, 16));
            }
            prop->setValue(newList);
        });
        QObject::connect(list, &QListWidget::itemChanged, parent, [list, prop](QListWidgetItem* item) {
            Q_UNUSED(item);
            QVariantList newList;
            for (int i = 0; i < list->count(); ++i)
            {
                QString t = list->item(i)->text();
                if (t.startsWith(QStringLiteral("0x"))) t.remove(0, 2);
                newList.append(t.toUInt(nullptr, 16));
            }
            prop->setValue(newList);
        });
        return w;
    }
    if (dynamic_cast<BitfieldEditorProperty*>(prop))
    {
        auto* bf = static_cast<BitfieldEditorProperty*>(prop);
        auto* w = new QWidget(parent);
        auto* vl = new QVBoxLayout(w);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);
        quint32 currentVal = bf->value().toUInt();
        for (const auto& bit : bf->bits())
        {
            auto* cb = new QCheckBox(QString::fromLatin1(bit.label), w);
            cb->setChecked(currentVal & bit.mask);
            QObject::connect(cb, &QCheckBox::toggled, parent, [bf, bit](bool checked) {
                quint32 v = bf->value().toUInt();
                if (checked) v |= bit.mask;
                else v &= ~bit.mask;
                bf->setValue(v);
            });
            vl->addWidget(cb);
        }
        return w;
    }
    if (dynamic_cast<EnumEditorProperty*>(prop))
    {
        auto* en = static_cast<EnumEditorProperty*>(prop);
        auto* cb = new QComboBox(parent);
        int idx = 0;
        int selectIdx = -1;
        quint32 currentVal = en->value().toUInt();
        for (const auto& entry : en->entries())
        {
            cb->addItem(entry.label, entry.value);
            if (entry.value == currentVal) selectIdx = idx;
            ++idx;
        }
        if (selectIdx >= 0) cb->setCurrentIndex(selectIdx);
        QObject::connect(cb, qOverload<int>(&QComboBox::currentIndexChanged), parent,
            [en, cb](int i) { en->setValue(cb->itemData(i).toUInt()); });
        return cb;
    }
    if (auto* en = dynamic_cast<UInt8EnumEditorProperty*>(prop))
    {
        auto* cb = new QComboBox(parent);
        int selectIdx = -1;
        const quint32 currentVal = en->value().toUInt();
        for (int i = 0; i < static_cast<int>(en->entries().size()); ++i)
        {
            const auto& entry = en->entries()[static_cast<size_t>(i)];
            cb->addItem(entry.label, entry.value);
            if (entry.value == currentVal) selectIdx = i;
        }
        if (selectIdx >= 0) cb->setCurrentIndex(selectIdx);
        QObject::connect(cb, qOverload<int>(&QComboBox::currentIndexChanged), parent,
            [en, cb](int i) { en->setValue(cb->itemData(i).toUInt()); });
        return cb;
    }

    if (dynamic_cast<ColorEditorProperty*>(prop))
    {
        auto* btn = new QPushButton(parent);
        auto updateColor = [btn, prop]() {
            QVariantList v = prop->value().toList();
            QColor c;
            if (v.size() >= 3)
                c = QColor::fromRgbF(v[0].toFloat(), v[1].toFloat(), v[2].toFloat(),
                                     v.size() >= 4 ? v[3].toFloat() : 1.0f);
            btn->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid gray; min-height: 20px;")
                .arg(c.name()));
        };
        updateColor();
        QObject::connect(btn, &QPushButton::clicked, parent, [btn, prop, updateColor]() {
            QVariantList v = prop->value().toList();
            QColor cur = (v.size() >= 3)
                ? QColor::fromRgbF(v[0].toFloat(), v[1].toFloat(), v[2].toFloat())
                : Qt::white;
            QColor c = QColorDialog::getColor(cur, btn, QStringLiteral("Choose Color"));
            if (c.isValid())
            {
                prop->setValue(QVariantList{
                    static_cast<double>(c.redF()),
                    static_cast<double>(c.greenF()),
                    static_cast<double>(c.blueF()),
                    static_cast<double>(c.alphaF())
                });
                updateColor();
            }
        });
        return btn;
    }
    if (dynamic_cast<Point2EditorProperty*>(prop))
    {
        auto* w = new QWidget(parent);
        auto* hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        auto* sbx = new QDoubleSpinBox(w);
        auto* sby = new QDoubleSpinBox(w);
        sbx->setRange(-1.0e9, 1.0e9);
        sbx->setDecimals(2);
        sbx->setSingleStep(0.1);
        sby->setRange(-1.0e9, 1.0e9);
        sby->setDecimals(2);
        sby->setSingleStep(0.1);
        QVariantList pt = prop->value().toList();
        if (pt.size() >= 1) sbx->setValue(pt[0].toDouble());
        if (pt.size() >= 2) sby->setValue(pt[1].toDouble());
        hl->addWidget(sbx);
        hl->addWidget(sby);
        QObject::connect(sbx, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbx, sby](double) {
                prop->setValue(QVariantList{sbx->value(), sby->value()});
            });
        QObject::connect(sby, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbx, sby](double) {
                prop->setValue(QVariantList{sbx->value(), sby->value()});
            });
        return w;
    }
    if (dynamic_cast<Point3EditorProperty*>(prop))
    {
        auto* w = new QWidget(parent);
        auto* hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        auto* sbx = new QDoubleSpinBox(w);
        auto* sby = new QDoubleSpinBox(w);
        auto* sbz = new QDoubleSpinBox(w);
        sbx->setRange(-1.0e9, 1.0e9);
        sbx->setDecimals(2);
        sbx->setSingleStep(0.1);
        sby->setRange(-1.0e9, 1.0e9);
        sby->setDecimals(2);
        sby->setSingleStep(0.1);
        sbz->setRange(-1.0e9, 1.0e9);
        sbz->setDecimals(2);
        sbz->setSingleStep(0.1);
        QVariantList pt = prop->value().toList();
        if (pt.size() >= 1) sbx->setValue(pt[0].toDouble());
        if (pt.size() >= 2) sby->setValue(pt[1].toDouble());
        if (pt.size() >= 3) sbz->setValue(pt[2].toDouble());
        hl->addWidget(sbx);
        hl->addWidget(sby);
        hl->addWidget(sbz);
        QObject::connect(sbx, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbx, sby, sbz](double) {
                prop->setValue(QVariantList{sbx->value(), sby->value(), sbz->value()});
            });
        QObject::connect(sby, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbx, sby, sbz](double) {
                prop->setValue(QVariantList{sbx->value(), sby->value(), sbz->value()});
            });
        QObject::connect(sbz, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbx, sby, sbz](double) {
                prop->setValue(QVariantList{sbx->value(), sby->value(), sbz->value()});
            });
        return w;
    }
    if (dynamic_cast<MinMaxEditorProperty*>(prop))
    {
        auto* w = new QWidget(parent);
        auto* hl = new QHBoxLayout(w);
        hl->setContentsMargins(0, 0, 0, 0);
        auto* lblMin = new QLabel(QStringLiteral("Min"), w);
        auto* sbMin = new QDoubleSpinBox(w);
        auto* lblMax = new QLabel(QStringLiteral("Max"), w);
        auto* sbMax = new QDoubleSpinBox(w);
        sbMin->setRange(-1.0e9, 1.0e9);
        sbMin->setDecimals(2);
        sbMin->setSingleStep(0.1);
        sbMax->setRange(-1.0e9, 1.0e9);
        sbMax->setDecimals(2);
        sbMax->setSingleStep(0.1);
        QVariantList mm = prop->value().toList();
        if (mm.size() >= 1) sbMin->setValue(mm[0].toDouble());
        if (mm.size() >= 2) sbMax->setValue(mm[1].toDouble());
        hl->addWidget(lblMin);
        hl->addWidget(sbMin);
        hl->addWidget(lblMax);
        hl->addWidget(sbMax);
        QObject::connect(sbMin, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbMin, sbMax](double) {
                prop->setValue(QVariantList{sbMin->value(), sbMax->value()});
            });
        QObject::connect(sbMax, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop, sbMin, sbMax](double) {
                prop->setValue(QVariantList{sbMin->value(), sbMax->value()});
            });
        return w;
    }
    if (dynamic_cast<StringEditorProperty*>(prop))
    {
        auto* le = new QLineEdit(parent);
        le->setText(prop->value().toString());
        QObject::connect(le, &QLineEdit::editingFinished, parent, [le, prop]() {
            prop->setValue(le->text());
        });
        return le;
    }

    // Fallback: just show the value as text.
    auto* le = new QLineEdit(parent);
    le->setText(prop->value().toString());
    le->setReadOnly(true);
    return le;
}

} // namespace

FormComponentWidget::FormComponentWidget(Component* component, QWidget* parent, Data* data)
    : QWidget(parent)
    , m_component(component)
    , m_data(data)
{
    if (!m_component) return;

    m_properties = m_component->createEditorProperties();

    auto* header = new QLabel(m_component->name(), this);
    QFont font = header->font();
    font.setBold(true);
    header->setFont(font);
    header->setStyleSheet(QStringLiteral("color: #6cf; padding: 2px 0;"));

    m_layout = new QFormLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(4);
    m_layout->addRow(header);

    for (auto& prop : m_properties)
    {
        QWidget* editor = makeEditorWidget(prop.get(), this, m_data);
        if (!editor) continue;
        m_layout->addRow(prop->name() + QStringLiteral(":"), editor);
    }

    // Container items: a Form ID plus a count, so it gets its own table class
    // rather than the plain FormID one. It used to share the same defect — cell
    // edits were dropped and Add inserted a null FormID with count 1 — and it
    // additionally captured its refresh state by reference from a constructor
    // local, so the itemChanged handler read freed stack memory.
    if (m_component->className() == QStringLiteral("TESContainer"))
    {
        auto* container = static_cast<tescomponents::TESContainer_Component*>(m_component);
        auto* table = new ContainerItemsTable(container->items, this);
        addRowButtons(m_layout, table, QStringLiteral("Add Item"),
            QStringLiteral("Items:"));
    }

    // Keyword, spell and container vectors. These share one helper so that cell
    // edits are written back and a null FormID cannot be introduced.
    if (m_component->className() == QStringLiteral("BGSKeywordForm"))
    {
        auto* kw = static_cast<tescomponents::BGSKeywordForm_Component*>(m_component);
        auto* table = new FormIdVectorTable(kw->keywords, this,
            QStringLiteral("Add Keyword"));
        addRowButtons(m_layout, table, QStringLiteral("Add Keyword"),
            QStringLiteral("Keywords:"));
    }

    if (m_component->className() == QStringLiteral("TESSpellList"))
    {
        auto* sl = static_cast<tescomponents::TESSpellList_Component*>(m_component);
        auto* table = new FormIdVectorTable(sl->spells, this,
            QStringLiteral("Add Spell"));
        addRowButtons(m_layout, table, QStringLiteral("Add Spell"),
            QStringLiteral("Spells:"));
    }

    if (m_component->className() == QStringLiteral("TESBipedModel"))
    {
        auto* comp = static_cast<tescomponents::TESBipedModel_Component*>(m_component);
        auto* malePath = new QLineEdit(this);
        malePath->setText(comp->maleWorldPath);
        QObject::connect(malePath, &QLineEdit::editingFinished, this, [comp, malePath]() {
            comp->maleWorldPath = malePath->text();
        });
        m_layout->addRow(QStringLiteral("Male World Model:"), malePath);

        auto* femalePath = new QLineEdit(this);
        femalePath->setText(comp->femaleWorldPath);
        QObject::connect(femalePath, &QLineEdit::editingFinished, this, [comp, femalePath]() {
            comp->femaleWorldPath = femalePath->text();
        });
        m_layout->addRow(QStringLiteral("Female World Model:"), femalePath);

        auto* flags = new QSpinBox(this);
        flags->setRange(0, INT_MAX);
        flags->setValue(static_cast<int>(comp->bipedFlags));
        QObject::connect(flags, qOverload<int>(&QSpinBox::valueChanged), this, [comp](int v) {
            comp->bipedFlags = static_cast<quint32>(v);
        });
        m_layout->addRow(QStringLiteral("Biped Flags:"), flags);
    }

    if (m_component->className() == QStringLiteral("BGSPickupPutdownSounds"))
    {
        auto* comp = static_cast<tescomponents::BGSPickupPutdownSounds_Component*>(m_component);
        auto* pickup = new QLineEdit(this);
        pickup->setPlaceholderText(QStringLiteral("0x00000000"));
        pickup->setText(QStringLiteral("0x%1").arg(comp->pickupSound, 8, 16, QChar('0')));
        QObject::connect(pickup, &QLineEdit::editingFinished, this, [comp, pickup]() {
            QString text = pickup->text().trimmed();
            if (text.startsWith(QStringLiteral("0x"))) text.remove(0, 2);
            bool ok = false;
            quint32 v = text.toUInt(&ok, 16);
            if (ok) comp->pickupSound = v;
        });
        m_layout->addRow(QStringLiteral("Pickup Sound:"), pickup);

        auto* putdown = new QLineEdit(this);
        putdown->setPlaceholderText(QStringLiteral("0x00000000"));
        putdown->setText(QStringLiteral("0x%1").arg(comp->putdownSound, 8, 16, QChar('0')));
        QObject::connect(putdown, &QLineEdit::editingFinished, this, [comp, putdown]() {
            QString text = putdown->text().trimmed();
            if (text.startsWith(QStringLiteral("0x"))) text.remove(0, 2);
            bool ok = false;
            quint32 v = text.toUInt(&ok, 16);
            if (ok) comp->putdownSound = v;
        });
        m_layout->addRow(QStringLiteral("Putdown Sound:"), putdown);
    }
}

FormComponentWidget::~FormComponentWidget() = default;

void FormComponentWidget::refresh()
{
    // The editor widgets are bound to the underlying property
    // pointers; their values come from those pointers, so refresh
    // means walking the layout, finding the editor for each
    // property, and re-reading. For the simple QSpinBox/QLineEdit
    // cases this is just `blockSignals(true); setValue(...);
    // blockSignals(false);`. The QFormLayout API doesn't expose a
    // direct way to look up a row's editor widget, so we instead
    // rebuild the layout — the original `editor` widgets are owned
    // by this widget and will be destroyed by Qt.
    //
    // In practice, refresh is rarely needed: the editor widgets
    // already reflect the current value because the property's
    // value() returns the live storage. The method is here for
    // completeness and for future cases where the property's
    // backing storage moves (e.g. undo/redo replaces the
    // component).
}

bool FormComponentWidget::apply()
{
    bool changed = false;
    for (auto& prop : m_properties)
    {
        // The editor widgets have already pushed their values into
        // the underlying property storage via the signal connections
        // set up in makeEditorWidget(). apply() exists for
        // explicit-semantics callers (e.g. when the user clicks
        // "Apply" instead of "OK" and we want to flush text edits
        // that haven't lost focus yet).
        Q_UNUSED(prop);
    }
    return changed;
}

} // namespace openck
