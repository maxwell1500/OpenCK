#ifndef FORMPICKERWIDGET_HPP
#define FORMPICKERWIDGET_HPP

#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class QComboBox;

struct FormPickerEntry
{
    quint32 formId = 0;
    QString editorId;
    QString typeName;
};

class FormPickerWidget : public QWidget
{
public:
    explicit FormPickerWidget(const QVector<FormPickerEntry>& entries,
                              quint32 currentFormId = 0, QWidget* parent = nullptr);

    quint32 value() const { return m_value; }
    void setValue(quint32 formId);
    void setEntries(const QVector<FormPickerEntry>& entries);

private:
    void rebuild();
    void selectRow(int row);
    void selectNext(int direction);

    QVector<FormPickerEntry> m_entries;
    QLineEdit* m_search = nullptr;
    QComboBox* m_type = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_status = nullptr;
    quint32 m_value = 0;
};

#endif // FORMPICKERWIDGET_HPP
