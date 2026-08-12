#ifndef NODEGRAPHWIDGET_H
#define NODEGRAPHWIDGET_H

#include <QWidget>
#include <QPointF>

#include "../../../model/tools/nodegraph.hpp"

class NodeGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NodeGraphWidget(QWidget* parent = nullptr);

    void setGraph(NodeGraph* graph);

    // Adds a node of the given type at a view position (auto-pans to keep it
    // under the cursor). Returns the new node id.
    int addNode(const QString& type, const QString& label, QPointF viewPos);

    // Prompts for a file and persists / restores the whole graph via the
    // model's JSON serialization.
    void saveToFile(QWidget* parent = nullptr);
    void loadFromFile(QWidget* parent = nullptr);

    // The node palette (Phase 22.2); node types the editor can create.
    static QStringList paletteTypes();

    QSize minimumSizeHint() const override;

signals:
    void nodeAdded(int nodeId);
    void nodeSelected(int nodeId);
    void edgeAdded(int fromNode, int toNode);
    void graphChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QPointF viewToScene(const QPoint& p) const;
    QPoint sceneToView(QPointF p) const;
    QPointF nodeCenter(const GraphNode& n) const;
    void paintEdge(QPainter& p, const GraphEdge& e);
    void paintNode(QPainter& p, const GraphNode& n, bool selected);
    void startConnect(const GraphNode& from, int fromPort);

    NodeGraph* m_graph;
    double m_zoom;
    QPointF m_pan;

    // Interaction state.
    int m_selectedNode;
    int m_dragNode;
    QPoint m_dragStartPos;
    QPointF m_dragNodeOrigin;
    bool m_panning;
    QPoint m_panStart;
    QPointF m_panOrigin;

    // Connection drag.
    bool m_connecting;
    int m_connectFromNode;
    int m_connectFromPort;
    QPoint m_connectCursorPos;
};

#endif // NODEGRAPHWIDGET_H
