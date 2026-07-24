#include "mesheditordialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <algorithm>
#include <QComboBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QLineEdit>

#include "../../libs/files/nif/nifparser.hpp"
#include "../../libs/files/nif/ddsencoder.hpp"
#include "nifviewportwidget.hpp"
#include "textureeditordialog.hpp"
#include "logger.hpp"

MeshEditorDialog::MeshEditorDialog(Nif::NifParser* parser,
                                   NifViewportWidget* viewport,
                                   QWidget* parent)
    : QDialog(parent)
    , mParser(parser)
    , mViewport(viewport)
{
    setWindowTitle(tr("Mesh Editor"));
    setMinimumSize(520, 480);

    auto* mainLayout = new QVBoxLayout(this);

    // --- Whole-mesh transform ---
    auto* transformGroup = new QGroupBox(tr("Transform Mesh"));
    auto* form = new QFormLayout(transformGroup);

    auto* dx = new QDoubleSpinBox(); dx->setRange(-1e6, 1e6); dx->setObjectName("dx");
    auto* dy = new QDoubleSpinBox(); dy->setRange(-1e6, 1e6); dy->setObjectName("dy");
    auto* dz = new QDoubleSpinBox(); dz->setRange(-1e6, 1e6); dz->setObjectName("dz");
    auto* transLayout = new QHBoxLayout();
    transLayout->addWidget(dx); transLayout->addWidget(dy); transLayout->addWidget(dz);
    form->addRow(tr("Translate (X,Y,Z)"), transLayout);

    auto* scaleBox = new QDoubleSpinBox();
    scaleBox->setRange(0.0001, 1e6); scaleBox->setValue(1.0); scaleBox->setObjectName("scale");
    form->addRow(tr("Scale"), scaleBox);

    auto* applyTransformBtn = new QPushButton(tr("Apply Transform"));
    form->addRow(applyTransformBtn);
    mainLayout->addWidget(transformGroup);

    connect(applyTransformBtn, &QPushButton::clicked, this, &MeshEditorDialog::applyTransform);

    // --- Vertex table ---
    auto* vertexGroup = new QGroupBox(tr("Vertices (editable)"));
    auto* vLayout = new QVBoxLayout(vertexGroup);

    mTable = new QTableWidget(0, 6, this);
    mTable->setHorizontalHeaderLabels(
        {tr("Shape"), tr("Index"), tr("X"), tr("Y"), tr("Z"), tr("U/V")});
    mTable->horizontalHeader()->setStretchLastSection(true);
    vLayout->addWidget(mTable);

    auto* vBtnLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Vertex"));
    auto* removeBtn = new QPushButton(tr("Remove Selected"));
    auto* applyBtn = new QPushButton(tr("Apply Vertex Edits"));
    vBtnLayout->addWidget(addBtn);
    vBtnLayout->addWidget(removeBtn);
    vBtnLayout->addWidget(applyBtn);
    vLayout->addLayout(vBtnLayout);
    mainLayout->addWidget(vertexGroup);

    connect(addBtn, &QPushButton::clicked, this, &MeshEditorDialog::addVertex);
    connect(removeBtn, &QPushButton::clicked, this, &MeshEditorDialog::removeSelected);
    connect(applyBtn, &QPushButton::clicked, this, &MeshEditorDialog::applyVertexEdits);

    // --- Faces (triangles) ---
    auto* faceGroup = new QGroupBox(tr("Faces (triangles, per shape)"));
    auto* fLayout = new QVBoxLayout(faceGroup);

    mFaceTable = new QTableWidget(0, 4, this);
    mFaceTable->setHorizontalHeaderLabels({tr("Shape"), tr("Face #"), tr("V0"), tr("V1"), tr("V2")});
    mFaceTable->horizontalHeader()->setStretchLastSection(true);
    fLayout->addWidget(mFaceTable);

    auto* fBtnLayout = new QHBoxLayout();
    auto* addFaceBtn = new QPushButton(tr("Add Face"));
    auto* removeFaceBtn = new QPushButton(tr("Remove Selected Faces"));
    auto* applyFaceBtn = new QPushButton(tr("Apply Face Edits"));
    fBtnLayout->addWidget(addFaceBtn);
    fBtnLayout->addWidget(removeFaceBtn);
    fBtnLayout->addWidget(applyFaceBtn);
    fLayout->addLayout(fBtnLayout);
    mainLayout->addWidget(faceGroup);

    connect(addFaceBtn, &QPushButton::clicked, this, &MeshEditorDialog::addFace);
    connect(removeFaceBtn, &QPushButton::clicked, this, &MeshEditorDialog::removeSelectedFaces);
    connect(applyFaceBtn, &QPushButton::clicked, this, &MeshEditorDialog::applyFaceEdits);

    // --- Face Operations (extrude / bevel) ---
    auto* faceOpsGroup = new QGroupBox(tr("Face Operations"));
    auto* foLayout = new QFormLayout(faceOpsGroup);

    auto* extrudeDistBox = new QDoubleSpinBox();
    extrudeDistBox->setRange(0.1, 1000.0);
    extrudeDistBox->setValue(10.0);
    extrudeDistBox->setObjectName("extrudeDist");
    foLayout->addRow(tr("Extrude Distance"), extrudeDistBox);

    auto* bevelAmtBox = new QDoubleSpinBox();
    bevelAmtBox->setRange(0.01, 1.0);
    bevelAmtBox->setValue(0.5);
    bevelAmtBox->setSingleStep(0.05);
    bevelAmtBox->setObjectName("bevelAmt");
    foLayout->addRow(tr("Bevel Amount"), bevelAmtBox);

    auto* foBtnLayout = new QHBoxLayout();
    auto* extrudeBtn = new QPushButton(tr("Extrude"));
    auto* bevelBtn = new QPushButton(tr("Bevel"));
    foBtnLayout->addWidget(extrudeBtn);
    foBtnLayout->addWidget(bevelBtn);
    foLayout->addRow(foBtnLayout);
    mainLayout->addWidget(faceOpsGroup);

    connect(extrudeBtn, &QPushButton::clicked, this, &MeshEditorDialog::extrudeSelectedFace);
    connect(bevelBtn, &QPushButton::clicked, this, &MeshEditorDialog::bevelSelectedFace);

    // --- Materials ---
    auto* materialGroup = new QGroupBox(tr("Materials (per shape)"));
    auto* mLayout = new QFormLayout(materialGroup);

    auto* shapeCombo = new QComboBox();
    shapeCombo->setObjectName("shapeCombo");
    if (mParser && mParser->getRoot()) {
        const auto* root = mParser->getRoot();
        for (int s = 0; s < root->shapes.size(); ++s) {
            shapeCombo->addItem(
                QString("%1 (%2 verts)").arg(root->shapes[s].name).arg(root->shapes[s].vertices.size()),
                s);
            mShapeColors[s] = QColor::fromRgbF(
                qBound(0.0f, root->shapes[s].baseColor.r, 1.0f),
                qBound(0.0f, root->shapes[s].baseColor.g, 1.0f),
                qBound(0.0f, root->shapes[s].baseColor.b, 1.0f));
            mShapeTextures[s] = root->shapes[s].texture;
        }
    }
    mLayout->addRow(tr("Shape"), shapeCombo);

    auto* colorBtn = new QPushButton(tr("Base Color..."));
    colorBtn->setObjectName("colorBtn");
    mLayout->addRow(tr("Base Color"), colorBtn);

    auto* textureEdit = new QLineEdit();
    textureEdit->setObjectName("textureEdit");
    auto* browseBtn = new QPushButton(tr("Browse..."));
    auto* exportBtn = new QPushButton(tr("Export DDS..."));
    auto* texLayout = new QHBoxLayout();
    texLayout->addWidget(textureEdit);
    texLayout->addWidget(browseBtn);
    mLayout->addRow(tr("Texture"), texLayout);

    auto* applyMatBtn = new QPushButton(tr("Apply Material"));
    mLayout->addRow(applyMatBtn);

    auto* texEditorBtn = new QPushButton(tr("Texture Editor..."));
    mLayout->addRow(texEditorBtn);

    auto* exportTexLayout = new QHBoxLayout();
    exportTexLayout->addStretch();
    exportTexLayout->addWidget(exportBtn);
    mainLayout->addLayout(exportTexLayout);
    mainLayout->addWidget(materialGroup);

    connect(shapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeshEditorDialog::onShapeSelected);
    connect(colorBtn, &QPushButton::clicked, this, &MeshEditorDialog::pickColor);
    connect(browseBtn, &QPushButton::clicked, this, &MeshEditorDialog::browseTexture);
    connect(exportBtn, &QPushButton::clicked, this, &MeshEditorDialog::exportTexture);
    connect(applyMatBtn, &QPushButton::clicked, this, &MeshEditorDialog::applyMaterial);
    connect(texEditorBtn, &QPushButton::clicked, this, &MeshEditorDialog::openTextureEditor);

    // Initialize first shape selection
    onShapeSelected(shapeCombo->currentData().toInt());

    // --- Dialog buttons ---
    auto* dialogBtns = new QHBoxLayout();
    auto* closeBtn = new QPushButton(tr("Close"));
    dialogBtns->addStretch();
    dialogBtns->addWidget(closeBtn);
    mainLayout->addLayout(dialogBtns);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    loadVertices();
    loadFaces();
}

