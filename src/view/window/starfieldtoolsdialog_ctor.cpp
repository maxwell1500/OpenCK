#include "starfieldtoolsdialog.hpp"

#include "galaxyviewwidget.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QString jsonFilter()
{
    return QObject::tr("JSON files (*.json);;All files (*)");
}

QDoubleSpinBox* makeDoubleSpin(double lo, double hi, double value)
{
    auto* spin = new QDoubleSpinBox();
    spin->setRange(lo, hi);
    spin->setDecimals(2);
    spin->setValue(value);
    return spin;
}

QSpinBox* makeSpin(int lo, int hi, int value)
{
    auto* spin = new QSpinBox();
    spin->setRange(lo, hi);
    spin->setValue(value);
    return spin;
}

QTableWidget* makeTable(const QStringList& headers)
{
    auto* table = new QTableWidget(0, headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}

QPushButton* toolButton(const QString& text, QWidget* parent, QLayout* layout)
{
    auto* btn = new QPushButton(text, parent);
    auto* row = new QHBoxLayout();
    row->addWidget(btn);
    row->addStretch();
    layout->addItem(row);
    return btn;
}

QJsonObject readJsonFile(QWidget* parent, const QString& title)
{
    const QString path = QFileDialog::getOpenFileName(parent, title, QString(), jsonFilter());
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(parent, title, QObject::tr("Could not open %1").arg(path));
        return {};
    }
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        QMessageBox::warning(parent, title,
            QObject::tr("Invalid JSON: %1").arg(err.errorString()));
        return {};
    }
    QJsonObject obj = doc.object();
    obj.insert(QStringLiteral("__path"), path);
    return obj;
}

bool writeJsonFile(QWidget* parent, const QString& title,
                   const QString& suggestedName, const QJsonObject& obj)
{
    const QString path = QFileDialog::getSaveFileName(parent, title, suggestedName, jsonFilter());
    if (path.isEmpty())
        return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(parent, title, QObject::tr("Could not write %1").arg(path));
        return false;
    }
    QJsonObject out = obj;
    out.remove(QStringLiteral("__path"));
    file.write(QJsonDocument(out).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

} // namespace

QString StarfieldToolsDialog::jsonFilter()
{
    return ::jsonFilter();
}

