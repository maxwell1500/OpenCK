#ifndef MODMANAGERDIALOG_HPP
#define MODMANAGERDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class ModManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModManagerDialog(QWidget* parent = nullptr);
    ~ModManagerDialog();

private slots:
    void onRefreshDetection();
    void onOpenModManager();
    void onLaunchWithProfile();

private:
    void setupUI();
    void populateUI();

    QLabel* managerStatusLabel;
    QLabel* installPathLabel;
    QLabel* versionLabel;
    QLabel* gamePathLabel;
    QLabel* statusLabel;
    QComboBox* profileCombo;
    QListWidget* modListWidget;
    QPushButton* refreshButton;
    QPushButton* openManagerButton;
    QPushButton* launchProfileButton;
};

#endif // MODMANAGERDIALOG_HPP
