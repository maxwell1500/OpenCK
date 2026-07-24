#include "navmesheditordialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QQueue>
#include <cmath>
#include <limits>
#include <algorithm>
#include "logger.hpp"

NavmeshEditorDialog::NavmeshEditorDialog(QWidget* parent)
    : QDialog(parent),
      mTriangleCountLabel(nullptr),
      mVertexCountLabel(nullptr),
      mEdgeCountLabel(nullptr),
      mTriangleTable(nullptr),
      mVerticesTable(nullptr),
      mAddVertexButton(nullptr),
      mRemoveVertexButton(nullptr),
      mVertexX(nullptr),
      mVertexY(nullptr),
      mVertexZ(nullptr),
      mEdgesTable(nullptr),
      mAddEdgeButton(nullptr),
      mRemoveEdgeButton(nullptr),
      mEdgeBlockedCheck(nullptr),
      mPortalsTable(nullptr),
      mAddPortalButton(nullptr),
      mRemovePortalButton(nullptr),
      mStartX(nullptr),
      mStartY(nullptr),
      mStartZ(nullptr),
      mEndX(nullptr),
      mEndY(nullptr),
      mEndZ(nullptr),
      mFindPathButton(nullptr),
      mPathResultList(nullptr),
      mHighlightPathCheck(nullptr)
{
    LOG_INFO("NavmeshEditorDialog created");
    setupUI();
}

NavmeshEditorDialog::~NavmeshEditorDialog()
{
}

void NavmeshEditorDialog::setNavMesh(const NavMeshData& mesh)
{
    mMesh = mesh;
    refreshInfoPanel();
    refreshVerticesTable();
    refreshEdgesTable();
    refreshPortalsTable();
}

NavMeshData NavmeshEditorDialog::getNavMesh() const
{
    return mMesh;
}

void NavmeshEditorDialog::setupUI()
{
    setWindowTitle("NavMesh Editor - Detailed");
    setMinimumSize(1000, 700);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    setupInfoPanel(splitter);
    setupEditingTools(splitter);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);
}

void NavmeshEditorDialog::setupInfoPanel(QSplitter* splitter)
{
    auto* infoWidget = new QWidget();
    auto* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(4, 4, 4, 4);

    auto* statsGroup = new QGroupBox("NavMesh Statistics");
    auto* statsLayout = new QVBoxLayout(statsGroup);

    mTriangleCountLabel = new QLabel("Triangles: 0");
    mVertexCountLabel = new QLabel("Vertices: 0");
    mEdgeCountLabel = new QLabel("Edges: 0");

    statsLayout->addWidget(mTriangleCountLabel);
    statsLayout->addWidget(mVertexCountLabel);
    statsLayout->addWidget(mEdgeCountLabel);
    infoLayout->addWidget(statsGroup);

    auto* triangleGroup = new QGroupBox("Triangles");
    auto* triangleLayout = new QVBoxLayout(triangleGroup);

    mTriangleTable = new QTableWidget();
    mTriangleTable->setColumnCount(6);
    mTriangleTable->setHorizontalHeaderLabels(
        QStringList() << "Index" << "V0" << "V1" << "V2" << "Normal" << "Walkable");
    mTriangleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTriangleTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTriangleTable->horizontalHeader()->setStretchLastSection(true);
    mTriangleTable->verticalHeader()->setVisible(false);
    triangleLayout->addWidget(mTriangleTable);

    infoLayout->addWidget(triangleGroup, 1);
    splitter->addWidget(infoWidget);

    connect(mTriangleTable, &QTableWidget::cellClicked,
            this, &NavmeshEditorDialog::onTriangleRowClicked);
}