void MeshEditorDialog::loadVertices()
{
    mTable->setRowCount(0);
    if (!mParser || !mParser->getRoot()) {
        return;
    }

    const auto* root = mParser->getRoot();
    for (int s = 0; s < root->shapes.size(); ++s) {
        const auto& shape = root->shapes[s];
        for (int v = 0; v < shape.vertices.size(); ++v) {
            const int row = mTable->rowCount();
            mTable->insertRow(row);

            auto* shapeItem = new QTableWidgetItem(QString::number(s));
            shapeItem->setFlags(shapeItem->flags() & ~Qt::ItemIsEditable);
            mTable->setItem(row, 0, shapeItem);

            auto* indexItem = new QTableWidgetItem(QString::number(v));
            indexItem->setFlags(indexItem->flags() & ~Qt::ItemIsEditable);
            mTable->setItem(row, 1, indexItem);

            mTable->setItem(row, 2, new QTableWidgetItem(QString::number(shape.vertices[v].x)));
            mTable->setItem(row, 3, new QTableWidgetItem(QString::number(shape.vertices[v].y)));
            mTable->setItem(row, 4, new QTableWidgetItem(QString::number(shape.vertices[v].z)));

            const Nif::Vector2 uv = (v < shape.uvs.size()) ? shape.uvs[v] : Nif::Vector2{0.0f, 0.0f};
            mTable->setItem(row, 5, new QTableWidgetItem(QString("%1, %2").arg(uv.u).arg(uv.v)));
        }
    }
}

