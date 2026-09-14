#include "starfieldtoolsdialog.hpp"

#include "galaxyviewwidget.hpp"
#include "voicepreview.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QTableWidget>
#include <QTextEdit>

namespace {

QString tableText(QTableWidget* table, int row, int col)
{
    auto* item = table->item(row, col);
    return item ? item->text() : QString();
}

void setTableText(QTableWidget* table, int row, int col, const QString& text)
{
    table->setItem(row, col, new QTableWidgetItem(text));
}

} // namespace

// ===========================================================================
// Spaceship
// ===========================================================================
void StarfieldToolsDialog::syncShipToWidgets()
{
    m_shipId->setText(m_ship.editorId);
    m_shipName->setText(m_ship.name);
    m_shipClass->setText(m_ship.shipClass);
    m_shipReactor->setText(m_ship.reactorId);
    m_shipGrav->setText(m_ship.gravDriveId);
    m_shipShield->setText(m_ship.shieldId);
    m_shipEngine->setText(m_ship.engineId);
    m_shipEngineCount->setValue(m_ship.engineCount);
    m_shipCargo->setValue(m_ship.cargoCapacity);
    m_shipCrew->setValue(m_ship.crewCapacity);
    m_shipMass->setValue(m_ship.mass);
    m_shipHull->setValue(m_ship.hull);

    m_shipModuleTable->setRowCount(m_ship.modules.size());
    for (int i = 0; i < m_ship.modules.size(); ++i)
    {
        setTableText(m_shipModuleTable, i, 0, m_ship.modules[i].slot);
        setTableText(m_shipModuleTable, i, 1, m_ship.modules[i].id);
        setTableText(m_shipModuleTable, i, 2, QString::number(m_ship.modules[i].count));
    }
}

void StarfieldToolsDialog::syncShipFromWidgets()
{
    m_ship.editorId = m_shipId->text().trimmed();
    m_ship.name = m_shipName->text().trimmed();
    m_ship.shipClass = m_shipClass->text().trimmed();
    m_ship.reactorId = m_shipReactor->text().trimmed();
    m_ship.gravDriveId = m_shipGrav->text().trimmed();
    m_ship.shieldId = m_shipShield->text().trimmed();
    m_ship.engineId = m_shipEngine->text().trimmed();
    m_ship.engineCount = m_shipEngineCount->value();
    m_ship.cargoCapacity = m_shipCargo->value();
    m_ship.crewCapacity = m_shipCrew->value();
    m_ship.mass = m_shipMass->value();
    m_ship.hull = m_shipHull->value();

    m_ship.modules.clear();
    for (int i = 0; i < m_shipModuleTable->rowCount(); ++i)
    {
        SpaceshipDefinition::Module m;
        m.slot = tableText(m_shipModuleTable, i, 0).trimmed();
        m.id = tableText(m_shipModuleTable, i, 1).trimmed();
        m.count = tableText(m_shipModuleTable, i, 2).toInt();
        if (!m.slot.isEmpty() || !m.id.isEmpty())
            m_ship.modules.append(m);
    }
}

void StarfieldToolsDialog::loadSpaceship()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Ship"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_ship = SpaceshipDefinition::fromJson(doc.object());
    syncShipToWidgets();
}

void StarfieldToolsDialog::saveSpaceship()
{
    syncShipFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Ship"),
        m_ship.editorId.isEmpty() ? QStringLiteral("ship.json")
                                  : m_ship.editorId + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_ship.toJson()).toJson(QJsonDocument::Indented));
}

void StarfieldToolsDialog::addShipModule()
{
    const int row = m_shipModuleTable->rowCount();
    m_shipModuleTable->insertRow(row);
    setTableText(m_shipModuleTable, row, 0, QStringLiteral("Weapon"));
    setTableText(m_shipModuleTable, row, 1, QString());
    setTableText(m_shipModuleTable, row, 2, QStringLiteral("1"));
}

void StarfieldToolsDialog::removeShipModule()
{
    const int row = m_shipModuleTable->currentRow();
    if (row >= 0)
        m_shipModuleTable->removeRow(row);
}

// ===========================================================================
// Galaxy
// ===========================================================================
void StarfieldToolsDialog::syncGalaxyToWidgets()
{
    m_galaxyName->setText(m_galaxy.name);
    m_galaxyView->setMap(m_galaxy);

    m_galaxySystemList->clear();
    for (const auto& s : m_galaxy.systems)
        m_galaxySystemList->addItem(
            QStringLiteral("%1 (%2)").arg(s.name).arg(s.planets.size()));
    if (!m_galaxy.systems.isEmpty())
        m_galaxySystemList->setCurrentRow(0);

    refreshGalaxyPlanets();
}

