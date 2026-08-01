#include "worldspacedatawidget.hpp"

#include "../libs/files/esm/worldspacerecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLabel>

WorldspaceDataWidget::WorldspaceDataWidget(void* recordPtr,
                                           openck::FormComponents*,
                                           QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
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
    auto* musicSpin = new QSpinBox(infoGroup);
    musicSpin->setRange(0, INT_MAX);
    auto* terrainSpin = new QSpinBox(infoGroup);
    terrainSpin->setRange(0, INT_MAX);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Name:"), nameEdit);
    infoForm->addRow(QStringLiteral("Water Type:"), waterTypeSpin);
    infoForm->addRow(QStringLiteral("Climate:"), climateSpin);
    infoForm->addRow(QStringLiteral("Lighting:"), lightingSpin);
    infoForm->addRow(QStringLiteral("Music:"), musicSpin);
    infoForm->addRow(QStringLiteral("Terrain:"), terrainSpin);
    mainLayout->addWidget(infoGroup);

    // Cell grid summary: the worldspace spans a grid of interior/exterior
    // cells bounded by (dataMinX, dataMinY) .. (dataMinX + mapSize - 1,
    // dataMinY + mapSize - 1).
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
        editorIdEdit->setText(rec->editorId);
        nameEdit->setText(rec->name);
        waterTypeSpin->setValue(static_cast<int>(rec->waterType));
        climateSpin->setValue(static_cast<int>(rec->climateId));
        lightingSpin->setValue(static_cast<int>(rec->lightingId));
        musicSpin->setValue(static_cast<int>(rec->music));
        terrainSpin->setValue(static_cast<int>(rec->terrain));

        const int size = static_cast<int>(rec->mapSize);
        const int minX = rec->dataMinX;
        const int minY = rec->dataMinY;
        const int maxX = minX + size - 1;
        const int maxY = minY + size - 1;
        gridLabel->setText(QStringLiteral(
            "Map size: %1 x %1\n"
            "Cell range: (%2, %3) .. (%4, %5)\n"
            "Stored cells: %6\n"
            "Cell coordinates use the exterior cell grid origin at (0,0).")
            .arg(size).arg(minX).arg(minY).arg(maxX).arg(maxY)
            .arg(rec->cellIds.size()));
    }
}

WorldspaceDataWidget::~WorldspaceDataWidget() = default;
