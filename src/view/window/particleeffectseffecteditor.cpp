#include "particleeffectseffecteditor.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>

#include "../../libs/files/nif/particle/particleeffects.hpp"
#include "colorgradientwidget.hpp"
#include "sizecurvewidget.hpp"
#include "logger.hpp"

ParticleEffectsEditor::ParticleEffectsEditor(QWidget* parent)
    : QDialog(parent)
    , mSelectedIndex(-1)
{
    setWindowTitle(tr("Particle Effects Editor"));
    setMinimumSize(700, 800);

    auto* mainLayout = new QVBoxLayout(this);

    auto* nifGroup = new QGroupBox(tr("NIF Particle Source"));
    auto* nifLayout = new QHBoxLayout(nifGroup);

    auto* browseBtn = new QPushButton(tr("Open NIF with Particles..."));
    nifLayout->addWidget(browseBtn);
    mainLayout->addWidget(nifGroup);

    connect(browseBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::browseNif);

    auto* sysGroup = new QGroupBox(tr("Particle Systems"));
    auto* sysLayout = new QVBoxLayout(sysGroup);

    mSystemList = new QListWidget();
    sysLayout->addWidget(mSystemList);
    mainLayout->addWidget(sysGroup);

    connect(mSystemList, &QListWidget::currentRowChanged,
            this, &ParticleEffectsEditor::onParticleSystemSelected);

    auto* emitGroup = new QGroupBox(tr("Emitter Properties"));
    auto* emitLayout = new QVBoxLayout(emitGroup);

    mEmitterTable = new QTableWidget(0, 3, this);
    mEmitterTable->setHorizontalHeaderLabels({tr("Property"), tr("Value"), tr("Type")});
    mEmitterTable->horizontalHeader()->setStretchLastSection(true);
    emitLayout->addWidget(mEmitterTable);

    mApplyEmitterBtn = new QPushButton(tr("Apply Emitter Changes"));
    emitLayout->addWidget(mApplyEmitterBtn);

    mainLayout->addWidget(emitGroup);

    connect(mApplyEmitterBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::applyEmitterChanges);

    auto* colorGroup = new QGroupBox(tr("Color Over Lifetime"));
    auto* colorLayout = new QVBoxLayout(colorGroup);

    mColorGradient = new ColorGradientWidget(this);
    colorLayout->addWidget(mColorGradient);

    mApplyColorBtn = new QPushButton(tr("Apply Color Gradient"));
    colorLayout->addWidget(mApplyColorBtn);

    mainLayout->addWidget(colorGroup);

    connect(mApplyColorBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::applyColorGradient);

    auto* sizeGroup = new QGroupBox(tr("Size Over Lifetime"));
    auto* sizeLayout = new QVBoxLayout(sizeGroup);

    mSizeCurve = new SizeCurveWidget(this);
    sizeLayout->addWidget(mSizeCurve);

    mApplySizeBtn = new QPushButton(tr("Apply Size Curve"));
    sizeLayout->addWidget(mApplySizeBtn);

    mainLayout->addWidget(sizeGroup);

    connect(mApplySizeBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::applySizeCurve);

    auto* renderGroup = new QGroupBox(tr("Renderer Properties"));
    auto* renderLayout = new QVBoxLayout(renderGroup);

    mRendererTable = new QTableWidget(0, 3, this);
    mRendererTable->setHorizontalHeaderLabels({tr("Property"), tr("Value"), tr("Type")});
    mRendererTable->horizontalHeader()->setStretchLastSection(true);
    renderLayout->addWidget(mRendererTable);

    mApplyRendererBtn = new QPushButton(tr("Apply Renderer Changes"));
    renderLayout->addWidget(mApplyRendererBtn);

    mainLayout->addWidget(renderGroup);

    connect(mApplyRendererBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::applyRendererChanges);

    auto* exportLayout = new QHBoxLayout();
    auto* exportBtn = new QPushButton(tr("Export Particle Data..."));
    exportLayout->addStretch();
    exportLayout->addWidget(exportBtn);
    mainLayout->addLayout(exportLayout);

    connect(exportBtn, &QPushButton::clicked, this, &ParticleEffectsEditor::exportParticles);

    loadEmitterDefaults();
    loadRendererDefaults();
}

void ParticleEffectsEditor::browseNif()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open NIF"), "", tr("NIF Files (*.nif)"));
    if (!path.isEmpty()) {
        loadParticles(path);
    }
}

