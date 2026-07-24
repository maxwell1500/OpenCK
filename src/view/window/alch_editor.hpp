#ifndef ALCH_EDITOR_HPP
#define ALCH_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>

class Data;
struct AlchRecord;
struct RawSubRecord;

class AlchEditor : public QDialog
{
    Q_OBJECT

public:
    AlchEditor(Data* data, AlchRecord* alch, QWidget* parent = nullptr);

    AlchRecord* getRecord() const { return mRecord; }

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromAlch();
    void buildSynergies();
    void buildIngredientTable();

    static QVector<quint32> extractEffects(const QVector<RawSubRecord>& rawSubRecords);
    static QString schoolName(quint32 school);
    static bool areSchoolsOpposing(quint32 a, quint32 b);
    static bool areSchoolsSame(quint32 a, quint32 b);

    Data* mData;
    AlchRecord* mRecord;
    QString mOriginalEditorId;

    QTabWidget* mTabs;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mEnchantmentSpin;
    QDoubleSpinBox* mWeightSpin;
    QSpinBox* mValueSpin;

    QListWidget* mSynergiesList;
    QTableWidget* mIngredientTable;
};

#endif // ALCH_EDITOR_HPP