void NavmeshEditorDialog::setupEditingTools(QSplitter* splitter)
{
    auto* toolsWidget = new QWidget();
    auto* toolsLayout = new QVBoxLayout(toolsWidget);
    toolsLayout->setContentsMargins(4, 4, 4, 4);

    auto* tabWidget = new QTabWidget();

    // Vertices tab
    auto* vertTab = new QWidget();
    auto* vertTabLayout = new QVBoxLayout(vertTab);
    mVerticesTable = new QTableWidget();
    mVerticesTable->setColumnCount(3);
    mVerticesTable->setHorizontalHeaderLabels(QStringList() << "X" << "Y" << "Z");
    mVerticesTable->horizontalHeader()->setStretchLastSection(true);
    mVerticesTable->verticalHeader()->setVisible(false);
    mVerticesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mVerticesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    vertTabLayout->addWidget(mVerticesTable, 1);

    auto* vertEditGroup = new QGroupBox("Edit Position");
    auto* vertEditLayout = new QGridLayout(vertEditGroup);
    vertEditLayout->addWidget(new QLabel("X:"), 0, 0);
    mVertexX = new QDoubleSpinBox();
    mVertexX->setRange(-100000, 100000);
    mVertexX->setDecimals(4);
    vertEditLayout->addWidget(mVertexX, 0, 1);
    vertEditLayout->addWidget(new QLabel("Y:"), 0, 2);
    mVertexY = new QDoubleSpinBox();
    mVertexY->setRange(-100000, 100000);
    mVertexY->setDecimals(4);
    vertEditLayout->addWidget(mVertexY, 0, 3);
    vertEditLayout->addWidget(new QLabel("Z:"), 0, 4);
    mVertexZ = new QDoubleSpinBox();
    mVertexZ->setRange(-100000, 100000);
    mVertexZ->setDecimals(4);
    vertEditLayout->addWidget(mVertexZ, 0, 5);
    vertTabLayout->addWidget(vertEditGroup);

    auto* vertButtonLayout = new QHBoxLayout();
    mAddVertexButton = new QPushButton("Add Vertex");
    vertButtonLayout->addWidget(mAddVertexButton);
    mRemoveVertexButton = new QPushButton("Remove Vertex");
    mRemoveVertexButton->setEnabled(false);
    vertButtonLayout->addWidget(mRemoveVertexButton);
    vertButtonLayout->addStretch();
    vertTabLayout->addLayout(vertButtonLayout);

    tabWidget->addTab(vertTab, "Vertices");

    // Edges tab
    auto* edgeTab = new QWidget();
    auto* edgeTabLayout = new QVBoxLayout(edgeTab);
    mEdgesTable = new QTableWidget();
    mEdgesTable->setColumnCount(3);
    mEdgesTable->setHorizontalHeaderLabels(QStringList() << "Start Vertex" << "End Vertex" << "Blocked");
    mEdgesTable->horizontalHeader()->setStretchLastSection(true);
    mEdgesTable->verticalHeader()->setVisible(false);
    mEdgesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mEdgesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    edgeTabLayout->addWidget(mEdgesTable, 1);

    auto* edgeEditGroup = new QGroupBox("Edge Properties");
    auto* edgeEditLayout = new QHBoxLayout(edgeEditGroup);
    mEdgeBlockedCheck = new QCheckBox("Blocked");
    edgeEditLayout->addWidget(mEdgeBlockedCheck);
    edgeEditLayout->addStretch();
    edgeTabLayout->addWidget(edgeEditGroup);

    auto* edgeButtonLayout = new QHBoxLayout();
    mAddEdgeButton = new QPushButton("Add Edge");
    edgeButtonLayout->addWidget(mAddEdgeButton);
    mRemoveEdgeButton = new QPushButton("Remove Edge");
    mRemoveEdgeButton->setEnabled(false);
    edgeButtonLayout->addWidget(mRemoveEdgeButton);
    edgeButtonLayout->addStretch();
    edgeTabLayout->addLayout(edgeButtonLayout);

    tabWidget->addTab(edgeTab, "Edges");

    // Portals tab
    auto* portalTab = new QWidget();
    auto* portalTabLayout = new QVBoxLayout(portalTab);
    mPortalsTable = new QTableWidget();
    mPortalsTable->setColumnCount(4);
    mPortalsTable->setHorizontalHeaderLabels(
        QStringList() << "Portal Name" << "Triangle A" << "Triangle B" << "Width");
    mPortalsTable->horizontalHeader()->setStretchLastSection(true);
    mPortalsTable->verticalHeader()->setVisible(false);
    mPortalsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPortalsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    portalTabLayout->addWidget(mPortalsTable, 1);

    auto* portalButtonLayout = new QHBoxLayout();
    mAddPortalButton = new QPushButton("Add Portal");
    portalButtonLayout->addWidget(mAddPortalButton);
    mRemovePortalButton = new QPushButton("Remove Portal");
    mRemovePortalButton->setEnabled(false);
    portalButtonLayout->addWidget(mRemovePortalButton);
    portalButtonLayout->addStretch();
    portalTabLayout->addLayout(portalButtonLayout);

    tabWidget->addTab(portalTab, "Portals");

    // Pathfinding tab
    auto* pathTab = new QWidget();
    auto* pathTabLayout = new QVBoxLayout(pathTab);

    auto* posGroup = new QGroupBox("Positions");
    auto* posLayout = new QGridLayout(posGroup);

    posLayout->addWidget(new QLabel("Start X:"), 0, 0);
    mStartX = new QDoubleSpinBox();
    mStartX->setRange(-100000, 100000);
    mStartX->setDecimals(2);
    posLayout->addWidget(mStartX, 0, 1);
    posLayout->addWidget(new QLabel("Y:"), 0, 2);
    mStartY = new QDoubleSpinBox();
    mStartY->setRange(-100000, 100000);
    mStartY->setDecimals(2);
    posLayout->addWidget(mStartY, 0, 3);
    posLayout->addWidget(new QLabel("Z:"), 0, 4);
    mStartZ = new QDoubleSpinBox();
    mStartZ->setRange(-100000, 100000);
    mStartZ->setDecimals(2);
    posLayout->addWidget(mStartZ, 0, 5);

    posLayout->addWidget(new QLabel("End X:"), 1, 0);
    mEndX = new QDoubleSpinBox();
    mEndX->setRange(-100000, 100000);
    mEndX->setDecimals(2);
    posLayout->addWidget(mEndX, 1, 1);
    posLayout->addWidget(new QLabel("Y:"), 1, 2);
    mEndY = new QDoubleSpinBox();
    mEndY->setRange(-100000, 100000);
    mEndY->setDecimals(2);
    posLayout->addWidget(mEndY, 1, 3);
    posLayout->addWidget(new QLabel("Z:"), 1, 4);
    mEndZ = new QDoubleSpinBox();
    mEndZ->setRange(-100000, 100000);
    mEndZ->setDecimals(2);
    posLayout->addWidget(mEndZ, 1, 5);

    pathTabLayout->addWidget(posGroup);

    auto* pathButtonLayout = new QHBoxLayout();
    mFindPathButton = new QPushButton("Find Path");
    pathButtonLayout->addWidget(mFindPathButton);
    mHighlightPathCheck = new QCheckBox("Highlight Path");
    pathButtonLayout->addWidget(mHighlightPathCheck);
    pathButtonLayout->addStretch();
    pathTabLayout->addLayout(pathButtonLayout);

    auto* resultGroup = new QGroupBox("Path Result");
    auto* resultLayout = new QVBoxLayout(resultGroup);
    mPathResultList = new QListWidget();
    resultLayout->addWidget(mPathResultList);
    pathTabLayout->addWidget(resultGroup, 1);

    tabWidget->addTab(pathTab, "Pathfinding");

    toolsLayout->addWidget(tabWidget, 1);
    splitter->addWidget(toolsWidget);

    connect(mAddVertexButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onAddVertex);
    connect(mRemoveVertexButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onRemoveVertex);
    connect(mVerticesTable, &QTableWidget::cellChanged, this, &NavmeshEditorDialog::onVertexCellChanged);

    connect(mAddEdgeButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onAddEdge);
    connect(mRemoveEdgeButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onRemoveEdge);
    connect(mEdgesTable, &QTableWidget::cellChanged, this, &NavmeshEditorDialog::onEdgeCellChanged);

    connect(mAddPortalButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onAddPortal);
    connect(mRemovePortalButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onRemovePortal);
    connect(mPortalsTable, &QTableWidget::cellChanged, this, &NavmeshEditorDialog::onPortalCellChanged);

    connect(mFindPathButton, &QPushButton::clicked, this, &NavmeshEditorDialog::onFindPath);
    connect(mHighlightPathCheck, &QCheckBox::toggled, this, &NavmeshEditorDialog::onHighlightPathToggled);
}

