#pragma once

#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>

struct ParticleSystemData;
class ColorGradientWidget;
class SizeCurveWidget;

class ParticleEffectsEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ParticleEffectsEditor(QWidget* parent = nullptr);

private slots:
    void browseNif();
    void loadParticles(const QString& path);
    void onParticleSystemSelected(int index);
    void applyEmitterChanges();
    void applyRendererChanges();
    void applyColorGradient();
    void applySizeCurve();
    void exportParticles();

private:
    void updateEmitterTable();
    void updateRendererTable();
    void updateColorGradient();
    void updateSizeCurve();
    void loadEmitterDefaults();
    void loadRendererDefaults();

    QListWidget* mSystemList;
    QTableWidget* mEmitterTable;
    QTableWidget* mRendererTable;
    ColorGradientWidget* mColorGradient;
    SizeCurveWidget* mSizeCurve;
    QPushButton* mApplyEmitterBtn;
    QPushButton* mApplyRendererBtn;
    QPushButton* mApplyColorBtn;
    QPushButton* mApplySizeBtn;
    QList<ParticleSystemData*> mSystems;
    int mSelectedIndex;

signals:
    void particleSystemUpdated(const ParticleSystemData* data);
};