void StarfieldToolsDialog::syncGalaxyFromWidgets()
{
    m_galaxy.name = m_galaxyName->text().trimmed();

    for (int i = 0; i < m_galaxy.systems.size(); ++i)
    {
        // Keep positions; the list only shows names, preserved by index.
        Q_UNUSED(i);
    }

    // Planets table belongs to the current system row.
    const int sys = m_galaxySystemList->currentRow();
    if (sys >= 0 && sys < m_galaxy.systems.size())
    {
        auto& planets = m_galaxy.systems[sys].planets;
        planets.clear();
        for (int r = 0; r < m_galaxyPlanetTable->rowCount(); ++r)
        {
            GalaxyMap::Planet p;
            p.name = tableText(m_galaxyPlanetTable, r, 0).trimmed();
            p.type = tableText(m_galaxyPlanetTable, r, 1).trimmed();
            p.planetEditorId = tableText(m_galaxyPlanetTable, r, 2).trimmed();
            p.moons = tableText(m_galaxyPlanetTable, r, 3).toInt();
            if (!p.name.isEmpty())
                planets.append(p);
        }
    }

    m_galaxyView->setMap(m_galaxy);
}

void StarfieldToolsDialog::refreshGalaxyPlanets()
{
    const int sys = m_galaxySystemList->currentRow();
    m_galaxyPlanetTable->setRowCount(0);
    if (sys < 0 || sys >= m_galaxy.systems.size())
        return;

    const auto& planets = m_galaxy.systems[sys].planets;
    for (const auto& p : planets)
    {
        const int row = m_galaxyPlanetTable->rowCount();
        m_galaxyPlanetTable->insertRow(row);
        setTableText(m_galaxyPlanetTable, row, 0, p.name);
        setTableText(m_galaxyPlanetTable, row, 1, p.type);
        setTableText(m_galaxyPlanetTable, row, 2, p.planetEditorId);
        setTableText(m_galaxyPlanetTable, row, 3, QString::number(p.moons));
    }
    m_galaxyView->setSelectedSystem(sys);
}

void StarfieldToolsDialog::onGalaxySystemSelected(int index)
{
    if (index < 0 || index >= m_galaxySystemList->count())
        return;
    if (m_galaxySystemList->currentRow() != index)
        m_galaxySystemList->setCurrentRow(index);
    refreshGalaxyPlanets();
}

void StarfieldToolsDialog::addGalaxySystem()
{
    syncGalaxyFromWidgets();
    GalaxyMap::StarSystem s;
    s.name = tr("New System %1").arg(m_galaxy.systems.size() + 1);
    m_galaxy.systems.append(s);
    syncGalaxyToWidgets();
    m_galaxySystemList->setCurrentRow(m_galaxy.systems.size() - 1);
}

void StarfieldToolsDialog::removeGalaxySystem()
{
    syncGalaxyFromWidgets();
    const int sys = m_galaxySystemList->currentRow();
    if (sys >= 0 && sys < m_galaxy.systems.size())
        m_galaxy.systems.remove(sys);
    syncGalaxyToWidgets();
}

void StarfieldToolsDialog::addGalaxyPlanet()
{
    const int row = m_galaxyPlanetTable->rowCount();
    m_galaxyPlanetTable->insertRow(row);
    setTableText(m_galaxyPlanetTable, row, 0, tr("New Planet"));
    setTableText(m_galaxyPlanetTable, row, 1, QStringLiteral("Rock"));
    setTableText(m_galaxyPlanetTable, row, 2, QString());
    setTableText(m_galaxyPlanetTable, row, 3, QStringLiteral("0"));
}

void StarfieldToolsDialog::removeGalaxyPlanet()
{
    const int row = m_galaxyPlanetTable->currentRow();
    if (row >= 0)
        m_galaxyPlanetTable->removeRow(row);
}

void StarfieldToolsDialog::loadGalaxy()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Galaxy"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_galaxy = GalaxyMap::fromJson(doc.object());
    syncGalaxyToWidgets();
}

void StarfieldToolsDialog::saveGalaxy()
{
    syncGalaxyFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Galaxy"),
        m_galaxy.name.isEmpty() ? QStringLiteral("galaxy.json")
                                : m_galaxy.name + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_galaxy.toJson()).toJson(QJsonDocument::Indented));
}