void NavmeshEditorDialog::refreshInfoPanel()
{
    mTriangleCountLabel->setText(QString("Triangles: %1").arg(mMesh.triangles.size()));
    mVertexCountLabel->setText(QString("Vertices: %1").arg(mMesh.vertices.size()));
    mEdgeCountLabel->setText(QString("Edges: %1").arg(mMesh.edges.size()));

    mTriangleTable->blockSignals(true);
    mTriangleTable->setRowCount(mMesh.triangles.size());

    for (int i = 0; i < mMesh.triangles.size(); ++i) {
        const auto& tri = mMesh.triangles[i];
        auto* idxItem = new QTableWidgetItem(QString::number(i));
        idxItem->setFlags(idxItem->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 0, idxItem);

        auto* v0Item = new QTableWidgetItem(QString::number(tri.v0));
        v0Item->setFlags(v0Item->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 1, v0Item);

        auto* v1Item = new QTableWidgetItem(QString::number(tri.v1));
        v1Item->setFlags(v1Item->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 2, v1Item);

        auto* v2Item = new QTableWidgetItem(QString::number(tri.v2));
        v2Item->setFlags(v2Item->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 3, v2Item);

        QString normalStr = QString("(%1, %2, %3)")
            .arg(tri.normal.x(), 0, 'f', 2)
            .arg(tri.normal.y(), 0, 'f', 2)
            .arg(tri.normal.z(), 0, 'f', 2);
        auto* normalItem = new QTableWidgetItem(normalStr);
        normalItem->setFlags(normalItem->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 4, normalItem);

        auto* walkItem = new QTableWidgetItem(tri.walkable ? "Yes" : "No");
        walkItem->setFlags(walkItem->flags() & ~Qt::ItemIsEditable);
        mTriangleTable->setItem(i, 5, walkItem);
    }

    mTriangleTable->resizeColumnsToContents();
    mTriangleTable->blockSignals(false);
}

