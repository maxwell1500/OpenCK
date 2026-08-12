#include "testlogdialog.hpp"

#include "logger.hpp"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QProcess>
#include <QCoreApplication>

TestLogDialog::TestLogDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Test Log"));
    setMinimumSize(560, 360);

    mLogView = new QPlainTextEdit(this);
    mLogView->setReadOnly(true);

    auto* buttonBox = new QDialogButtonBox(this);
    mRerunButton = buttonBox->addButton(tr("Re-run"), QDialogButtonBox::ActionRole);
    QPushButton* closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(mRerunButton, &QPushButton::clicked, this, &TestLogDialog::startSelfTest);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(mLogView);
    layout->addWidget(buttonBox);
}

void TestLogDialog::appendLine(const QString& line)
{
    mLogView->appendPlainText(line);
}

void TestLogDialog::drainOutput(QProcess* process)
{
    mOutputBuffer += process->readAllStandardOutput();
    int idx;
    while ((idx = mOutputBuffer.indexOf('\n')) >= 0)
    {
        const QString line = QString::fromUtf8(mOutputBuffer.left(idx)).trimmed();
        mOutputBuffer.remove(0, idx + 1);
        if (!line.isEmpty())
        {
            appendLine(line);
        }
    }
}

void TestLogDialog::startSelfTest()
{
    mLogView->clear();
    mOutputBuffer.clear();
    mRerunButton->setEnabled(false);
    mExitCode = 0;
    LOG_INFO("Running CLI self-test from Tests menu");

    auto* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        drainOutput(process);
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
                drainOutput(process);
                mExitCode = exitCode;
                mRerunButton->setEnabled(true);
                appendLine(exitCode == 0 ? tr("All tests passed.")
                                         : tr("One or more tests failed (exit code %1).").arg(exitCode));
                process->deleteLater();
            });

    const QString exePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/openck.exe");
    process->start(exePath, { QStringLiteral("--cli"), QStringLiteral("selftest") });
}

int TestLogDialog::runAllTests(QWidget* parent)
{
    TestLogDialog dialog(parent);
    dialog.startSelfTest();
    dialog.exec();
    return dialog.mExitCode;
}
