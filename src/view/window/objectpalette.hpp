#ifndef OBJECTPALETTE_HPP
#define OBJECTPALETTE_HPP

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QModelIndex>

class QListView;
class QLineEdit;
class QPushButton;
class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

class CellRecord;
class Data;

class ObjectFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ObjectFilterProxyModel(QObject* parent = nullptr);
    void setSearchPattern(const QString& pattern);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString searchPattern;
};

class ObjectPalette : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectPalette(Data* data, QWidget* parent = nullptr);
    ~ObjectPalette();

    void loadCell(CellRecord* cell);
    void clear();
    void populateObjectList();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onObjectSelected(const QModelIndex& index);
    void onSavePlacementClicked();
    void onLoadPlacementClicked();
    void onTogglePlacementMode();

private:
    void setupUI();
    void renderPlacement();
    void syncPlacementsToCell();
    void syncPlacementsToRefrCollection();

    // Data model
    Data* mData;

    // Cell data
    CellRecord* currentCell;

    // Placement data
    struct Placement {
        quint32 baseObjectFormId;
        QString baseObjectName;
        float x, y, z;
        float rotX, rotY, rotZ;
        float scale;
        bool active;
    };
    QVector<Placement> placements;

    quint32 allocateRefFormId();
    float snapToGrid(float value, int gridSize) const;
    quint32 resolveFormIdFromName(const QString& editorId) const;
    void applyPlacement(const Placement& placement);

    // UI elements
    QLineEdit* searchEdit;
    QPushButton* saveButton;
    QPushButton* loadButton;
    QPushButton* placementModeButton;
    QLabel* statusLabel;
    QLabel* objectCountLabel;

    // Placement settings
    QDoubleSpinBox* xSpin;
    QDoubleSpinBox* ySpin;
    QDoubleSpinBox* zSpin;
    QDoubleSpinBox* rotXSpin;
    QDoubleSpinBox* rotYSpin;
    QDoubleSpinBox* rotZSpin;
    QDoubleSpinBox* scaleSpin;
    QCheckBox* activeCheckBox;

    // Grid snap settings
    QSpinBox* gridSizeSpin;
    QCheckBox* snapPositionCheck;
    QCheckBox* snapRotationCheck;

    // Model-based list view
    QStandardItemModel* objectModel;
    ObjectFilterProxyModel* filterProxyModel;
    QListView* objectListView;

    bool placementMode;
    QString selectedObject;
};

#endif // OBJECTPALETTE_HPP