void NavmeshEditorDialog::refreshVerticesTable()
{
    mVerticesTable->blockSignals(true);
    mVerticesTable->setRowCount(mMesh.vertices.size());

    for (int i = 0; i < mMesh.vertices.size(); ++i) {
        const auto& v = mMesh.vertices[i];
        auto* xItem = new QTableWidgetItem(QString::number(v.x(), 'f', 4));
        mVerticesTable->setItem(i, 0, xItem);

        auto* yItem = new QTableWidgetItem(QString::number(v.y(), 'f', 4));
        mVerticesTable->setItem(i, 1, yItem);

        auto* zItem = new QTableWidgetItem(QString::number(v.z(), 'f', 4));
        mVerticesTable->setItem(i, 2, zItem);
    }

    mVerticesTable->resizeColumnsToContents();
    mVerticesTable->blockSignals(false);
}

void NavmeshEditorDialog::refreshEdgesTable()
{
    mEdgesTable->blockSignals(true);
    mEdgesTable->setRowCount(mMesh.edges.size());

    for (int i = 0; i < mMesh.edges.size(); ++i) {
        const auto& e = mMesh.edges[i];
        auto* startItem = new QTableWidgetItem(QString::number(e.startVertex));
        mEdgesTable->setItem(i, 0, startItem);

        auto* endItem = new QTableWidgetItem(QString::number(e.endVertex));
        mEdgesTable->setItem(i, 1, endItem);

        auto* blockedItem = new QTableWidgetItem(e.blocked ? "Yes" : "No");
        mEdgesTable->setItem(i, 2, blockedItem);
    }

    mEdgesTable->resizeColumnsToContents();
    mEdgesTable->blockSignals(false);
}

