#ifndef NIFCOMPARISONDIALOG_H
#define NIFCOMPARISONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QString>
#include <QVector>

class NifComparisonDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NifComparisonDialog(const QString& nif1Path, const QString& nif2Path, QWidget* parent = nullptr);

private slots:
    void onNif1Selected();
    void onNif2Selected();
    void onCompareClicked();
    void onCloseClicked();

private:
    void loadComparison(const QString& nif1Path, const QString& nif2Path);
    void displayResults();

    QLabel* m_nif1Label;
    QLabel* m_nif2Label;
    QLabel* m_nif1Preview;
    QLabel* m_nif2Preview;
    QLabel* m_resultsLabel;
    QPushButton* m_selectNif1Button;
    QPushButton* m_selectNif2Button;
    QPushButton* m_compareButton;
    QPushButton* m_closeButton;

    QString m_nif1Path;
    QString m_nif2Path;
};

#endif // NIFCOMPARISONDIALOG_H
