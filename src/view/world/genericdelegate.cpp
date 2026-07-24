#include "genericdelegate.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>

GenericDelegate::GenericDelegate(Document& document, QObject* parent) :
    QStyledItemDelegate(parent),
    document(document)
{

}

void GenericDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    setModelDataImp(editor, model, index);
}

QWidget* GenericDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    BaseColumn::Display display = getDisplayTypeFromIndex(index);

    if (display == BaseColumn::Display_Boolean)
    {
        return QItemEditorFactory::defaultFactory()->createEditor(QVariant::Bool, parent);
    }

    return createEditor(parent, option, index, display);
}

QWidget* GenericDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index, BaseColumn::Display display) const
{
    QVariant variant = index.data();

    if (!variant.isValid())
    {
        variant = index.data(Qt::DisplayRole);

        if (!variant.isValid())
        {
            return 0;
        }
    }

    switch (display)
    {
        // Will need to provide separate editor eventually
        case BaseColumn::Display_String:
        case BaseColumn::Display_LongString:
        {
            QPlainTextEdit* edit = new QPlainTextEdit(parent);
            edit->setUndoRedoEnabled(false);
            return edit;
        }
        case BaseColumn::Display_UnsignedInteger32:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<unsigned int>::min(), std::numeric_limits<unsigned int>::max());
            return spin;
        }
        case BaseColumn::Display_UnsignedInteger16:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<unsigned short>::min(), std::numeric_limits<unsigned short>::max());
            return spin;
        }
        case BaseColumn::Display_UnsignedInteger8:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<unsigned char>::min(), std::numeric_limits<unsigned char>::max());
            return spin;
        }
        case BaseColumn::Display_SignedInteger32:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            return spin;
        }
        case BaseColumn::Display_SignedInteger16:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<short>::min(), std::numeric_limits<short>::max());
            return spin;
        }
        case BaseColumn::Display_SignedInteger8:
        {
            QSpinBox* spin = new QSpinBox(parent);
            spin->setRange(std::numeric_limits<qint8>::min(), std::numeric_limits<qint8>::max());
            return spin;
        }
        case BaseColumn::Display_Var:
        {
            return new QLineEdit(parent);
        }
        case BaseColumn::Display_Float:
        {
            QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
            spin->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            spin->setSingleStep(0.01f);
            spin->setDecimals(3);
            return spin;
        }
        case BaseColumn::Display_Double:
        {
            QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
            spin->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
            spin->setSingleStep(0.01f);
            spin->setDecimals(6);
            return spin;
        }
        case BaseColumn::Display_Boolean:
        {
            return new QCheckBox(parent);
        }
        default:
        {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }
    }
}

void GenericDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    setEditorData(editor, index, false);
}

void GenericDelegate::setEditorData(QWidget* editor, const QModelIndex& index, bool tryDisplay) const
{
    QVariant variant = index.data(Qt::EditRole);

    if (tryDisplay)
    {
        if (!variant.isValid())
        {
            variant = index.data(Qt::DisplayRole);

            if (!variant.isValid())
            {
                return;
            }
        }
    }

    QPlainTextEdit* edit = qobject_cast<QPlainTextEdit*>(editor);
    if (edit)
    {
        edit->setPlainText(variant.toString());
        return;
    }

    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit)
    {
        lineEdit->setText(variant.toString());
        return;
    }

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
    if (spinBox)
    {
        spinBox->setValue(variant.toUInt());
        return;
    }

    QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor);
    if (doubleSpinBox)
    {
        doubleSpinBox->setValue(variant.toDouble());
        return;
    }

    QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
    if (checkBox)
    {
        checkBox->setChecked(variant.toBool());
        return;
    }
}

Document& GenericDelegate::getDocument() const
{
    return document;
}

BaseColumn::Display GenericDelegate::getDisplayTypeFromIndex(const QModelIndex& index) const
{
    int raw = index.data(BaseColumn::Role_Display).toInt();
    
    return static_cast<BaseColumn::Display>(raw);
}

void GenericDelegate::setModelDataImp(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QVariant variant;

    QPlainTextEdit* edit = qobject_cast<QPlainTextEdit*>(editor);
    if (edit)
    {
        variant = edit->toPlainText();
    }
    else
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (lineEdit)
        {
            variant = lineEdit->text();
        }
        else
        {
            QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
            if (spinBox)
            {
                variant = spinBox->value();
            }
            else
            {
                QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor);
                if (doubleSpinBox)
                {
                    variant = doubleSpinBox->value();
                }
                else
                {
                    QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
                    if (checkBox)
                    {
                        variant = checkBox->isChecked();
                    }
                }
            }
        }
    }

    if (model->data(index) != variant && model->flags(index) & Qt::ItemIsEditable)
    {
        model->setData(index, variant, Qt::EditRole);
    }
}