void NavmeshEditorDialog::refreshPortalsTable()
{
    mPortalsTable->blockSignals(true);
    mPortalsTable->setRowCount(mMesh.portals.size());

    for (int i = 0; i < mMesh.portals.size(); ++i) {
        const auto& p = mMesh.portals[i];
        auto* nameItem = new QTableWidgetItem(p.name);
        mPortalsTable->setItem(i, 0, nameItem);

        auto* triAItem = new QTableWidgetItem(QString::number(p.triangleA));
        mPortalsTable->setItem(i, 1, triAItem);

        auto* triBItem = new QTableWidgetItem(QString::number(p.triangleB));
        mPortalsTable->setItem(i, 2, triBItem);

        auto* widthItem = new QTableWidgetItem(QString::number(p.width, 'f', 2));
        mPortalsTable->setItem(i, 3, widthItem);
    }

    mPortalsTable->resizeColumnsToContents();
    mPortalsTable->blockSignals(false);
}

void NavmeshEditorDialog::onTriangleRowClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row >= 0 && row < mMesh.triangles.size()) {
        emit triangleSelected(row);
    }
}

void NavmeshEditorDialog::onAddVertex()
{
    QVector3D newPos(mVertexX->value(), mVertexY->value(), mVertexZ->value());
    mMesh.vertices.append(newPos);
    refreshVerticesTable();
    mVertexCountLabel->setText(QString("Vertices: %1").arg(mMesh.vertices.size()));
    LOG_INFO(QString("Added vertex at (%1, %2, %3)")
        .arg(newPos.x()).arg(newPos.y()).arg(newPos.z()));
}

void NavmeshEditorDialog::onRemoveVertex()
{
    int row = mVerticesTable->currentRow();
    if (row < 0 || row >= mMesh.vertices.size()) return;

    mMesh.vertices.remove(row);
    refreshVerticesTable();
    mVertexCountLabel->setText(QString("Vertices: %1").arg(mMesh.vertices.size()));
    LOG_INFO(QString("Removed vertex %1").arg(row));
}

void NavmeshEditorDialog::onVertexCellChanged(int row, int column)
{
    if (row < 0 || row >= mMesh.vertices.size()) return;

    bool ok;
    double val = mVerticesTable->item(row, column)->text().toDouble(&ok);
    if (!ok) return;

    auto& v = mMesh.vertices[row];
    if (column == 0) v.setX(val);
    else if (column == 1) v.setY(val);
    else if (column == 2) v.setZ(val);
}

void NavmeshEditorDialog::onAddEdge()
{
    NavEdge edge;
    edge.startVertex = 0;
    edge.endVertex = 0;
    edge.blocked = false;
    mMesh.edges.append(edge);
    refreshEdgesTable();
    mEdgeCountLabel->setText(QString("Edges: %1").arg(mMesh.edges.size()));
    LOG_INFO("Added new edge");
}

void NavmeshEditorDialog::onRemoveEdge()
{
    int row = mEdgesTable->currentRow();
    if (row < 0 || row >= mMesh.edges.size()) return;

    mMesh.edges.remove(row);
    refreshEdgesTable();
    mEdgeCountLabel->setText(QString("Edges: %1").arg(mMesh.edges.size()));
    LOG_INFO(QString("Removed edge %1").arg(row));
}

