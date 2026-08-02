#ifndef NAVMESHDITORDIALOG_HPP
#define NAVMESHDITORDIALOG_HPP

#include <QDialog>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QCheckBox>
#include <QVector3D>
#include <QVector>
#include <QString>
#include "../../model/tools/navmeshtoolkit.hpp"

struct NavTriangle {
    int v0, v1, v2;
    QVector3D normal;
    bool walkable;
    QVector<int> adjacentTriangles;
};

struct NavPortal {
    QString name;
    int triangleA;
    int triangleB;
    float width;
};

struct NavEdge {
    int startVertex;
    int endVertex;
    bool blocked;
};

struct NavMeshData {
    QVector<QVector3D> vertices;
    QVector<NavTriangle> triangles;
    QVector<NavEdge> edges;
    QVector<NavPortal> portals;
};

struct PathNode {
    int triangleIndex;
    float g;
    float f;
    int parent;
};

class NavmeshEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NavmeshEditorDialog(QWidget* parent = nullptr);
    ~NavmeshEditorDialog();

    void setNavMesh(const NavMeshData& mesh);
    NavMeshData getNavMesh() const;

signals:
    void triangleSelected(int index);
    void pathChanged(const QVector<QVector3D>& waypoints);

private slots:
    void onTriangleRowClicked(int row, int column);
    void onAddVertex();
    void onRemoveVertex();
    void onVertexCellChanged(int row, int column);
    void onAddEdge();
    void onRemoveEdge();
    void onEdgeCellChanged(int row, int column);
    void onAddPortal();
    void onRemovePortal();
    void onPortalCellChanged(int row, int column);
    void onFindPath();
    void onHighlightPathToggled(bool checked);
    void onCheckMesh();
    void onCleanMesh();
    void onWeldVertices();

private:
    void setupUI();
    void setupInfoPanel(QSplitter* splitter);
    void setupEditingTools(QSplitter* splitter);
    void refreshInfoPanel();
    void refreshVerticesTable();
    void refreshEdgesTable();
    void refreshPortalsTable();
    void refreshAdjacency();
    void showCheckResults(const QVector<QString>& lines);
    void convertToToolkit(QVector<QVector3D>& verts,
                          QVector<::NavMeshTools::MeshTriangle>& tris) const;
    void convertFromToolkit(const QVector<QVector3D>& verts,
                            const QVector<::NavMeshTools::MeshTriangle>& tris);
    void refreshAllTables();

    QVector<QVector3D> findPath(const QVector3D& start, const QVector3D& end);
    int findContainingTriangle(const QVector3D& point) const;
    QVector<int> getNeighborTriangles(int triangleIndex) const;
    float euclideanDistance(const QVector3D& a, const QVector3D& b) const;
    QVector3D triangleCenter(const NavTriangle& tri) const;

    NavMeshData mMesh;

    QLabel* mTriangleCountLabel;
    QLabel* mVertexCountLabel;
    QLabel* mEdgeCountLabel;
    QTableWidget* mTriangleTable;

    QTableWidget* mVerticesTable;
    QPushButton* mAddVertexButton;
    QPushButton* mRemoveVertexButton;
    QDoubleSpinBox* mVertexX;
    QDoubleSpinBox* mVertexY;
    QDoubleSpinBox* mVertexZ;

    QTableWidget* mEdgesTable;
    QPushButton* mAddEdgeButton;
    QPushButton* mRemoveEdgeButton;
    QCheckBox* mEdgeBlockedCheck;

    QTableWidget* mPortalsTable;
    QPushButton* mAddPortalButton;
    QPushButton* mRemovePortalButton;

    QDoubleSpinBox* mStartX;
    QDoubleSpinBox* mStartY;
    QDoubleSpinBox* mStartZ;
    QDoubleSpinBox* mEndX;
    QDoubleSpinBox* mEndY;
    QDoubleSpinBox* mEndZ;
    QPushButton* mFindPathButton;
    QListWidget* mPathResultList;
    QCheckBox* mHighlightPathCheck;
    QListWidget* mCheckResultList;
    QPushButton* mCheckButton;
    QPushButton* mCleanButton;
    QPushButton* mWeldButton;

    QVector<QVector3D> mLastPath;
};

#endif // NAVMESHDITORDIALOG_HPP