// ===========================================================================
// Reflection probe
// ===========================================================================
void StarfieldToolsDialog::syncProbeToWidgets()
{
    m_probeId->setText(m_probe.editorId);
    m_probeX->setValue(m_probe.x);
    m_probeY->setValue(m_probe.y);
    m_probeZ->setValue(m_probe.z);
    m_probeRadius->setValue(m_probe.radius);
    m_probeResolution->setValue(m_probe.resolution);
    m_probeBoxProjection->setChecked(m_probe.boxProjection);
    m_probeBoxProjection->setText(m_probe.boxProjection ? tr("Box Projection: On")
                                                       : tr("Box Projection: Off"));
    m_probeBrightness->setValue(m_probe.brightness);
    m_probeShape->setText(m_probe.shape);
}

void StarfieldToolsDialog::syncProbeFromWidgets()
{
    m_probe.editorId = m_probeId->text().trimmed();
    m_probe.x = m_probeX->value();
    m_probe.y = m_probeY->value();
    m_probe.z = m_probeZ->value();
    m_probe.radius = m_probeRadius->value();
    m_probe.resolution = m_probeResolution->value();
    m_probe.boxProjection = m_probeBoxProjection->isChecked();
    m_probe.brightness = m_probeBrightness->value();
    m_probe.shape = m_probeShape->text().trimmed();
    if (m_probe.shape.isEmpty())
        m_probe.shape = QStringLiteral("Sphere");
}

void StarfieldToolsDialog::loadProbe()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Probe"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_probe = ReflectionProbeDefinition::fromJson(doc.object());
    syncProbeToWidgets();
}

void StarfieldToolsDialog::saveProbe()
{
    syncProbeFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Probe"),
        m_probe.editorId.isEmpty() ? QStringLiteral("probe.json")
                                   : m_probe.editorId + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_probe.toJson()).toJson(QJsonDocument::Indented));
}

// ===========================================================================
// Crowd region
// ===========================================================================
void StarfieldToolsDialog::syncCrowdToWidgets()
{
    m_crowdId->setText(m_crowd.editorId);
    m_crowdBehavior->setText(m_crowd.behavior);
    m_crowdX->setValue(m_crowd.x);
    m_crowdY->setValue(m_crowd.y);
    m_crowdZ->setValue(m_crowd.z);
    m_crowdRadius->setValue(m_crowd.radius);
    m_crowdDensity->setValue(m_crowd.density);
    m_crowdMax->setValue(m_crowd.maxMembers);

    m_crowdMemberTable->setRowCount(m_crowd.members.size());
    for (int i = 0; i < m_crowd.members.size(); ++i)
    {
        setTableText(m_crowdMemberTable, i, 0, m_crowd.members[i].id);
        setTableText(m_crowdMemberTable, i, 1,
                     QString::number(m_crowd.members[i].weight, 'f', 2));
    }
}

void StarfieldToolsDialog::syncCrowdFromWidgets()
{
    m_crowd.editorId = m_crowdId->text().trimmed();
    m_crowd.behavior = m_crowdBehavior->text().trimmed();
    m_crowd.x = m_crowdX->value();
    m_crowd.y = m_crowdY->value();
    m_crowd.z = m_crowdZ->value();
    m_crowd.radius = m_crowdRadius->value();
    m_crowd.density = m_crowdDensity->value();
    m_crowd.maxMembers = m_crowdMax->value();

    m_crowd.members.clear();
    for (int i = 0; i < m_crowdMemberTable->rowCount(); ++i)
    {
        CrowdRegionDefinition::Member m;
        m.id = tableText(m_crowdMemberTable, i, 0).trimmed();
        m.weight = tableText(m_crowdMemberTable, i, 1).toDouble();
        if (!m.id.isEmpty())
            m_crowd.members.append(m);
    }
}

void StarfieldToolsDialog::loadCrowd()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Region"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_crowd = CrowdRegionDefinition::fromJson(doc.object());
    syncCrowdToWidgets();
}

void StarfieldToolsDialog::saveCrowd()
{
    syncCrowdFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Region"),
        m_crowd.editorId.isEmpty() ? QStringLiteral("crowd.json")
                                   : m_crowd.editorId + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_crowd.toJson()).toJson(QJsonDocument::Indented));
}

void StarfieldToolsDialog::addCrowdMember()
{
    const int row = m_crowdMemberTable->rowCount();
    m_crowdMemberTable->insertRow(row);
    setTableText(m_crowdMemberTable, row, 0, QStringLiteral("Citizen"));
    setTableText(m_crowdMemberTable, row, 1, QStringLiteral("1.00"));
}