void NavmeshEditorDialog::onEdgeCellChanged(int row, int column)
{
    if (row < 0 || row >= mMesh.edges.size()) return;

    auto& e = mMesh.edges[row];
    if (column == 0) {
        bool ok;
        int val = mEdgesTable->item(row, column)->text().toInt(&ok);
        if (ok) e.startVertex = val;
    } else if (column == 1) {
        bool ok;
        int val = mEdgesTable->item(row, column)->text().toInt(&ok);
        if (ok) e.endVertex = val;
    } else if (column == 2) {
        e.blocked = (mEdgesTable->item(row, column)->text().toLower() == "yes");
    }
}

void NavmeshEditorDialog::onAddPortal()
{
    NavPortal portal;
    portal.name = QString("Portal_%1").arg(mMesh.portals.size());
    portal.triangleA = 0;
    portal.triangleB = 0;
    portal.width = 1.0f;
    mMesh.portals.append(portal);
    refreshPortalsTable();
    LOG_INFO(QString("Added portal '%1'").arg(portal.name));
}

void NavmeshEditorDialog::onRemovePortal()
{
    int row = mPortalsTable->currentRow();
    if (row < 0 || row >= mMesh.portals.size()) return;

    mMesh.portals.remove(row);
    refreshPortalsTable();
    LOG_INFO(QString("Removed portal %1").arg(row));
}

void NavmeshEditorDialog::onPortalCellChanged(int row, int column)
{
    if (row < 0 || row >= mMesh.portals.size()) return;

    auto& p = mMesh.portals[row];
    if (column == 0) {
        p.name = mPortalsTable->item(row, column)->text();
    } else if (column == 1) {
        bool ok;
        int val = mPortalsTable->item(row, column)->text().toInt(&ok);
        if (ok) p.triangleA = val;
    } else if (column == 2) {
        bool ok;
        int val = mPortalsTable->item(row, column)->text().toInt(&ok);
        if (ok) p.triangleB = val;
    } else if (column == 3) {
        bool ok;
        double val = mPortalsTable->item(row, column)->text().toDouble(&ok);
        if (ok) p.width = static_cast<float>(val);
    }
}

void NavmeshEditorDialog::onFindPath()
{
    QVector3D start(mStartX->value(), mStartY->value(), mStartZ->value());
    QVector3D end(mEndX->value(), mEndY->value(), mEndZ->value());

    mLastPath = findPath(start, end);

    mPathResultList->clear();
    if (mLastPath.isEmpty()) {
        mPathResultList->addItem("No path found");
    } else {
        for (int i = 0; i < mLastPath.size(); ++i) {
            const auto& wp = mLastPath[i];
            mPathResultList->addItem(QString("Waypoint %1: (%2, %3, %4)")
                .arg(i)
                .arg(wp.x(), 0, 'f', 2)
                .arg(wp.y(), 0, 'f', 2)
                .arg(wp.z(), 0, 'f', 2));
        }
    }

    if (mHighlightPathCheck->isChecked()) {
        emit pathChanged(mLastPath);
    }

    LOG_INFO(QString("Pathfinding: %1 waypoints found").arg(mLastPath.size()));
}

void NavmeshEditorDialog::onHighlightPathToggled(bool checked)
{
    if (checked) {
        emit pathChanged(mLastPath);
    } else {
        emit pathChanged(QVector<QVector3D>());
    }
}