void MeshEditorDialog::applyTransform()
{
    if (!mParser) return;

    auto* dx = findChild<QDoubleSpinBox*>("dx");
    auto* dy = findChild<QDoubleSpinBox*>("dy");
    auto* dz = findChild<QDoubleSpinBox*>("dz");
    auto* scaleBox = findChild<QDoubleSpinBox*>("scale");

    if (dx && dy && dz) {
        mParser->translateAll(static_cast<float>(dx->value()),
                              static_cast<float>(dy->value()),
                              static_cast<float>(dz->value()));
    }
    if (scaleBox && scaleBox->value() != 1.0) {
        mParser->scaleAll(static_cast<float>(scaleBox->value()));
    }

    if (mViewport) mViewport->refreshMesh();
    loadVertices();
    LOG_INFO("Mesh transform applied");
}

void MeshEditorDialog::applyVertexEdits()
{
    if (!mParser || !mParser->getRoot()) return;

    auto* root = mParser->getRoot();
    for (int row = 0; row < mTable->rowCount(); ++row) {
        const int shapeIdx = mTable->item(row, 0)->text().toInt();
        const int vertIdx = mTable->item(row, 1)->text().toInt();
        if (shapeIdx < 0 || shapeIdx >= root->shapes.size()) continue;
        auto& shape = root->shapes[shapeIdx];
        if (vertIdx < 0 || vertIdx >= shape.vertices.size()) continue;

        shape.vertices[vertIdx].x = mTable->item(row, 2)->text().toFloat();
        shape.vertices[vertIdx].y = mTable->item(row, 3)->text().toFloat();
        shape.vertices[vertIdx].z = mTable->item(row, 4)->text().toFloat();

        const QStringList uv = mTable->item(row, 5)->text().split(',');
        if (uv.size() == 2 && vertIdx < shape.uvs.size()) {
            shape.uvs[vertIdx].u = uv[0].trimmed().toFloat();
            shape.uvs[vertIdx].v = uv[1].trimmed().toFloat();
        }
    }

    if (mViewport) mViewport->refreshMesh();
    LOG_INFO("Vertex edits applied");
}

