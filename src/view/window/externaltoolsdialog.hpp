#ifndef EXTERNALTOOLSDIALOG_HPP
#define EXTERNALTOOLSDIALOG_HPP

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>

class ExternalToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExternalToolsDialog(QWidget* parent = nullptr);
    ~ExternalToolsDialog();

private slots:
    void onLaunchNifSkope();
    void onLaunchGimp();
    void onLaunchWryeBash();
    void onLaunchTes5Edit();
    void onBrowseNifSkope();
    void onBrowseGimp();
    void onBrowseWryeBash();
    void onBrowseTes5Edit();
    void onSaveSettings();

private:
    void setupUI();
    void loadSettings();

    QLineEdit* nifSkopePath;
    QLineEdit* gimpPath;
    QLineEdit* wryeBashPath;
    QLineEdit* tes5EditPath;

    QPushButton* nifSkopeButton;
    QPushButton* gimpButton;
    QPushButton* wryeBashButton;
    QPushButton* tes5EditButton;

    QPushButton* browseNifSkopeButton;
    QPushButton* browseGimpButton;
    QPushButton* browseWryeBashButton;
    QPushButton* browseTes5EditButton;

    QPushButton* saveButton;
};

#endif // EXTERNALTOOLSDIALOG_HPP