QVector<QVector3D> NavmeshEditorDialog::findPath(const QVector3D& start, const QVector3D& end)
{
    int startTri = findContainingTriangle(start);
    int endTri = findContainingTriangle(end);

    if (startTri < 0 || endTri < 0) return QVector<QVector3D>();
    if (startTri == endTri) {
        return QVector<QVector3D>() << start << end;
    }

    QVector<PathNode> nodes(mMesh.triangles.size());
    for (int i = 0; i < mMesh.triangles.size(); ++i) {
        nodes[i].triangleIndex = i;
        nodes[i].g = std::numeric_limits<float>::max();
        nodes[i].f = std::numeric_limits<float>::max();
        nodes[i].parent = -1;
    }

    nodes[startTri].g = 0.0f;
    nodes[startTri].f = euclideanDistance(triangleCenter(mMesh.triangles[startTri]),
                                         triangleCenter(mMesh.triangles[endTri]));

    QVector<bool> closed(mMesh.triangles.size(), false);

    auto compareF = [&nodes](int a, int b) {
        return nodes[a].f < nodes[b].f;
    };

    QVector<int> openSet;
    openSet.append(startTri);

    while (!openSet.isEmpty()) {
        std::sort(openSet.begin(), openSet.end(), compareF);
        int current = openSet.takeFirst();

        if (current == endTri) {
            QVector<QVector3D> path;
            int idx = endTri;
            while (idx != -1) {
                path.prepend(triangleCenter(mMesh.triangles[idx]));
                idx = nodes[idx].parent;
            }
            return path;
        }

        closed[current] = true;

        QVector<int> neighbors = getNeighborTriangles(current);
        for (int neighbor : neighbors) {
            if (closed[neighbor]) continue;
            if (!mMesh.triangles[neighbor].walkable) continue;

            float tentativeG = nodes[current].g +
                euclideanDistance(triangleCenter(mMesh.triangles[current]),
                                 triangleCenter(mMesh.triangles[neighbor]));

            if (tentativeG < nodes[neighbor].g) {
                nodes[neighbor].parent = current;
                nodes[neighbor].g = tentativeG;
                nodes[neighbor].f = tentativeG +
                    euclideanDistance(triangleCenter(mMesh.triangles[neighbor]),
                                     triangleCenter(mMesh.triangles[endTri]));

                if (!openSet.contains(neighbor)) {
                    openSet.append(neighbor);
                }
            }
        }
    }

    return QVector<QVector3D>();
}

int NavmeshEditorDialog::findContainingTriangle(const QVector3D& point) const
{
    float minDist = std::numeric_limits<float>::max();
    int bestTri = -1;

    for (int i = 0; i < mMesh.triangles.size(); ++i) {
        const auto& tri = mMesh.triangles[i];
        if (tri.v0 < 0 || tri.v0 >= mMesh.vertices.size() ||
            tri.v1 < 0 || tri.v1 >= mMesh.vertices.size() ||
            tri.v2 < 0 || tri.v2 >= mMesh.vertices.size()) {
            continue;
        }

        QVector3D center = triangleCenter(tri);
        float dist = euclideanDistance(point, center);
        if (dist < minDist) {
            minDist = dist;
            bestTri = i;
        }
    }

    return bestTri;
}

QVector<int> NavmeshEditorDialog::getNeighborTriangles(int triangleIndex) const
{
    if (triangleIndex < 0 || triangleIndex >= mMesh.triangles.size()) {
        return QVector<int>();
    }

    return mMesh.triangles[triangleIndex].adjacentTriangles;
}

float NavmeshEditorDialog::euclideanDistance(const QVector3D& a, const QVector3D& b) const
{
    float dx = a.x() - b.x();
    float dy = a.y() - b.y();
    float dz = a.z() - b.z();
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

QVector3D NavmeshEditorDialog::triangleCenter(const NavTriangle& tri) const
{
    if (tri.v0 < 0 || tri.v0 >= mMesh.vertices.size() ||
        tri.v1 < 0 || tri.v1 >= mMesh.vertices.size() ||
        tri.v2 < 0 || tri.v2 >= mMesh.vertices.size()) {
        return QVector3D();
    }

    return (mMesh.vertices[tri.v0] + mMesh.vertices[tri.v1] + mMesh.vertices[tri.v2]) / 3.0f;
}
