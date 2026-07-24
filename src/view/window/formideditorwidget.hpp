#ifndef FORMIDEDITORWIDGET_HPP
#define FORMIDEDITORWIDGET_HPP

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>

class Data;

class FormIdEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FormIdEditorWidget(Data* data, QWidget* parent = nullptr);
    ~FormIdEditorWidget();

signals:
    void formIdChanged(const QString& oldFormId, const QString& newFormId);

private slots:
    void onSearch();
    void onConflictCheck();
    void onFormIdEdited(const QString& text);

private:
    void setupUI();
    void updateConflictInfo(const QString& formId);

    Data* mData;
    QLineEdit* searchEdit;
    QLabel* conflictLabel;
    QLabel* recordInfoLabel;
    QTableWidget* conflictTable;
    QPushButton* searchButton;
    QPushButton* conflictButton;
};

#endif // FORMIDEDITORWIDGET_HPP
