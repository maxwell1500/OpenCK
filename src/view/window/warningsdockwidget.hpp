#ifndef WARNINGSDOCKWIDGET_HPP
#define WARNINGSDOCKWIDGET_HPP

#include <QWidget>

#include <QVector>

class QTableWidget;
class QPushButton;
struct Message;
class Messages;

class WarningsDockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WarningsDockWidget(QWidget* parent = nullptr);

    void addMessage(const Message& message);
    void setMessages(const Messages& messages);
    void clear();
    int count() const;

private slots:
    void onExport();

private:
    void setupUI();
    QTableWidget* mTable;
    QPushButton* mExportButton;
    QVector<Message> mStoredMessages;
};

#endif // WARNINGSDOCKWIDGET_HPP
