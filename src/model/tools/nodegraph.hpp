#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include <QString>
#include <QPointF>
#include <QVector>
#include <QMap>

// NodeGraph is the data model for the behavior/animation graph editor. A
// graph holds nodes (state-machine / blend-tree blocks) and directed edges
// between node ports. The model is kept separate from the canvas widget so
// the connection logic and hit-testing are unit-testable without a UI.
struct GraphNode
{
    int id = -1;
    QString type;              // e.g. "State_Machine", "Blend_Tree"
    QString label;
    QPointF pos;
    int inputPorts = 1;
    int outputPorts = 1;
};

struct GraphEdge
{
    int id = -1;
    int fromNode = -1;
    int fromPort = 0;
    int toNode = -1;
    int toPort = 0;
};

class NodeGraph
{
public:
    int addNode(const QString& type, const QString& label, QPointF pos,
                int inPorts = 1, int outPorts = 1);
    bool removeNode(int nodeId);

    // Connects fromNode:fromPort -> toNode:toPort. Returns the edge id or -1
    // on failure (missing node, out-of-range port, or self-loop).
    int connect(int fromNode, int fromPort, int toNode, int toPort);
    bool removeEdge(int edgeId);

    int nodeCount() const { return m_nodes.size(); }
    int edgeCount() const { return m_edges.size(); }

    const GraphNode* node(int id) const;
    const GraphEdge* edge(int id) const;
    const QVector<GraphNode>& nodes() const { return m_nodes; }
    const QVector<GraphEdge>& edges() const { return m_edges; }

    // Returns the node at the given position within 'radius' px, or -1.
    int nodeAt(QPointF pos, double radius = 10.0) const;

    // Returns the node whose port (input or output) is near the point.
    // 'isOutput' selects which port row is checked; returns the node id.
    int nodePortAt(QPointF pos, bool isOutput, double radius = 12.0,
                   int* portIndex = nullptr) const;

    // True when connecting fromNode:fromPort -> toNode:toPort is valid.
    bool canConnect(int fromNode, int fromPort, int toNode, int toPort) const;

    // Returns true if the graph has any cycles (for validation).
    bool hasCycles() const;

private:
    QVector<GraphNode> m_nodes;
    QVector<GraphEdge> m_edges;
    int m_nextNodeId = 1;
    int m_nextEdgeId = 1;
    QMap<int, int> m_nodeIndexById;  // id -> index in m_nodes
    QMap<int, int> m_edgeIndexById;
};

#endif // NODEGRAPH_H