void ParticleEffectsEditor::loadParticles(const QString& path)
{
    qDeleteAll(mSystems);
    mSystems.clear();
    mSystemList->clear();
    mSelectedIndex = -1;

    mSystems = ParticleEffectsParser::parse(path);

    if (!mSystems.isEmpty()) {
        setWindowTitle(tr("Particle Effects Editor - %1").arg(QFileInfo(path).baseName()));

        for (int i = 0; i < mSystems.size(); ++i) {
            mSystemList->addItem(mSystems[i]->name);
        }

        if (!mSystems.isEmpty()) {
            mSystemList->setCurrentRow(0);
            onParticleSystemSelected(0);
        }

        LOG_INFO(QString("Loaded %1 particle systems from NIF").arg(mSystems.size()));
    } else {
        QMessageBox::information(this, tr("No Particles"),
                                 tr("No particle system data found in this NIF file."));
    }
}

void ParticleEffectsEditor::onParticleSystemSelected(int index)
{
    if (index < 0 || index >= mSystems.size()) return;
    mSelectedIndex = index;
    updateEmitterTable();
    updateRendererTable();
    updateColorGradient();
    updateSizeCurve();
    LOG_INFO(QString("Selected particle system: %1").arg(mSystems[index]->name));
}

void ParticleEffectsEditor::updateEmitterTable()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    mEmitterTable->setRowCount(21);

    QString props[] = {
        tr("Emission Rate"), tr("Lifetime"), tr("Min Speed"), tr("Max Speed"),
        tr("X Velocity"), tr("Y Velocity"), tr("Z Velocity"),
        tr("X Spread"), tr("Y Spread"), tr("Z Spread"),
        tr("Min Radius"), tr("Max Radius"), tr("Min Angle"), tr("Max Angle"),
        tr("Start Size"), tr("Start Size Random"), tr("End Size"), tr("End Size Random"),
        tr("Initial Rotation"), tr("Rotation Speed"), tr("Max Particles")
    };

    QString vals[] = {
        QString::number(sys->emissionRate, 'f', 2),
        QString::number(sys->lifetime, 'f', 2),
        QString::number(sys->minSpeed, 'f', 2),
        QString::number(sys->maxSpeed, 'f', 2),
        QString::number(sys->xVelocity, 'f', 2),
        QString::number(sys->yVelocity, 'f', 2),
        QString::number(sys->zVelocity, 'f', 2),
        QString::number(sys->xSpread, 'f', 2),
        QString::number(sys->ySpread, 'f', 2),
        QString::number(sys->zSpread, 'f', 2),
        QString::number(sys->minRadius, 'f', 2),
        QString::number(sys->maxRadius, 'f', 2),
        QString::number(sys->minAngle, 'f', 2),
        QString::number(sys->maxAngle, 'f', 2),
        QString::number(sys->startSize, 'f', 2),
        QString::number(sys->startSizeRandom, 'f', 2),
        QString::number(sys->endSize, 'f', 2),
        QString::number(sys->endSizeRandom, 'f', 2),
        QString::number(sys->initialRotation, 'f', 2),
        QString::number(sys->rotationSpeed, 'f', 2),
        QString::number(sys->maxParticles)
    };

    for (int i = 0; i < 21; ++i) {
        mEmitterTable->setItem(i, 0, new QTableWidgetItem(props[i]));
        mEmitterTable->setItem(i, 1, new QTableWidgetItem(vals[i]));
        mEmitterTable->setItem(i, 2, new QTableWidgetItem(i == 20 ? tr("int") : tr("float")));
    }
}

void ParticleEffectsEditor::updateRendererTable()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    mRendererTable->setRowCount(6);

    QString props[] = {
        tr("Texture File"), tr("Columns"), tr("Rows"),
        tr("Type"), tr("Additive Blending"), tr("Alpha Test")
    };

    QString vals[] = {
        sys->textureFile,
        QString::number(sys->columns),
        QString::number(sys->numRows),
        QString::number(static_cast<int>(sys->rendererType)),
        sys->additiveBlending ? tr("Yes") : tr("No"),
        sys->alphaTest ? tr("Yes") : tr("No")
    };

    for (int i = 0; i < 6; ++i) {
        mRendererTable->setItem(i, 0, new QTableWidgetItem(props[i]));
        mRendererTable->setItem(i, 1, new QTableWidgetItem(vals[i]));
        mRendererTable->setItem(i, 2, new QTableWidgetItem(tr("string")));
    }
}

