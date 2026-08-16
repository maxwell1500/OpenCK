#include "worldspacedatawidget.hpp"

#include "../libs/files/esm/worldspacerecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../../model/world/data.hpp"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLabel>

WorldspaceDataWidget::WorldspaceDataWidget(void* recordPtr,
                                           openck::FormComponents*,
                                           Data* data,
                                           QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
    , m_data(data)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Worldspace Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* nameEdit = new QLineEdit(infoGroup);
    nameEdit->setObjectName(QStringLiteral("name"));
    nameEdit->setPlaceholderText(QStringLiteral("Name"));

    auto* waterTypeSpin = new QSpinBox(infoGroup);
    waterTypeSpin->setRange(0, INT_MAX);
    auto* climateSpin = new QSpinBox(infoGroup);
    climateSpin->setRange(0, INT_MAX);
    auto* lightingSpin = new QSpinBox(infoGroup);
    lightingSpin->setRange(0, INT_MAX);

    auto* mapWidthSpin = new QSpinBox(infoGroup);
    mapWidthSpin->setRange(0, 8192);
    auto* mapHeightSpin = new QSpinBox(infoGroup);
    mapHeightSpin->setRange(0, 8192);

    auto* nwXSpin = new QSpinBox(infoGroup);
    nwXSpin->setRange(-32768, 32767);
    auto* nwYSpin = new QSpinBox(infoGroup);
    nwYSpin->setRange(-32768, 32767);
    auto* seXSpin = new QSpinBox(infoGroup);
    seXSpin->setRange(-32768, 32767);
    auto* seYSpin = new QSpinBox(infoGroup);
    seYSpin->setRange(-32768, 32767);

    auto* scaleSpin = new QDoubleSpinBox(infoGroup);
    scaleSpin->setRange(0.01, 100.0);
    scaleSpin->setDecimals(2);
    scaleSpin->setSingleStep(0.1);
    auto* lodBiasSpin = new QDoubleSpinBox(infoGroup);
    lodBiasSpin->setRange(0.0, 10.0);
    lodBiasSpin->setDecimals(3);
    lodBiasSpin->setSingleStep(0.05);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Name:"), nameEdit);
    infoForm->addRow(QStringLiteral("Water Type (FormID):"), waterTypeSpin);
    infoForm->addRow(QStringLiteral("Climate (FormID):"), climateSpin);
    infoForm->addRow(QStringLiteral("Lighting (FormID):"), lightingSpin);
    infoForm->addRow(QStringLiteral("Map Width:"), mapWidthSpin);
    infoForm->addRow(QStringLiteral("Map Height:"), mapHeightSpin);
    infoForm->addRow(QStringLiteral("Map NW Cell X:"), nwXSpin);
    infoForm->addRow(QStringLiteral("Map NW Cell Y:"), nwYSpin);
    infoForm->addRow(QStringLiteral("Map SE Cell X:"), seXSpin);
    infoForm->addRow(QStringLiteral("Map SE Cell Y:"), seYSpin);
    infoForm->addRow(QStringLiteral("Map Scale:"), scaleSpin);
    infoForm->addRow(QStringLiteral("LOD Bias:"), lodBiasSpin);
    mainLayout->addWidget(infoGroup);

    // Raw flags / bounds, shown read-only as hex (DATA and DNAM vary by game).
    auto* rawGroup = new QGroupBox(QStringLiteral("Raw Data"), this);
    auto* rawForm = new QFormLayout(rawGroup);
    auto* dataLabel = new QLabel(rawGroup);
    dataLabel->setObjectName(QStringLiteral("dataFlags"));
    dataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* dnamLabel = new QLabel(rawGroup);
    dnamLabel->setObjectName(QStringLiteral("dnamData"));
    dnamLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rawForm->addRow(QStringLiteral("DATA:"), dataLabel);
    rawForm->addRow(QStringLiteral("DNAM:"), dnamLabel);
    mainLayout->addWidget(rawGroup);

    // Cell grid summary derived from the map dimensions and NW/SE cell coords.
    auto* gridGroup = new QGroupBox(QStringLiteral("Cell Grid"), this);
    auto* gridLayout = new QVBoxLayout(gridGroup);
    auto* gridLabel = new QLabel(gridGroup);
    gridLabel->setObjectName(QStringLiteral("cellGrid"));
    gridLabel->setWordWrap(true);
    gridLayout->addWidget(gridLabel);
    mainLayout->addWidget(gridGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<WorldspaceRecord*>(m_recordPtr);

        auto setSpin = [](QSpinBox* s, int v) {
            s->blockSignals(true); s->setValue(v); s->blockSignals(false);
        };
        auto setDSpin = [](QDoubleSpinBox* s, double v) {
            s->blockSignals(true); s->setValue(v); s->blockSignals(false);
        };

        editorIdEdit->setText(rec->editorId);
        nameEdit->setText(rec->name);
        setSpin(waterTypeSpin, static_cast<int>(rec->waterType));
        setSpin(climateSpin, static_cast<int>(rec->climateId));
        setSpin(lightingSpin, static_cast<int>(rec->lightingId));
        setSpin(mapWidthSpin, static_cast<int>(rec->mapWidth));
        setSpin(mapHeightSpin, static_cast<int>(rec->mapHeight));
        setSpin(nwXSpin, rec->mapNwX);
        setSpin(nwYSpin, rec->mapNwY);
        setSpin(seXSpin, rec->mapSeX);
        setSpin(seYSpin, rec->mapSeY);
        setDSpin(scaleSpin, rec->mapScale());
        setDSpin(lodBiasSpin, rec->mapLodBias);

        const int minX = qMin(rec->mapNwX, rec->mapSeX);
        const int minY = qMin(rec->mapNwY, rec->mapSeY);
        const int maxX = qMax(rec->mapNwX, rec->mapSeX);
        const int maxY = qMax(rec->mapNwY, rec->mapSeY);
        const int cellW = rec->mapWidth > 0 ? maxX - minX + 1 : 0;
        const int cellH = rec->mapHeight > 0 ? maxY - minY + 1 : 0;
                const int storedCells = m_data
            ? static_cast<int>(m_data->cellsInWorldspace(rec->formId).size())
            : static_cast<int>(rec->cellIds.size());
        gridLabel->setText(QStringLiteral(
            "Map size: %1 x %2 cells\n"
            "Cell range: (%3, %4) .. (%5, %6)\n"
            "Stored cells: %7")
            .arg(rec->mapWidth).arg(rec->mapHeight)
            .arg(minX).arg(minY).arg(maxX).arg(maxY)
            .arg(storedCells)
            + (cellW == 0 || cellH == 0 ? QString()
                : QStringLiteral("\nExtent: %1 x %2 cells").arg(cellW).arg(cellH)));

        dataLabel->setText(rec->dataFlags.isEmpty()
            ? QStringLiteral("(none)") : QString(rec->dataFlags.toHex(' ')));
        dnamLabel->setText(rec->dnamData.isEmpty()
            ? QStringLiteral("(none)") : QString(rec->dnamData.toHex(' ')));

        connect(editorIdEdit, &QLineEdit::textChanged, this,
            [rec](const QString& t) { rec->editorId = t; });
        connect(nameEdit, &QLineEdit::textChanged, this,
            [rec](const QString& t) { rec->name = t; });
        connect(waterTypeSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->waterType = static_cast<quint32>(v); });
        connect(climateSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->climateId = static_cast<quint32>(v); });
        connect(lightingSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->lightingId = static_cast<quint32>(v); });
        connect(mapWidthSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec, gridLabel](int v) {
                rec->mapWidth = static_cast<quint32>(v);
                rec->mapSize = rec->mapWidth;
                gridLabel->setText(QStringLiteral("Map size: %1 x %2 cells")
                    .arg(rec->mapWidth).arg(rec->mapHeight));
            });
        connect(mapHeightSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec, gridLabel](int v) {
                rec->mapHeight = static_cast<quint32>(v);
                gridLabel->setText(QStringLiteral("Map size: %1 x %2 cells")
                    .arg(rec->mapWidth).arg(rec->mapHeight));
            });
        connect(nwXSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->mapNwX = v; rec->dataMinX = v; });
        connect(nwYSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->mapNwY = v; rec->dataMinY = v; });
        connect(seXSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->mapSeX = v; });
        connect(seYSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [rec](int v) { rec->mapSeY = v; });
        connect(scaleSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [rec](double v) { rec->setMapScale(static_cast<float>(v)); });
        connect(lodBiasSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [rec](double v) { rec->mapLodBias = static_cast<float>(v); });
    }
}

WorldspaceDataWidget::~WorldspaceDataWidget() = default;
