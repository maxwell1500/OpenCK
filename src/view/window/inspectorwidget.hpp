#ifndef INSPECTORWIDGET_HPP
#define INSPECTORWIDGET_HPP

#include <QWidget>

class QScrollArea;
class QTableWidget;
class QLabel;

namespace openck {
class EditorPropertyGrid;
class FormComponents;
}

class InspectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InspectorWidget(QWidget* parent = nullptr);
    ~InspectorWidget();

public slots:
    void onRecordSelected(int categoryId, int recordIndex, const QString& editorId);
    void clear();
    void showComponents(openck::FormComponents* components, const QString& title);

private:
    QScrollArea* m_scrollArea;
    openck::EditorPropertyGrid* m_grid;
    QLabel* m_titleLabel;
    QTableWidget* m_subRecordsTable;
    openck::FormComponents* m_currentComponents;
    QString m_currentFormIdKey;
};

#endif // INSPECTORWIDGET_HPP
