#include "planeteditordialog.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
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
#include <QTableWidget>
#include <QVBoxLayout>

PlanetEditorDialog::PlanetEditorDialog(QWidget* parent)
    : QDialog(parent)
{
    LOG_INFO("PlanetEditorDialog: opening planet editor");

    setWindowTitle(tr("Planet Editor"));
    resize(680, 640);

    auto* mainLayout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    m_editorId = new QLineEdit(this);
    m_starSystem = new QLineEdit(this);
    m_dayLength = new QDoubleSpinBox(this);
    m_dayLength->setRange(0.1, 10000.0);
    m_dayLength->setDecimals(2);
    m_dayLength->setSuffix(tr(" hours"));
    m_gravity = new QLineEdit(this);
    m_gravity->setPlaceholderText(tr("e.g. 0.5G"));
    m_temperature = new QLineEdit(this);
    m_temperature->setPlaceholderText(tr("e.g. Temperate"));
    form->addRow(tr("Editor ID:"), m_editorId);
    form->addRow(tr("Star System:"), m_starSystem);
    form->addRow(tr("Day Length:"), m_dayLength);
    form->addRow(tr("Gravity:"), m_gravity);
    form->addRow(tr("Temperature:"), m_temperature);
    mainLayout->addLayout(form);

    // Biomes
    auto* biomeBox = new QGroupBox(tr("Biomes"), this);
    auto* biomeLayout = new QVBoxLayout(biomeBox);
    m_biomeTable = new QTableWidget(0, 3, biomeBox);
    m_biomeTable->setHorizontalHeaderLabels(
        {tr("Name"), tr("Color (hex)"), tr("Coverage")});
    m_biomeTable->horizontalHeader()->setStretchLastSection(true);
    biomeLayout->addWidget(m_biomeTable);
    auto* biomeButtons = new QHBoxLayout();
    auto* addBiomeBtn = new QPushButton(tr("Add Biome"), biomeBox);
    auto* removeBiomeBtn = new QPushButton(tr("Remove Biome"), biomeBox);
    biomeButtons->addWidget(addBiomeBtn);
    biomeButtons->addWidget(removeBiomeBtn);
    biomeButtons->addStretch();
    biomeLayout->addLayout(biomeButtons);
    mainLayout->addWidget(biomeBox);

    // Traits
    auto* traitBox = new QGroupBox(tr("Traits"), this);
    auto* traitLayout = new QVBoxLayout(traitBox);
    m_traitList = new QListWidget(traitBox);
    traitLayout->addWidget(m_traitList);
    mainLayout->addWidget(traitBox);

    // Resources
    auto* resBox = new QGroupBox(tr("Resources"), this);
    auto* resLayout = new QVBoxLayout(resBox);
    m_resourceTable = new QTableWidget(0, 2, resBox);
    m_resourceTable->setHorizontalHeaderLabels({tr("Name"), tr("Count")});
    m_resourceTable->horizontalHeader()->setStretchLastSection(true);
    resLayout->addWidget(m_resourceTable);
    auto* resButtons = new QHBoxLayout();
    auto* addResBtn = new QPushButton(tr("Add Resource"), resBox);
    auto* removeResBtn = new QPushButton(tr("Remove Resource"), resBox);
    resButtons->addWidget(addResBtn);
    resButtons->addWidget(removeResBtn);
    resButtons->addStretch();
    resLayout->addLayout(resButtons);
    mainLayout->addWidget(resBox);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* loadBtn = buttons->addButton(tr("Load JSON..."), QDialogButtonBox::ActionRole);
    auto* saveBtn = buttons->addButton(tr("Save JSON..."), QDialogButtonBox::ActionRole);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        syncFromWidgets();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addBiomeBtn, &QPushButton::clicked, this, &PlanetEditorDialog::addBiome);
    connect(removeBiomeBtn, &QPushButton::clicked, this, &PlanetEditorDialog::removeBiome);
    connect(addResBtn, &QPushButton::clicked, this, &PlanetEditorDialog::addResource);
    connect(removeResBtn, &QPushButton::clicked, this, &PlanetEditorDialog::removeResource);
    connect(loadBtn, &QPushButton::clicked, this, &PlanetEditorDialog::loadFromJson);
    connect(saveBtn, &QPushButton::clicked, this, &PlanetEditorDialog::saveToJson);

    syncToWidgets();
}