void MeshEditorDialog::addVertex()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* root = mParser->getRoot();
    if (root->shapes.isEmpty()) return;

    // Add to the last shape.
    auto& shape = root->shapes.last();
    Nif::Vector3 v{0.0f, 0.0f, 0.0f};
    Nif::Vector2 uv{0.0f, 0.0f};
    Nif::Color4 c{1.0f, 1.0f, 1.0f, 1.0f};
    shape.addVertex(v, uv, c);

    if (mViewport) mViewport->refreshMesh();
    loadVertices();
    LOG_INFO("Vertex added");
}

void MeshEditorDialog::removeSelected()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* root = mParser->getRoot();

    const QList<QTableWidgetSelectionRange> ranges = mTable->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this, tr("Remove Vertex"),
                                 tr("Select a row to remove."));
        return;
    }

    // Remove from highest row index downward to keep indices stable.
    QVector<QPair<int, int>> targets;
    for (const auto& range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            const int shapeIdx = mTable->item(r, 0)->text().toInt();
            const int vertIdx = mTable->item(r, 1)->text().toInt();
            targets.append({shapeIdx, vertIdx});
        }
    }
    std::sort(targets.begin(), targets.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first > b.first ||
                         (a.first == b.first && a.second > b.second);
              });

    for (const auto& t : targets) {
        if (t.first >= 0 && t.first < root->shapes.size()) {
            root->shapes[t.first].removeVertex(t.second);
        }
    }

    if (mViewport) mViewport->refreshMesh();
    loadVertices();
    LOG_INFO("Vertex(s) removed");
}

void MeshEditorDialog::onShapeSelected(int index)
{
    auto* colorBtn = findChild<QPushButton*>("colorBtn");
    auto* textureEdit = findChild<QLineEdit*>("textureEdit");

    mCurrentColor = mShapeColors.value(index, QColor(180, 180, 200));
    if (colorBtn) {
        colorBtn->setStyleSheet(
            QString("background-color: %1").arg(mCurrentColor.name()));
    }
    if (textureEdit) {
        textureEdit->setText(mShapeTextures.value(index));
    }
}

void MeshEditorDialog::pickColor()
{
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    if (!shapeCombo) return;
    const int idx = shapeCombo->currentData().toInt();

    QColor chosen = QColorDialog::getColor(mCurrentColor, this, tr("Select Base Color"));
    if (chosen.isValid()) {
        mCurrentColor = chosen;
        mShapeColors[idx] = chosen;
        auto* colorBtn = findChild<QPushButton*>("colorBtn");
        if (colorBtn) {
            colorBtn->setStyleSheet(
                QString("background-color: %1").arg(chosen.name()));
        }
    }
}

void MeshEditorDialog::browseTexture()
{
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    auto* textureEdit = findChild<QLineEdit*>("textureEdit");
    if (!shapeCombo || !textureEdit) return;
    const int idx = shapeCombo->currentData().toInt();

    QString path = QFileDialog::getOpenFileName(this, tr("Select Texture"),
        textureEdit->text(), tr("Images (*.png *.jpg *.jpeg *.bmp *.dds *.tga)"));
    if (!path.isEmpty()) {
        textureEdit->setText(path);
        mShapeTextures[idx] = path;
    }
}

void MeshEditorDialog::applyMaterial()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    auto* textureEdit = findChild<QLineEdit*>("textureEdit");
    if (!shapeCombo) return;
    const int idx = shapeCombo->currentData().toInt();
    if (idx < 0 || idx >= mParser->getRoot()->shapes.size()) return;

    auto& shape = mParser->getRoot()->shapes[idx];
    const QColor c = mShapeColors.value(idx, QColor(255, 255, 255));
    shape.baseColor = {static_cast<float>(c.redF()),
                       static_cast<float>(c.greenF()),
                       static_cast<float>(c.blueF()),
                       1.0f};
    shape.texture = (textureEdit ? textureEdit->text() : mShapeTextures.value(idx));

    if (mViewport) mViewport->refreshMesh();
    LOG_INFO(QString("Material applied to shape %1").arg(idx));
}