// ===========================================================================
// Construction — one tab per §3.8 slot
// ===========================================================================
StarfieldToolsDialog::StarfieldToolsDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_INFO("StarfieldToolsDialog: opening Starfield tools");

    setWindowTitle(tr("Starfield Tools"));
    resize(780, 620);

    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    // ---- Spaceship tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* form = new QFormLayout();
        m_shipId = new QLineEdit(tab);
        m_shipName = new QLineEdit(tab);
        m_shipClass = new QLineEdit(tab);
        m_shipClass->setPlaceholderText(tr("A, B or C"));
        m_shipReactor = new QLineEdit(tab);
        m_shipGrav = new QLineEdit(tab);
        m_shipShield = new QLineEdit(tab);
        m_shipEngine = new QLineEdit(tab);
        m_shipEngineCount = makeSpin(0, 8, 1);
        m_shipCargo = makeSpin(0, 100000, 0);
        m_shipCrew = makeSpin(0, 40, 0);
        m_shipMass = makeDoubleSpin(0.0, 1000000.0, 0.0);
        m_shipHull = makeDoubleSpin(0.0, 100000.0, 0.0);
        form->addRow(tr("Editor ID:"), m_shipId);
        form->addRow(tr("Name:"), m_shipName);
        form->addRow(tr("Class:"), m_shipClass);
        form->addRow(tr("Reactor:"), m_shipReactor);
        form->addRow(tr("Grav Drive:"), m_shipGrav);
        form->addRow(tr("Shield:"), m_shipShield);
        form->addRow(tr("Engine:"), m_shipEngine);
        form->addRow(tr("Engines:"), m_shipEngineCount);
        form->addRow(tr("Cargo:"), m_shipCargo);
        form->addRow(tr("Crew:"), m_shipCrew);
        form->addRow(tr("Mass:"), m_shipMass);
        form->addRow(tr("Hull:"), m_shipHull);
        tabLayout->addLayout(form);

        tabLayout->addWidget(new QLabel(tr("Modules:"), tab));
        m_shipModuleTable = makeTable({tr("Slot"), tr("Module ID"), tr("Count")});
        tabLayout->addWidget(m_shipModuleTable);
        auto* addModuleBtn = toolButton(tr("Add Module"), tab, tabLayout);
        auto* removeModuleBtn = toolButton(tr("Remove Module"), tab, tabLayout);
        connect(addModuleBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::addShipModule);
        connect(removeModuleBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::removeShipModule);

        auto* loadBtn = toolButton(tr("Load Ship JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Ship JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadSpaceship);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveSpaceship);

        syncShipToWidgets();
        tabs->addTab(tab, tr("Spaceship"));
    }

    // ---- Galaxy tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* nameRow = new QHBoxLayout();
        nameRow->addWidget(new QLabel(tr("Galaxy:"), tab));
        m_galaxyName = new QLineEdit(tab);
        nameRow->addWidget(m_galaxyName);
        tabLayout->addLayout(nameRow);

        m_galaxyView = new GalaxyViewWidget(tab);
        tabLayout->addWidget(m_galaxyView, 1);
        connect(m_galaxyView, &GalaxyViewWidget::systemSelected,
                this, &StarfieldToolsDialog::onGalaxySystemSelected);

        auto* lists = new QHBoxLayout();
        m_galaxySystemList = new QListWidget(tab);
        lists->addWidget(m_galaxySystemList);
        m_galaxyPlanetTable = makeTable({tr("Name"), tr("Type"), tr("Planet ID"), tr("Moons")});
        lists->addWidget(m_galaxyPlanetTable);
        tabLayout->addLayout(lists);

        auto* sysBtns = new QHBoxLayout();
        auto* addSys = new QPushButton(tr("Add System"), tab);
        auto* removeSys = new QPushButton(tr("Remove System"), tab);
        auto* addPlanet = new QPushButton(tr("Add Planet"), tab);
        auto* removePlanet = new QPushButton(tr("Remove Planet"), tab);
        sysBtns->addWidget(addSys);
        sysBtns->addWidget(removeSys);
        sysBtns->addWidget(addPlanet);
        sysBtns->addWidget(removePlanet);
        sysBtns->addStretch();
        tabLayout->addLayout(sysBtns);
        connect(addSys, &QPushButton::clicked, this, &StarfieldToolsDialog::addGalaxySystem);
        connect(removeSys, &QPushButton::clicked, this, &StarfieldToolsDialog::removeGalaxySystem);
        connect(addPlanet, &QPushButton::clicked, this, &StarfieldToolsDialog::addGalaxyPlanet);
        connect(removePlanet, &QPushButton::clicked, this, &StarfieldToolsDialog::removeGalaxyPlanet);
        connect(m_galaxySystemList, &QListWidget::currentRowChanged,
                this, &StarfieldToolsDialog::onGalaxySystemSelected);

        auto* loadBtn = toolButton(tr("Load Galaxy JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Galaxy JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadGalaxy);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveGalaxy);

        syncGalaxyToWidgets();
        tabs->addTab(tab, tr("Galaxy"));
    }

    // ---- Reflection probe tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* form = new QFormLayout();
        m_probeId = new QLineEdit(tab);
        m_probeX = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_probeY = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_probeZ = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_probeRadius = makeDoubleSpin(1.0, 100000.0, 512.0);
        m_probeResolution = makeSpin(32, 1024, 128);
        m_probeBoxProjection = new QPushButton(tr("Box Projection: Off"), tab);
        m_probeBoxProjection->setCheckable(true);
        m_probeBrightness = makeDoubleSpin(0.0, 10.0, 1.0);
        m_probeShape = new QLineEdit(tab);
        m_probeShape->setPlaceholderText(tr("Sphere or Box"));
        form->addRow(tr("Editor ID:"), m_probeId);
        form->addRow(tr("X:"), m_probeX);
        form->addRow(tr("Y:"), m_probeY);
        form->addRow(tr("Z:"), m_probeZ);
        form->addRow(tr("Radius:"), m_probeRadius);
        form->addRow(tr("Resolution:"), m_probeResolution);
        form->addRow(tr("Projection:"), m_probeBoxProjection);
        form->addRow(tr("Brightness:"), m_probeBrightness);
        form->addRow(tr("Shape:"), m_probeShape);
        tabLayout->addLayout(form);
        tabLayout->addStretch();
        connect(m_probeBoxProjection, &QPushButton::toggled, this, [this](bool on) {
            m_probeBoxProjection->setText(on ? tr("Box Projection: On")
                                             : tr("Box Projection: Off"));
        });

        auto* loadBtn = toolButton(tr("Load Probe JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Probe JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadProbe);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveProbe);

        syncProbeToWidgets();
        tabs->addTab(tab, tr("Reflection Probe"));
    }

    // ---- Crowd region tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* form = new QFormLayout();
        m_crowdId = new QLineEdit(tab);
        m_crowdBehavior = new QLineEdit(tab);
        m_crowdX = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_crowdY = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_crowdZ = makeDoubleSpin(-100000.0, 100000.0, 0.0);
        m_crowdRadius = makeDoubleSpin(1.0, 100000.0, 1024.0);
        m_crowdDensity = makeDoubleSpin(0.0, 10.0, 0.1);
        m_crowdDensity->setDecimals(3);
        m_crowdMax = makeSpin(0, 500, 20);
        form->addRow(tr("Editor ID:"), m_crowdId);
        form->addRow(tr("Behavior:"), m_crowdBehavior);
        form->addRow(tr("X:"), m_crowdX);
        form->addRow(tr("Y:"), m_crowdY);
        form->addRow(tr("Z:"), m_crowdZ);
        form->addRow(tr("Radius:"), m_crowdRadius);
        form->addRow(tr("Density:"), m_crowdDensity);
        form->addRow(tr("Max Members:"), m_crowdMax);
        tabLayout->addLayout(form);

        tabLayout->addWidget(new QLabel(tr("Members:"), tab));
        m_crowdMemberTable = makeTable({tr("Actor ID"), tr("Weight")});
        tabLayout->addWidget(m_crowdMemberTable);
        auto* addMember = toolButton(tr("Add Member"), tab, tabLayout);
        auto* removeMember = toolButton(tr("Remove Member"), tab, tabLayout);
        connect(addMember, &QPushButton::clicked, this, &StarfieldToolsDialog::addCrowdMember);
        connect(removeMember, &QPushButton::clicked, this, &StarfieldToolsDialog::removeCrowdMember);

        auto* loadBtn = toolButton(tr("Load Region JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Region JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadCrowd);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveCrowd);

        syncCrowdToWidgets();
        tabs->addTab(tab, tr("Crowd Region"));
    }

    // ---- Morph / face tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* form = new QFormLayout();
        m_morphId = new QLineEdit(tab);
        m_morphRace = new QLineEdit(tab);
        form->addRow(tr("Editor ID:"), m_morphId);
        form->addRow(tr("Race:"), m_morphRace);
        tabLayout->addLayout(form);

        tabLayout->addWidget(new QLabel(tr("Channels:"), tab));
        m_morphChannelTable = makeTable({tr("Name"), tr("Value"), tr("Min"), tr("Max")});
        tabLayout->addWidget(m_morphChannelTable);
        auto* addChannel = toolButton(tr("Add Channel"), tab, tabLayout);
        auto* removeChannel = toolButton(tr("Remove Channel"), tab, tabLayout);
        connect(addChannel, &QPushButton::clicked, this, &StarfieldToolsDialog::addMorphChannel);
        connect(removeChannel, &QPushButton::clicked, this, &StarfieldToolsDialog::removeMorphChannel);

        auto* loadBtn = toolButton(tr("Load Morph JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Morph JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadMorph);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveMorph);

        syncMorphToWidgets();
        tabs->addTab(tab, tr("Morph / Face"));
    }

    // ---- RoboVoicer tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* nameRow = new QHBoxLayout();
        nameRow->addWidget(new QLabel(tr("Plan:"), tab));
        m_voicePlanName = new QLineEdit(tab);
        nameRow->addWidget(m_voicePlanName);
        tabLayout->addLayout(nameRow);

        m_voiceLineTable = makeTable(
            {tr("Line ID"), tr("Speaker"), tr("Text"), tr("Voice"), tr("Output"), tr("Done")});
        tabLayout->addWidget(m_voiceLineTable);

        auto* lineBtns = new QHBoxLayout();
        auto* addLine = new QPushButton(tr("Add Line"), tab);
        auto* removeLine = new QPushButton(tr("Remove Line"), tab);
        auto* runBtn = new QPushButton(tr("Run (no engine)"), tab);
        runBtn->setToolTip(tr("Runs the plan with no speech engine attached; "
                              "pending lines are reported as failed so the counts are visible."));
        lineBtns->addWidget(addLine);
        lineBtns->addWidget(removeLine);
        lineBtns->addWidget(runBtn);
        lineBtns->addStretch();
        tabLayout->addLayout(lineBtns);
        connect(addLine, &QPushButton::clicked, this, &StarfieldToolsDialog::addVoiceLine);
        connect(removeLine, &QPushButton::clicked, this, &StarfieldToolsDialog::removeVoiceLine);
        connect(runBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::runVoicePlan);

        auto* loadBtn = toolButton(tr("Load Plan JSON..."), tab, tabLayout);
        auto* saveBtn = toolButton(tr("Save Plan JSON..."), tab, tabLayout);
        connect(loadBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::loadVoicePlan);
        connect(saveBtn, &QPushButton::clicked, this, &StarfieldToolsDialog::saveVoicePlan);

        syncVoiceToWidgets();
        tabs->addTab(tab, tr("RoboVoicer"));
    }

    // ---- Houdini tab ----
    {
        auto* tab = new QWidget();
        auto* tabLayout = new QVBoxLayout(tab);

        auto* form = new QFormLayout();
        m_houdiniExe = new QLineEdit(tab);
        m_houdiniExe->setPlaceholderText(tr("C:/.../hython.exe"));
        m_houdiniHip = new QLineEdit(tab);
        m_houdiniScript = new QLineEdit(tab);
        m_houdiniScript->setPlaceholderText(tr("export_ship.py"));
        m_houdiniExtra = new QLineEdit(tab);
        m_houdiniExtra->setPlaceholderText(tr("extra args, space separated"));
        form->addRow(tr("Executable:"), m_houdiniExe);
        form->addRow(tr(".hip File:"), m_houdiniHip);
        form->addRow(tr("Script:"), m_houdiniScript);
        form->addRow(tr("Extra Args:"), m_houdiniExtra);
        tabLayout->addLayout(form);

        tabLayout->addWidget(new QLabel(tr("Generated Command:"), tab));
        m_houdiniPreview = new QTextEdit(tab);
        m_houdiniPreview->setReadOnly(true);
        m_houdiniPreview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        tabLayout->addWidget(m_houdiniPreview);

        connect(m_houdiniExe, &QLineEdit::textChanged, this, &StarfieldToolsDialog::refreshHoudiniCommand);
        connect(m_houdiniHip, &QLineEdit::textChanged, this, &StarfieldToolsDialog::refreshHoudiniCommand);
        connect(m_houdiniScript, &QLineEdit::textChanged, this, &StarfieldToolsDialog::refreshHoudiniCommand);
        connect(m_houdiniExtra, &QLineEdit::textChanged, this, &StarfieldToolsDialog::refreshHoudiniCommand);

        refreshHoudiniCommand();
        tabs->addTab(tab, tr("Houdini"));
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