void PlanetEditorDialog::syncToWidgets()
{
    m_editorId->setText(m_def.editorId);
    m_starSystem->setText(m_def.starSystem);
    m_dayLength->setValue(m_def.dayLengthHours > 0.0 ? m_def.dayLengthHours : 24.0);
    m_gravity->setText(m_def.gravity);
    m_temperature->setText(m_def.temperature);

    m_biomeTable->setRowCount(m_def.biomes.size());
    for (int i = 0; i < m_def.biomes.size(); ++i)
    {
        m_biomeTable->setItem(i, 0, new QTableWidgetItem(m_def.biomes[i].name));
        m_biomeTable->setItem(i, 1, new QTableWidgetItem(m_def.biomes[i].colorHex));
        m_biomeTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(m_def.biomes[i].coverage, 'f', 3)));
    }

    m_traitList->clear();
    const QStringList common = PlanetDefinition::commonTraits();
    for (const QString& t : common)
    {
        auto* item = new QListWidgetItem(t, m_traitList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_def.traits.contains(t) ? Qt::Checked : Qt::Unchecked);
    }
    // Preserve custom traits not in the common list.
    for (const QString& t : m_def.traits)
    {
        if (!common.contains(t))
        {
            auto* item = new QListWidgetItem(t, m_traitList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
    }

    m_resourceTable->setRowCount(m_def.resources.size());
    for (int i = 0; i < m_def.resources.size(); ++i)
    {
        m_resourceTable->setItem(i, 0, new QTableWidgetItem(m_def.resources[i].name));
        m_resourceTable->setItem(i, 1, new QTableWidgetItem(
            QString::number(m_def.resources[i].count)));
    }
}

void PlanetEditorDialog::syncFromWidgets()
{
    m_def.editorId = m_editorId->text().trimmed();
    m_def.starSystem = m_starSystem->text().trimmed();
    m_def.dayLengthHours = m_dayLength->value();
    m_def.gravity = m_gravity->text().trimmed();
    m_def.temperature = m_temperature->text().trimmed();

    m_def.biomes.clear();
    for (int i = 0; i < m_biomeTable->rowCount(); ++i)
    {
        PlanetDefinition::Biome b;
        if (auto* item = m_biomeTable->item(i, 0)) b.name = item->text().trimmed();
        if (auto* item = m_biomeTable->item(i, 1)) b.colorHex = item->text().trimmed();
        if (auto* item = m_biomeTable->item(i, 2)) b.coverage = item->text().toDouble();
        if (!b.name.isEmpty())
            m_def.biomes.append(b);
    }

    m_def.traits.clear();
    for (int i = 0; i < m_traitList->count(); ++i)
    {
        QListWidgetItem* item = m_traitList->item(i);
        if (item->checkState() == Qt::Checked)
            m_def.traits.append(item->text());
    }

    m_def.resources.clear();
    for (int i = 0; i < m_resourceTable->rowCount(); ++i)
    {
        PlanetDefinition::Resource r;
        if (auto* item = m_resourceTable->item(i, 0)) r.name = item->text().trimmed();
        if (auto* item = m_resourceTable->item(i, 1)) r.count = item->text().toInt();
        if (!r.name.isEmpty())
            m_def.resources.append(r);
    }
}

void PlanetEditorDialog::setDefinition(const PlanetDefinition& def)
{
    m_def = def;
    syncToWidgets();
}

PlanetDefinition PlanetEditorDialog::definition() const
{
    return m_def;
}

void PlanetEditorDialog::addBiome()
{
    const int row = m_biomeTable->rowCount();
    m_biomeTable->insertRow(row);
    m_biomeTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("New Biome")));
    m_biomeTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("#808080")));
    m_biomeTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("0.000")));
}

void PlanetEditorDialog::removeBiome()
{
    const int row = m_biomeTable->currentRow();
    if (row >= 0)
        m_biomeTable->removeRow(row);
}

void PlanetEditorDialog::addResource()
{
    const int row = m_resourceTable->rowCount();
    m_resourceTable->insertRow(row);
    m_resourceTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("New Resource")));
    m_resourceTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("0")));
}

void PlanetEditorDialog::removeResource()
{
    const int row = m_resourceTable->currentRow();
    if (row >= 0)
        m_resourceTable->removeRow(row);
}

void PlanetEditorDialog::loadFromJson()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Planet Definition"), QString(),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Load Failed"),
            tr("Could not open %1").arg(path));
        return;
    }

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        QMessageBox::warning(this, tr("Load Failed"),
            tr("Invalid JSON: %1").arg(err.errorString()));
        return;
    }

    setDefinition(PlanetDefinition::fromJson(doc.object()));
    LOG_INFO(QString("PlanetEditorDialog: loaded definition from %1").arg(path));
}

void PlanetEditorDialog::saveToJson()
{
    syncFromWidgets();

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Planet Definition"),
        m_def.editorId.isEmpty() ? QStringLiteral("planet.json")
                                 : m_def.editorId + QStringLiteral(".json"),
        tr("JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Could not write %1").arg(path));
        return;
    }

    file.write(QJsonDocument(m_def.toJson()).toJson(QJsonDocument::Indented));
    file.close();
    LOG_INFO(QString("PlanetEditorDialog: saved definition to %1").arg(path));
}