void MeshEditorDialog::loadFaces()
{
    mFaceTable->setRowCount(0);
    if (!mParser || !mParser->getRoot()) return;

    const auto* root = mParser->getRoot();
    for (int s = 0; s < root->shapes.size(); ++s) {
        const auto& shape = root->shapes[s];
        for (int f = 0; f < static_cast<int>(shape.indices.size()) / 3; ++f) {
            const int row = mFaceTable->rowCount();
            mFaceTable->insertRow(row);

            auto* shapeItem = new QTableWidgetItem(QString::number(s));
            shapeItem->setFlags(shapeItem->flags() & ~Qt::ItemIsEditable);
            mFaceTable->setItem(row, 0, shapeItem);

            auto* faceItem = new QTableWidgetItem(QString::number(f));
            faceItem->setFlags(faceItem->flags() & ~Qt::ItemIsEditable);
            mFaceTable->setItem(row, 1, faceItem);

            const int i0 = shape.indices[f * 3 + 0];
            const int i1 = shape.indices[f * 3 + 1];
            const int i2 = shape.indices[f * 3 + 2];
            mFaceTable->setItem(row, 2, new QTableWidgetItem(QString::number(i0)));
            mFaceTable->setItem(row, 3, new QTableWidgetItem(QString::number(i1)));
            mFaceTable->setItem(row, 4, new QTableWidgetItem(QString::number(i2)));
        }
    }
}

void MeshEditorDialog::addFace()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    if (!shapeCombo) return;
    const int idx = shapeCombo->currentData().toInt();
    if (idx < 0 || idx >= mParser->getRoot()->shapes.size()) return;

    auto& shape = mParser->getRoot()->shapes[idx];
    if (shape.vertices.isEmpty()) return;

    // Add a face referencing the first 3 vertices (or clamp to available)
    const int vCount = shape.vertices.size();
    shape.indices.append(vCount > 0 ? 0 : 0);
    shape.indices.append(vCount > 1 ? 1 : 0);
    shape.indices.append(vCount > 2 ? 2 : 0);

    loadFaces();
    if (mViewport) mViewport->refreshMesh();
    LOG_INFO(QString("Face added to shape %1").arg(idx));
}

void MeshEditorDialog::removeSelectedFaces()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* root = mParser->getRoot();

    const QList<QTableWidgetSelectionRange> ranges = mFaceTable->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this, tr("Remove Faces"),
                                 tr("Select rows to remove."));
        return;
    }

    // Collect unique (shape, face) pairs and sort descending to remove safely
    QVector<QPair<int, int>> targets;
    for (const auto& range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            const int shapeIdx = mFaceTable->item(r, 0)->text().toInt();
            const int faceIdx = mFaceTable->item(r, 1)->text().toInt();
            targets.append({shapeIdx, faceIdx});
        }
    }
    std::sort(targets.begin(), targets.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first > b.first ||
                         (a.first == b.first && a.second > b.second);
              });

    for (const auto& t : targets) {
        if (t.first >= 0 && t.first < root->shapes.size()) {
            auto& shape = root->shapes[t.first];
            const int faceIdx = t.second;
            const int start = faceIdx * 3;
            if (start + 2 < static_cast<int>(shape.indices.size())) {
                shape.indices.removeAt(start);
                shape.indices.removeAt(start);
                shape.indices.removeAt(start);
            }
        }
    }

    loadFaces();
    if (mViewport) mViewport->refreshMesh();
    LOG_INFO("Face(s) removed");
}

void MeshEditorDialog::applyFaceEdits()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* root = mParser->getRoot();

    // First, rebuild all shape indices from the face table
    for (int s = 0; s < root->shapes.size(); ++s) {
        root->shapes[s].indices.clear();
    }

    for (int row = 0; row < mFaceTable->rowCount(); ++row) {
        const int shapeIdx = mFaceTable->item(row, 0)->text().toInt();
        if (shapeIdx < 0 || shapeIdx >= root->shapes.size()) continue;
        auto& shape = root->shapes[shapeIdx];

        const int v0 = mFaceTable->item(row, 2)->text().toInt();
        const int v1 = mFaceTable->item(row, 3)->text().toInt();
        const int v2 = mFaceTable->item(row, 4)->text().toInt();

        shape.indices.append(static_cast<unsigned int>(v0));
        shape.indices.append(static_cast<unsigned int>(v1));
        shape.indices.append(static_cast<unsigned int>(v2));
    }

    loadFaces();
    if (mViewport) mViewport->refreshMesh();
    LOG_INFO("Face edits applied");
}

