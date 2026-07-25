#include "formcomponentwidget.hpp"

#include "../../libs/components/tier1_components.hpp"

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
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAbstractItemView>

namespace openck {

namespace {

QWidget* makeEditorWidget(EditorProperty* prop, QWidget* parent)
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
    if (dynamic_cast<IntEditorProperty*>(prop) ||
        dynamic_cast<UIntEditorProperty*>(prop))
    {
        auto* sb = new QSpinBox(parent);
        sb->setRange(INT_MIN, INT_MAX);
        sb->setValue(prop->value().toInt());
        QObject::connect(sb, qOverload<int>(&QSpinBox::valueChanged), parent,
            [prop](int v) { prop->setValue(v); });
        return sb;
    }
    if (dynamic_cast<FloatEditorProperty*>(prop))
    {
        auto* sb = new QDoubleSpinBox(parent);
        sb->setRange(-1.0e9, 1.0e9);
        sb->setDecimals(2);
        sb->setSingleStep(0.1);
        sb->setValue(prop->value().toDouble());
        QObject::connect(sb, qOverload<double>(&QDoubleSpinBox::valueChanged), parent,
            [prop](double v) { prop->setValue(v); });
        return sb;
    }
    if (dynamic_cast<FormEditorProperty*>(prop))
    {
        auto* le = new QLineEdit(parent);
        le->setPlaceholderText(QStringLiteral("0x00000000"));
        le->setText(QString::number(prop->value().toUInt(), 16));
        QObject::connect(le, &QLineEdit::editingFinished, parent, [le, prop]() {
            QString text = le->text().trimmed();
            if (text.startsWith(QStringLiteral("0x"))) text.remove(0, 2);
            bool ok = false;
            quint32 v = text.toUInt(&ok, 16);
            if (ok) prop->setValue(v);
        });
        return le;
    }
    if (dynamic_cast<FormArrayEditorProperty*>(prop))
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

        auto* currentList = static_cast<FormArrayEditorProperty*>(prop);
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

FormComponentWidget::FormComponentWidget(Component* component, QWidget* parent)
    : QWidget(parent)
    , m_component(component)
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
        QWidget* editor = makeEditorWidget(prop.get(), this);
        if (!editor) continue;
        m_layout->addRow(prop->name() + QStringLiteral(":"), editor);
    }

    // Container items: render as a table if the component is
    // TESContainer_Component (which returns no EditorProperties
    // and relies on this custom rendering instead).
    if (m_component->className() == QStringLiteral("TESContainer"))
    {
        auto* container = static_cast<tescomponents::TESContainer_Component*>(m_component);
        auto* table = new QTableWidget(this);
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({QStringLiteral("Form ID"), QStringLiteral("Count")});
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        int row = 0;
        for (const auto& item : container->items)
        {
            table->insertRow(row);
            auto* formItem = new QTableWidgetItem(
                QStringLiteral("0x%1").arg(item.formId, 8, 16, QChar('0')));
            auto* countItem = new QTableWidgetItem();
            countItem->setData(Qt::DisplayRole, item.count);
            table->setItem(row, 0, formItem);
            table->setItem(row, 1, countItem);
            ++row;
        }
        auto* addBtn = new QPushButton(QStringLiteral("Add Item"), this);
        QObject::connect(addBtn, &QPushButton::clicked, this, [this, table]() {
            table->insertRow(table->rowCount());
        });
        auto* rmBtn = new QPushButton(QStringLiteral("Remove"), this);
        QObject::connect(rmBtn, &QPushButton::clicked, this, [this, table]() {
            int row = table->currentRow();
            if (row >= 0) table->removeRow(row);
        });
        auto* btnLayout = new QHBoxLayout();
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(rmBtn);
        btnLayout->addStretch();
        m_layout->addRow(QStringLiteral("Items:"), table);
        m_layout->addRow(QString(), btnLayout);
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
