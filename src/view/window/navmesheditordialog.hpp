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

class UndoStack;
struct NavTriangle {
    int v0, v1, v2;
    QVector3D normal;
    bool walkable;
    QVector<int> adjacentTriangles;
};

enum class NavEdgeType {
    Regular = 0,
    Cover = 1,
    WaterBoundary = 2,
    Portal = 3
};

struct NavPortal {
    QString name;
    int triangleA = -1;
    int triangleB = -1;
    float width = 0.0f;
    quint32 doorRefFormId = 0; // linked interior cell door reference (XNDP)
};

struct NavEdge {
    int startVertex = -1;
    int endVertex = -1;
    bool blocked = false;
    NavEdgeType edgeType = NavEdgeType::Regular;
    float coverHeight = 0.0f; // computed or manual cover height
    quint32 linkedDoorRef = 0; // door portal link if edgeType == Portal
};

struct NavMeshData {
    QVector<QVector3D> vertices;
    QVector<NavTriangle> triangles;
    QVector<NavEdge> edges;
    QVector<NavPortal> portals;
    QVector<int> disconnectedIslandTriangles; // tri indices in disconnected components
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
    void setUndoStack(UndoStack* undoStack) { mUndoStack = undoStack; }
    UndoStack* getUndoStack() const { return mUndoStack; }
    // Interactive topology operations
    int addVertex(const QVector3D& pos);
    bool addTriangle(int v0, int v1, int v2);
    bool extrudeEdge(int v0, int v1, const QVector3D& targetPos);
    bool splitEdge(int v0, int v1, const QVector3D& targetPos);
    bool flipEdge(int v0, int v1);

    // Portals & Cover
    void linkDoorPortal(int portalIdx, quint32 doorRefFormId);
    void linkEdgeDoorPortal(int edgeIdx, quint32 doorRefFormId);
    void generateEdgeCover(float minCoverDepth = 128.0f);

    // Reachability
    void updateReachability();

signals:
    void triangleSelected(int index);
    void pathChanged(const QVector<QVector3D>& waypoints);
    void navMeshUpdated(const NavMeshData& mesh);
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
    void onExtrudeEdge();
    void onSplitEdge();
    void onFlipEdge();
    void onGenerateCover();
    void onCheckReachability();
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
    UndoStack* mUndoStack = nullptr;
};

#endif // NAVMESHDITORDIALOG_HPP