void MeshEditorDialog::extrudeSelectedFace()
{
    if (!mParser || !mParser->getRoot()) return;

    const QList<QTableWidgetSelectionRange> ranges = mFaceTable->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this, tr("Extrude Face"),
                                 tr("Select a face row to extrude."));
        return;
    }

    auto* extrudeDistBox = findChild<QDoubleSpinBox*>("extrudeDist");
    const float distance = extrudeDistBox ? static_cast<float>(extrudeDistBox->value()) : 10.0f;

    // Process selected faces from bottom up so indices stay valid
    QVector<QPair<int, int>> targets;
    for (const auto& range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            const int shapeIdx = mFaceTable->item(r, 0)->text().toInt();
            const int faceIdx = mFaceTable->item(r, 1)->text().toInt();
            targets.append({shapeIdx, faceIdx});
        }
    }
    std::sort(targets.begin(), targets.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first > b.first ||
                         (a.first == b.first && a.second > b.second);
              });

    for (const auto& t : targets) {
        if (t.first >= 0 && t.first < mParser->getRoot()->shapes.size()) {
            mParser->getRoot()->shapes[t.first].extrudeFace(t.second, distance);
        }
    }

    loadVertices();
    loadFaces();
    if (mViewport) mViewport->refreshMesh();
    LOG_INFO(QString("Extruded %1 face(s), distance=%2").arg(targets.size()).arg(distance));
}

void MeshEditorDialog::bevelSelectedFace()
{
    if (!mParser || !mParser->getRoot()) return;

    const QList<QTableWidgetSelectionRange> ranges = mFaceTable->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this, tr("Bevel Face"),
                                 tr("Select a face row to bevel."));
        return;
    }

    auto* bevelAmtBox = findChild<QDoubleSpinBox*>("bevelAmt");
    const float amount = bevelAmtBox ? static_cast<float>(bevelAmtBox->value()) : 0.5f;

    QVector<QPair<int, int>> targets;
    for (const auto& range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            const int shapeIdx = mFaceTable->item(r, 0)->text().toInt();
            const int faceIdx = mFaceTable->item(r, 1)->text().toInt();
            targets.append({shapeIdx, faceIdx});
        }
    }
    std::sort(targets.begin(), targets.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first > b.first ||
                         (a.first == b.first && a.second > b.second);
              });

    for (const auto& t : targets) {
        if (t.first >= 0 && t.first < mParser->getRoot()->shapes.size()) {
            mParser->getRoot()->shapes[t.first].bevelFace(t.second, amount);
        }
    }

    loadVertices();
    loadFaces();
    if (mViewport) mViewport->refreshMesh();
    LOG_INFO(QString("Beveled %1 face(s), amount=%2").arg(targets.size()).arg(amount));
}

void MeshEditorDialog::exportTexture()
{
    if (!mParser || !mParser->getRoot()) return;
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    if (!shapeCombo) return;
    const int idx = shapeCombo->currentData().toInt();
    if (idx < 0 || idx >= mParser->getRoot()->shapes.size()) return;

    const auto& shape = mParser->getRoot()->shapes[idx];
    if (shape.texture.isEmpty()) {
        QMessageBox::information(this, tr("Export Texture"),
                                 tr("No texture assigned to this shape."));
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, tr("Export Texture"),
        QFileInfo(shape.texture).completeBaseName() + ".dds",
        tr("DDS Images (*.dds)"));
    if (savePath.isEmpty()) return;

    // Load the texture image from the assigned path
    QImage texImg = NifViewportWidget::loadTextureImage(shape.texture);
    if (texImg.isNull()) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Cannot load texture from: %1").arg(shape.texture));
        return;
    }

    // Encode as DDS
    if (!DdsEncoder::encode(texImg, savePath, 5)) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Failed to encode DDS: %1").arg(savePath));
        return;
    }

    LOG_INFO(QString("Texture exported: %1 (%2x%3)").arg(savePath).arg(texImg.width()).arg(texImg.height()));
    QMessageBox::information(this, tr("Export Complete"),
                             tr("Texture exported to:\n%1").arg(savePath));
}

void MeshEditorDialog::openTextureEditor()
{
    auto* shapeCombo = findChild<QComboBox*>("shapeCombo");
    if (!shapeCombo || !mParser) return;
    const int idx = shapeCombo->currentData().toInt();

    TextureEditorDialog dialog(mParser, mViewport, idx, this);
    dialog.exec();

    if (mViewport) mViewport->refreshMesh();
}