void StarfieldToolsDialog::removeCrowdMember()
{
    const int row = m_crowdMemberTable->currentRow();
    if (row >= 0)
        m_crowdMemberTable->removeRow(row);
}

// ===========================================================================
// Morph / face
// ===========================================================================
void StarfieldToolsDialog::syncMorphToWidgets()
{
    m_morphId->setText(m_morph.editorId);
    m_morphRace->setText(m_morph.raceId);

    m_morphChannelTable->setRowCount(m_morph.channels.size());
    for (int i = 0; i < m_morph.channels.size(); ++i)
    {
        setTableText(m_morphChannelTable, i, 0, m_morph.channels[i].name);
        setTableText(m_morphChannelTable, i, 1,
                     QString::number(m_morph.channels[i].value, 'f', 3));
        setTableText(m_morphChannelTable, i, 2,
                     QString::number(m_morph.channels[i].minimum, 'f', 3));
        setTableText(m_morphChannelTable, i, 3,
                     QString::number(m_morph.channels[i].maximum, 'f', 3));
    }
}

void StarfieldToolsDialog::syncMorphFromWidgets()
{
    m_morph.editorId = m_morphId->text().trimmed();
    m_morph.raceId = m_morphRace->text().trimmed();

    m_morph.channels.clear();
    for (int i = 0; i < m_morphChannelTable->rowCount(); ++i)
    {
        MorphDefinition::Channel c;
        c.name = tableText(m_morphChannelTable, i, 0).trimmed();
        c.value = tableText(m_morphChannelTable, i, 1).toDouble();
        c.minimum = tableText(m_morphChannelTable, i, 2).toDouble();
        c.maximum = tableText(m_morphChannelTable, i, 3).toDouble();
        if (c.maximum < c.minimum)
            qSwap(c.maximum, c.minimum);
        c.value = qBound(c.minimum, c.value, c.maximum);
        if (!c.name.isEmpty())
            m_morph.channels.append(c);
    }
}

void StarfieldToolsDialog::loadMorph()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Morph"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_morph = MorphDefinition::fromJson(doc.object());
    syncMorphToWidgets();
}

void StarfieldToolsDialog::saveMorph()
{
    syncMorphFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Morph"),
        m_morph.editorId.isEmpty() ? QStringLiteral("morph.json")
                                   : m_morph.editorId + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_morph.toJson()).toJson(QJsonDocument::Indented));
}

void StarfieldToolsDialog::addMorphChannel()
{
    const int row = m_morphChannelTable->rowCount();
    m_morphChannelTable->insertRow(row);
    const QStringList common = MorphDefinition::commonChannels();
    setTableText(m_morphChannelTable, row, 0,
                 common.value(row % common.size(), QStringLiteral("Custom")));
    setTableText(m_morphChannelTable, row, 1, QStringLiteral("0.000"));
    setTableText(m_morphChannelTable, row, 2, QStringLiteral("-1.000"));
    setTableText(m_morphChannelTable, row, 3, QStringLiteral("1.000"));
}

void StarfieldToolsDialog::removeMorphChannel()
{
    const int row = m_morphChannelTable->currentRow();
    if (row >= 0)
        m_morphChannelTable->removeRow(row);
}

// ===========================================================================
// RoboVoicer
// ===========================================================================
void StarfieldToolsDialog::syncVoiceToWidgets()
{
    m_voicePlanName->setText(m_voicePlan.name);

    m_voiceLineTable->setRowCount(m_voicePlan.lines.size());
    for (int i = 0; i < m_voicePlan.lines.size(); ++i)
    {
        const auto& l = m_voicePlan.lines[i];
        setTableText(m_voiceLineTable, i, 0, l.lineId);
        setTableText(m_voiceLineTable, i, 1, l.speaker);
        setTableText(m_voiceLineTable, i, 2, l.text);
        setTableText(m_voiceLineTable, i, 3, l.voiceId);
        setTableText(m_voiceLineTable, i, 4, l.outputPath);
        setTableText(m_voiceLineTable, i, 5, l.done ? tr("yes") : tr("no"));
    }
}

