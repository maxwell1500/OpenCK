#ifndef WORLDSpace_EDITOR_HPP
#define WORLDSpace_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct WorldspaceRecord;

class WorldspaceEditor : public QDialog
{
    Q_OBJECT

public:
    WorldspaceEditor(Data* data, WorldspaceRecord* worldspace, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromWorldspace();

    Data* mData;
    WorldspaceRecord* mWorldspace;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mNameEdit;
    QLineEdit* mIconPathEdit;
    QSpinBox* mWaterTypeSpin;
    QSpinBox* mTemplSpin;
    QSpinBox* mTerrainSpin;
    QLineEdit* mMapImageEdit;
    QLineEdit* mLodNoiseEdit;
    QLineEdit* mBillboardTextureEdit;
    QSpinBox* mMusicSpin;
};

#endif // WORLDSpace_EDITOR_HPP
