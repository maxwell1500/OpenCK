#ifndef NIFPREVIEWDIALOG_H
#define NIFPREVIEWDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QVector>
#include <QString>

/**
 * @brief Dialog for previewing NIF file images
 */
class NifPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NifPreviewDialog(const QString& nifPath, QWidget* parent = nullptr);
    ~NifPreviewDialog();

private slots:
    void onPreviousClicked();
    void onNextClicked();
    void onZoomInClicked();
    void onZoomOutClicked();
    void onFitToWindowClicked();
    void onActualSizeClicked();

private:
    void loadPreview(int index);
    void updateNavigationButtons();
    void updateZoomLabel();
    void generatePreviews();

    QLabel* m_previewLabel;
    QString m_nifPath;
    QString m_previewDir;
    QVector<QString> m_previewFiles;
    int m_currentIndex;
    double m_zoomLevel;

public:
    // UI elements
    QLabel* pageLabel = nullptr;
    QLabel* zoomLabel = nullptr;
    QPushButton* previousButton = nullptr;
    QPushButton* nextButton = nullptr;
    QPushButton* zoomInButton = nullptr;
    QPushButton* zoomOutButton = nullptr;
    QPushButton* fitButton = nullptr;
    QPushButton* actualSizeButton = nullptr;
};

#endif // NIFPREVIEWDIALOG_H
