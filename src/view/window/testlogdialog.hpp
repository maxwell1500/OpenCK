#ifndef TESTLOGDIALOG_HPP
#define TESTLOGDIALOG_HPP

#include <QDialog>
#include <QByteArray>

class QPlainTextEdit;
class QPushButton;
class QProcess;

class TestLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TestLogDialog(QWidget* parent = nullptr);
    void appendLine(const QString& line);
    static int runAllTests(QWidget* parent);

private slots:
    void startSelfTest();

private:
    void drainOutput(QProcess* process);

    QPlainTextEdit* mLogView;
    QPushButton* mRerunButton;
    QByteArray mOutputBuffer;
    int mExitCode = 0;
};

#endif // TESTLOGDIALOG_HPP