void ParticleEffectsEditor::updateColorGradient()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    QVector<ColorGradientWidget::ColorStop> stops;
    if (sys->colorOverLifetime.isEmpty()) {
        stops.append({0.0f, QColor(255, 255, 255)});
        stops.append({1.0f, QColor(255, 255, 255)});
    } else {
        for (const auto& pair : sys->colorOverLifetime) {
            stops.append({pair.first, pair.second});
        }
    }
    mColorGradient->setGradient(stops);
}

void ParticleEffectsEditor::updateSizeCurve()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    QVector<QPointF> pts = sys->sizeOverLifetime;
    if (pts.isEmpty()) {
        pts.append(QPointF(0.0f, 1.0f));
    }
    mSizeCurve->setCurve(pts);
}

void ParticleEffectsEditor::loadEmitterDefaults() {
    mEmitterTable->setRowCount(21);

    QString props[] = {
        tr("Emission Rate"), tr("Lifetime"), tr("Min Speed"), tr("Max Speed"),
        tr("X Velocity"), tr("Y Velocity"), tr("Z Velocity"),
        tr("X Spread"), tr("Y Spread"), tr("Z Spread"),
        tr("Min Radius"), tr("Max Radius"), tr("Min Angle"), tr("Max Angle"),
        tr("Start Size"), tr("Start Size Random"), tr("End Size"), tr("End Size Random"),
        tr("Initial Rotation"), tr("Rotation Speed"), tr("Max Particles")
    };

    QString vals[] = {
        "10.00", "5.00", "1.00", "5.00",
        "0.00", "0.00", "0.00",
        "0.00", "0.00", "0.00",
        "0.00", "0.00", "0.00", "0.00",
        "1.00", "0.00", "0.00", "0.00",
        "0.00", "0.00", "100"
    };

    for (int i = 0; i < 21; ++i) {
        mEmitterTable->setItem(i, 0, new QTableWidgetItem(props[i]));
        mEmitterTable->setItem(i, 1, new QTableWidgetItem(vals[i]));
        mEmitterTable->setItem(i, 2, new QTableWidgetItem(i == 20 ? tr("int") : tr("float")));
    }
}

void ParticleEffectsEditor::loadRendererDefaults() {
    mRendererTable->setRowCount(6);

    QString props[] = {
        tr("Texture File"), tr("Columns"), tr("Rows"),
        tr("Type"), tr("Additive Blending"), tr("Alpha Test")
    };

    QString vals[] = {
        "", "1", "1", "0", "No", "No"
    };

    for (int i = 0; i < 6; ++i) {
        mRendererTable->setItem(i, 0, new QTableWidgetItem(props[i]));
        mRendererTable->setItem(i, 1, new QTableWidgetItem(vals[i]));
        mRendererTable->setItem(i, 2, new QTableWidgetItem(tr("string")));
    }
}

void ParticleEffectsEditor::applyEmitterChanges()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    for (int row = 0; row < mEmitterTable->rowCount(); ++row) {
        const QString& prop = mEmitterTable->item(row, 0)->text();
        const QString& val = mEmitterTable->item(row, 1)->text();
        bool ok = false;

        if (prop == tr("Max Particles")) {
            int ival = val.toInt(&ok);
            if (ok) sys->maxParticles = ival;
            continue;
        }

        float fval = val.toFloat(&ok);
        if (!ok) continue;

        if (prop == tr("Emission Rate")) sys->emissionRate = fval;
        else if (prop == tr("Lifetime")) sys->lifetime = fval;
        else if (prop == tr("Min Speed")) sys->minSpeed = fval;
        else if (prop == tr("Max Speed")) sys->maxSpeed = fval;
        else if (prop == tr("X Velocity")) sys->xVelocity = fval;
        else if (prop == tr("Y Velocity")) sys->yVelocity = fval;
        else if (prop == tr("Z Velocity")) sys->zVelocity = fval;
        else if (prop == tr("X Spread")) sys->xSpread = fval;
        else if (prop == tr("Y Spread")) sys->ySpread = fval;
        else if (prop == tr("Z Spread")) sys->zSpread = fval;
        else if (prop == tr("Min Radius")) sys->minRadius = fval;
        else if (prop == tr("Max Radius")) sys->maxRadius = fval;
        else if (prop == tr("Min Angle")) sys->minAngle = fval;
        else if (prop == tr("Max Angle")) sys->maxAngle = fval;
        else if (prop == tr("Start Size")) sys->startSize = fval;
        else if (prop == tr("Start Size Random")) sys->startSizeRandom = fval;
        else if (prop == tr("End Size")) sys->endSize = fval;
        else if (prop == tr("End Size Random")) sys->endSizeRandom = fval;
        else if (prop == tr("Initial Rotation")) sys->initialRotation = fval;
        else if (prop == tr("Rotation Speed")) sys->rotationSpeed = fval;
    }

    LOG_INFO(QString("Emitter changes applied to: %1").arg(sys->name));
    emit particleSystemUpdated(sys);
    QMessageBox::information(this, tr("Applied"), tr("Emitter properties updated."));
}

