#ifndef WARNINGSDOCKWIDGET_HPP
#define WARNINGSDOCKWIDGET_HPP

#include <QWidget>

#include <QVector>

class QTableWidget;
class Message;
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

private:
    QTableWidget* mTable;
};

#endif // WARNINGSDOCKWIDGET_HPP