void StarfieldToolsDialog::syncVoiceFromWidgets()
{
    m_voicePlan.name = m_voicePlanName->text().trimmed();

    m_voicePlan.lines.clear();
    for (int i = 0; i < m_voiceLineTable->rowCount(); ++i)
    {
        VoiceLine l;
        l.lineId = tableText(m_voiceLineTable, i, 0).trimmed();
        l.speaker = tableText(m_voiceLineTable, i, 1).trimmed();
        l.text = tableText(m_voiceLineTable, i, 2);
        l.voiceId = tableText(m_voiceLineTable, i, 3).trimmed();
        l.outputPath = tableText(m_voiceLineTable, i, 4).trimmed();
        const QString doneText = tableText(m_voiceLineTable, i, 5).trimmed().toLower();
        l.done = (doneText == QStringLiteral("yes") || doneText == QStringLiteral("1")
                  || doneText == QStringLiteral("true"));
        if (!l.lineId.isEmpty() || !l.text.isEmpty())
            m_voicePlan.lines.append(l);
    }
}

void StarfieldToolsDialog::loadVoicePlan()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Plan"), QString(), tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    m_voicePlan = VoiceLinePlan::fromJson(doc.object());
    syncVoiceToWidgets();
}

void StarfieldToolsDialog::saveVoicePlan()
{
    syncVoiceFromWidgets();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Plan"),
        m_voicePlan.name.isEmpty() ? QStringLiteral("voiceplan.json")
                                   : m_voicePlan.name + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(m_voicePlan.toJson()).toJson(QJsonDocument::Indented));
}

void StarfieldToolsDialog::addVoiceLine()
{
    const int row = m_voiceLineTable->rowCount();
    m_voiceLineTable->insertRow(row);
    setTableText(m_voiceLineTable, row, 0, tr("LINE_%1").arg(row + 1, 3, 10, QChar('0')));
    setTableText(m_voiceLineTable, row, 5, tr("no"));
}

void StarfieldToolsDialog::removeVoiceLine()
{
    const int row = m_voiceLineTable->currentRow();
    if (row >= 0)
        m_voiceLineTable->removeRow(row);
}

void StarfieldToolsDialog::runVoicePlan()
{
    syncVoiceFromWidgets();

#ifdef _WIN32
    SapiVoiceSynthesizer sapi;
    IVoiceSynthesizer* synth = sapi.isAvailable() ? &sapi : nullptr;
    const QString engineName = synth ? sapi.name()
                                     : tr("none (no SAPI voices installed)");
#else
    IVoiceSynthesizer* synth = nullptr;
    const QString engineName = tr("none (SAPI is Windows-only)");
#endif

    const VoiceRunReport report = ::runVoicePlan(m_voicePlan, synth);
    syncVoiceToWidgets();

    QMessageBox::information(this, tr("RoboVoicer"),
        tr("Plan run with speech engine: %1.\n"
           "Completed: %2\nFailed (pending): %3")
            .arg(engineName)
            .arg(report.completed)
            .arg(report.failed));

    if (report.completed <= 0)
        return;

    // Offer in-engine playback of the first completed line's WAV.
    QString previewPath;
    for (const VoiceLine& line : m_voicePlan.lines)
    {
        if (line.done && QFileInfo::exists(line.outputPath))
        {
            previewPath = line.outputPath;
            break;
        }
    }
    if (previewPath.isEmpty())
        return;
    if (QMessageBox::question(this, tr("RoboVoicer"),
            tr("Play back the first completed line?\n%1").arg(previewPath))
        != QMessageBox::Yes)
        return;

    QFile wav(previewPath);
    if (!wav.open(QIODevice::ReadOnly))
        return;
    const QByteArray audio = wav.readAll();
    if (!VoicePreview::playVoiceAudio(audio, QString(), this))
    {
        QMessageBox::warning(this, tr("RoboVoicer"),
            tr("Could not play back %1.").arg(previewPath));
    }
}

// ===========================================================================
// Houdini bridge
// ===========================================================================
void StarfieldToolsDialog::refreshHoudiniCommand()
{
    m_houdini.houdiniExecutable = m_houdiniExe->text().trimmed();
    m_houdini.hipFile = m_houdiniHip->text().trimmed();

    const QString script = m_houdiniScript->text().trimmed();
    const QStringList extra = m_houdiniExtra->text().trimmed()
        .split(' ', Qt::SkipEmptyParts);
    m_houdini.extraArgs = extra;

    QString text;
    if (!m_houdini.isConfigured())
    {
        text = tr("Set an executable to preview the command.");
    }
    else
    {
        const QStringList cmd = script.isEmpty()
            ? m_houdini.interactiveCommand()
            : m_houdini.batchCommand(script, {});
        text = cmd.join(' ');
    }
    m_houdiniPreview->setPlainText(text);
}
