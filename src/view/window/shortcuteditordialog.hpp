#ifndef SHORTCUTEDITORDIALOG_HPP
#define SHORTCUTEDITORDIALOG_HPP

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

class ShortcutEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutEditorDialog(QWidget* parent = nullptr);

signals:
    void shortcutsChanged();

private slots:
    void recordShortcut();
    void resetSelected();
    void resetAll();
    void saveAndClose();

private:
    void populateTable();
    void updateButtonStates();
    bool eventFilter(QObject* obj, QEvent* event) override;

    QTableWidget* mTable;
    QPushButton* mRecordBtn;
    QPushButton* mResetBtn;
    QPushButton* mResetAllBtn;
    QPushButton* mSaveBtn;
    int mRecordingRow;
    bool mRecording;
};

#endif // SHORTCUTEDITORDIALOG_HPP
