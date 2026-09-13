#ifndef PLANETEDITORDIALOG_HPP
#define PLANETEDITORDIALOG_HPP

#include <QDialog>

#include "../../model/tools/planetdefinition.hpp"

class QLineEdit;
class QDoubleSpinBox;
class QTableWidget;
class QListWidget;

// Starfield PNDT planet editor (REMAINING.md §3.8). Edits a PlanetDefinition's
// star system, day length, gravity/temperature, biome table, traits and
// resources. Because no on-disk PNDT binary sample is available locally, the
// definition is loaded from / saved to JSON via the model's toJson/fromJson.
class PlanetEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlanetEditorDialog(QWidget* parent = nullptr);

    void setDefinition(const PlanetDefinition& def);
    PlanetDefinition definition() const;

private slots:
    void addBiome();
    void removeBiome();
    void addResource();
    void removeResource();
    void loadFromJson();
    void saveToJson();

private:
    void syncToWidgets();
    void syncFromWidgets();

    PlanetDefinition m_def;

    QLineEdit* m_editorId = nullptr;
    QLineEdit* m_starSystem = nullptr;
    QDoubleSpinBox* m_dayLength = nullptr;
    QLineEdit* m_gravity = nullptr;
    QLineEdit* m_temperature = nullptr;
    QTableWidget* m_biomeTable = nullptr;
    QListWidget* m_traitList = nullptr;
    QTableWidget* m_resourceTable = nullptr;
};

#endif // PLANETEDITORDIALOG_HPP
