#ifndef MODMANAGERDIALOG_HPP
#define MODMANAGERDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QLineEdit>
#include <QVBoxLayout>

#include "../../model/tools/moddeploymentresolver.hpp"

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
    void onResolveAsset();

private:
    void setupUI();
    void populateUI();
    void populateDeployment(const QString& gameDataDir);

    QLabel* managerStatusLabel;
    QLabel* installPathLabel;
    QLabel* versionLabel;
    QLabel* gamePathLabel;
    QLabel* statusLabel;
    QLabel* deploymentStatusLabel;
    QLabel* deploymentMethodLabel;
    QTableWidget* deploymentTable;
    QLineEdit* assetPathEdit;
    QLabel* assetResultLabel;
    QComboBox* profileCombo;
    QListWidget* modListWidget;
    QPushButton* refreshButton;
    QPushButton* openManagerButton;
    QPushButton* launchProfileButton;

    ModDeploymentResolver::VortexDeployment mDeployment;
};

#endif // MODMANAGERDIALOG_HPP