void ParticleEffectsEditor::applyRendererChanges()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    for (int row = 0; row < mRendererTable->rowCount(); ++row) {
        const QString& prop = mRendererTable->item(row, 0)->text();
        const QString& val = mRendererTable->item(row, 1)->text();

        if (prop == tr("Texture File")) sys->textureFile = val;
        else if (prop == tr("Columns")) sys->columns = val.toInt();
        else if (prop == tr("Rows")) sys->numRows = val.toInt();
        else if (prop == tr("Type")) sys->rendererType = static_cast<RendererType>(val.toInt());
        else if (prop == tr("Additive Blending")) sys->additiveBlending = (val == tr("Yes"));
        else if (prop == tr("Alpha Test")) sys->alphaTest = (val == tr("Yes"));
    }

    LOG_INFO(QString("Renderer changes applied to: %1").arg(sys->name));
    emit particleSystemUpdated(sys);
    QMessageBox::information(this, tr("Applied"), tr("Renderer properties updated."));
}

void ParticleEffectsEditor::applyColorGradient()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    QVector<ColorGradientWidget::ColorStop> stops = mColorGradient->gradient();
    sys->colorOverLifetime.clear();
    for (const auto& s : stops) {
        sys->colorOverLifetime.append(qMakePair(s.position, s.color));
    }

    LOG_INFO(QString("Color gradient applied to: %1").arg(sys->name));
    QMessageBox::information(this, tr("Applied"), tr("Color gradient updated."));
}

void ParticleEffectsEditor::applySizeCurve()
{
    if (mSelectedIndex < 0 || mSelectedIndex >= mSystems.size()) return;
    ParticleSystemData* sys = mSystems[mSelectedIndex];

    sys->sizeOverLifetime = mSizeCurve->curve();

    LOG_INFO(QString("Size curve applied to: %1").arg(sys->name));
    QMessageBox::information(this, tr("Applied"), tr("Size curve updated."));
}

void ParticleEffectsEditor::exportParticles()
{
    if (mSystems.isEmpty()) {
        QMessageBox::information(this, tr("Export Failed"), tr("No particle systems loaded."));
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, tr("Export Particle Data"),
        "particles.json", tr("JSON Files (*.json);;CSV Files (*.csv)"));
    if (savePath.isEmpty()) return;

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Cannot write to: %1").arg(savePath));
        return;
    }

    QString json = "{\n  \"count\": " + QString::number(mSystems.size()) + ",\n  \"systems\": [\n";

    for (int i = 0; i < mSystems.size(); ++i) {
        ParticleSystemData* sys = mSystems[i];
        json += "    {\n";
        json += "      \"name\": \"" + sys->name + "\",\n";
        json += "      \"emissionRate\": " + QString::number(sys->emissionRate, 'f', 2) + ",\n";
        json += "      \"lifetime\": " + QString::number(sys->lifetime, 'f', 2) + ",\n";
        json += "      \"textureFile\": \"" + sys->textureFile + "\",\n";
        json += "      \"columns\": " + QString::number(sys->columns) + ",\n";
        json += "      \"numRows\": " + QString::number(sys->numRows) + ",\n";
        json += "      \"maxParticles\": " + QString::number(sys->maxParticles) + ",\n";
        json += "      \"startSize\": " + QString::number(sys->startSize, 'f', 2) + ",\n";
        json += "      \"endSize\": " + QString::number(sys->endSize, 'f', 2) + "\n";
        json += (i < mSystems.size() - 1) ? "    },\n" : "    }\n";
    }

    json += "  ]\n}\n";

    file.write(json.toUtf8());
    file.close();

    LOG_INFO(QString("Particle data exported: %1 (%2 systems)").arg(savePath).arg(mSystems.size()));
    QMessageBox::information(this, tr("Export Complete"), tr("Particle data exported to:\n%1").arg(savePath));
}
