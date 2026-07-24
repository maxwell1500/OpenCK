#ifndef GENERIC_DELEGATE_H
#define GENERIC_DELEGATE_H

#include "../../model/doc/document.hpp"
#include "../../model/world/basecolumn.hpp"

#include <QStyledItemDelegate>

class GenericDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    GenericDelegate(Document& document, QObject* parent);

    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index, BaseColumn::Display display) const;

    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index, bool tryDisplay) const;

protected:
    Document& getDocument() const;
    BaseColumn::Display getDisplayTypeFromIndex(const QModelIndex& index) const;

    virtual void setModelDataImp(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;


private:
    Document& document;
};

#endif // GENERIC_DELEGATE_H