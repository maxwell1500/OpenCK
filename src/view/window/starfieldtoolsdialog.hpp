#ifndef STARFIELDTOOLSDIALOG_HPP
#define STARFIELDTOOLSDIALOG_HPP

#include <QDialog>

#include "../../model/tools/starfielddefinitions.hpp"
#include "../../model/tools/galaxymap.hpp"
#include "../../model/tools/robovoicer.hpp"
#include "../../model/tools/houdinibridge.hpp"

class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;
class GalaxyViewWidget;

// Starfield feature-slot editors (REMAINING.md §3.8) in one tabbed dialog:
// spaceship, galaxy view, reflection probe, crowd region, morph/face,
// RoboVoicer (voice-line plan) and the Houdini launch bridge. Each data
// tab loads/saves the model's JSON; Houdini previews the generated command.
class StarfieldToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StarfieldToolsDialog(QWidget* parent = nullptr);

private slots:
    void loadSpaceship();
    void saveSpaceship();
    void addShipModule();
    void removeShipModule();
    void loadGalaxy();
    void saveGalaxy();
    void addGalaxySystem();
    void removeGalaxySystem();
    void addGalaxyPlanet();
    void removeGalaxyPlanet();
    void onGalaxySystemSelected(int index);
    void loadProbe();
    void saveProbe();
    void loadCrowd();
    void saveCrowd();
    void addCrowdMember();
    void removeCrowdMember();
    void loadMorph();
    void saveMorph();
    void addMorphChannel();
    void removeMorphChannel();
    void loadVoicePlan();
    void saveVoicePlan();
    void addVoiceLine();
    void removeVoiceLine();
    void runVoicePlan();
    void refreshHoudiniCommand();

private:
    void syncShipToWidgets();
    void syncShipFromWidgets();
    void syncGalaxyToWidgets();
    void syncGalaxyFromWidgets();
    void refreshGalaxyPlanets();
    void syncProbeToWidgets();
    void syncProbeFromWidgets();
    void syncCrowdToWidgets();
    void syncCrowdFromWidgets();
    void syncMorphToWidgets();
    void syncMorphFromWidgets();
    void syncVoiceToWidgets();
    void syncVoiceFromWidgets();

    static QString jsonFilter();

    // Spaceship tab
    SpaceshipDefinition m_ship;
    QLineEdit* m_shipId = nullptr;
    QLineEdit* m_shipName = nullptr;
    QLineEdit* m_shipClass = nullptr;
    QLineEdit* m_shipReactor = nullptr;
    QLineEdit* m_shipGrav = nullptr;
    QLineEdit* m_shipShield = nullptr;
    QLineEdit* m_shipEngine = nullptr;
    QSpinBox* m_shipEngineCount = nullptr;
    QSpinBox* m_shipCargo = nullptr;
    QSpinBox* m_shipCrew = nullptr;
    QDoubleSpinBox* m_shipMass = nullptr;
    QDoubleSpinBox* m_shipHull = nullptr;
    QTableWidget* m_shipModuleTable = nullptr;

    // Galaxy tab
    GalaxyMap m_galaxy;
    GalaxyViewWidget* m_galaxyView = nullptr;
    QListWidget* m_galaxySystemList = nullptr;
    QTableWidget* m_galaxyPlanetTable = nullptr;
    QLineEdit* m_galaxyName = nullptr;

    // Reflection probe tab
    ReflectionProbeDefinition m_probe;
    QLineEdit* m_probeId = nullptr;
    QDoubleSpinBox* m_probeX = nullptr;
    QDoubleSpinBox* m_probeY = nullptr;
    QDoubleSpinBox* m_probeZ = nullptr;
    QDoubleSpinBox* m_probeRadius = nullptr;
    QSpinBox* m_probeResolution = nullptr;
    QPushButton* m_probeBoxProjection = nullptr;
    QDoubleSpinBox* m_probeBrightness = nullptr;
    QLineEdit* m_probeShape = nullptr;

    // Crowd region tab
    CrowdRegionDefinition m_crowd;
    QLineEdit* m_crowdId = nullptr;
    QLineEdit* m_crowdBehavior = nullptr;
    QDoubleSpinBox* m_crowdX = nullptr;
    QDoubleSpinBox* m_crowdY = nullptr;
    QDoubleSpinBox* m_crowdZ = nullptr;
    QDoubleSpinBox* m_crowdRadius = nullptr;
    QDoubleSpinBox* m_crowdDensity = nullptr;
    QSpinBox* m_crowdMax = nullptr;
    QTableWidget* m_crowdMemberTable = nullptr;

    // Morph tab
    MorphDefinition m_morph;
    QLineEdit* m_morphId = nullptr;
    QLineEdit* m_morphRace = nullptr;
    QTableWidget* m_morphChannelTable = nullptr;

    // RoboVoicer tab
    VoiceLinePlan m_voicePlan;
    QLineEdit* m_voicePlanName = nullptr;
    QTableWidget* m_voiceLineTable = nullptr;

    // Houdini tab
    HoudiniBridge m_houdini;
    QLineEdit* m_houdiniExe = nullptr;
    QLineEdit* m_houdiniHip = nullptr;
    QLineEdit* m_houdiniScript = nullptr;
    QLineEdit* m_houdiniExtra = nullptr;
    QTextEdit* m_houdiniPreview = nullptr;
};

#endif // STARFIELDTOOLSDIALOG_HPP
