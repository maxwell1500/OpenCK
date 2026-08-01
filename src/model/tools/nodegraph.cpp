#include "nodegraph.hpp"

#include <cmath>
#include <algorithm>

int NodeGraph::addNode(const QString& type, const QString& label, QPointF pos,
                       int inPorts, int outPorts)
{
    GraphNode node;
    node.id = m_nextNodeId++;
    node.type = type;
    node.label = label;
    node.pos = pos;
    node.inputPorts = qMax(1, inPorts);
    node.outputPorts = qMax(1, outPorts);

    const int index = m_nodes.size();
    m_nodes.append(node);
    m_nodeIndexById.insert(node.id, index);
    return node.id;
}

bool NodeGraph::removeNode(int nodeId)
{
    if (!m_nodeIndexById.contains(nodeId))
        return false;

    // Drop edges touching this node.
    for (int i = m_edges.size() - 1; i >= 0; --i)
    {
        if (m_edges[i].fromNode == nodeId || m_edges[i].toNode == nodeId)
        {
            m_edgeIndexById.remove(m_edges[i].id);
            m_edges.removeAt(i);
        }
    }

    const int index = m_nodeIndexById.take(nodeId);
    m_nodes.removeAt(index);
    // Rebuild the index (ids are stable; only positions shift).
    m_nodeIndexById.clear();
    for (int i = 0; i < m_nodes.size(); ++i)
        m_nodeIndexById.insert(m_nodes[i].id, i);
    return true;
}

int NodeGraph::connect(int fromNode, int fromPort, int toNode, int toPort)
{
    if (!canConnect(fromNode, fromPort, toNode, toPort))
        return -1;

    GraphEdge edge;
    edge.id = m_nextEdgeId++;
    edge.fromNode = fromNode;
    edge.fromPort = fromPort;
    edge.toNode = toNode;
    edge.toPort = toPort;

    const int index = m_edges.size();
    m_edges.append(edge);
    m_edgeIndexById.insert(edge.id, index);
    return edge.id;
}

bool NodeGraph::removeEdge(int edgeId)
{
    if (!m_edgeIndexById.contains(edgeId))
        return false;
    const int index = m_edgeIndexById.take(edgeId);
    m_edges.removeAt(index);
    m_edgeIndexById.clear();
    for (int i = 0; i < m_edges.size(); ++i)
        m_edgeIndexById.insert(m_edges[i].id, i);
    return true;
}

const GraphNode* NodeGraph::node(int id) const
{
    const int index = m_nodeIndexById.value(id, -1);
    return index >= 0 ? &m_nodes[index] : nullptr;
}

const GraphEdge* NodeGraph::edge(int id) const
{
    const int index = m_edgeIndexById.value(id, -1);
    return index >= 0 ? &m_edges[index] : nullptr;
}

int NodeGraph::nodeAt(QPointF pos, double radius) const
{
    int best = -1;
    double bestDist = radius * radius;
    for (const GraphNode& n : m_nodes)
    {
        const double dx = n.pos.x() - pos.x();
        const double dy = n.pos.y() - pos.y();
        const double d2 = dx * dx + dy * dy;
        if (d2 <= bestDist)
        {
            bestDist = d2;
            best = n.id;
        }
    }
    return best;
}

int NodeGraph::nodePortAt(QPointF pos, bool isOutput, double radius,
                          int* portIndex) const
{
    // Ports are laid out vertically along the node's left (input) or right
    // (output) edge, spaced 24 px apart from the node's top.
    for (const GraphNode& n : m_nodes)
    {
        const int count = isOutput ? n.outputPorts : n.inputPorts;
        const double portX = n.pos.x() + (isOutput ? 80.0 : 0.0);
        for (int p = 0; p < count; ++p)
        {
            const double portY = n.pos.y() + 20.0 + p * 24.0;
            const double dx = portX - pos.x();
            const double dy = portY - pos.y();
            if (dx * dx + dy * dy <= radius * radius)
            {
                if (portIndex)
                    *portIndex = p;
                return n.id;
            }
        }
    }
    return -1;
}

bool NodeGraph::canConnect(int fromNode, int fromPort, int toNode, int toPort) const
{
    if (fromNode == toNode)
        return false;
    const GraphNode* from = node(fromNode);
    const GraphNode* to = node(toNode);
    if (!from || !to)
        return false;
    if (fromPort < 0 || fromPort >= from->outputPorts)
        return false;
    if (toPort < 0 || toPort >= to->inputPorts)
        return false;

    // Reject duplicate edges.
    for (const GraphEdge& e : m_edges)
        if (e.fromNode == fromNode && e.fromPort == fromPort
            && e.toNode == toNode && e.toPort == toPort)
            return false;
    return true;
}

bool NodeGraph::hasCycles() const
{
    // Kahn's algorithm: if all nodes can be removed, there is no cycle.
    QMap<int, int> inDegree;
    for (const GraphNode& n : m_nodes)
        inDegree.insert(n.id, 0);
    for (const GraphEdge& e : m_edges)
        if (inDegree.contains(e.toNode))
            inDegree[e.toNode]++;

    QVector<int> ready;
    for (auto it = inDegree.cbegin(); it != inDegree.cend(); ++it)
        if (it.value() == 0)
            ready.append(it.key());

    int visited = 0;
    while (!ready.isEmpty())
    {
        const int id = ready.takeLast();
        ++visited;
        for (const GraphEdge& e : m_edges)
        {
            if (e.fromNode == id && inDegree.contains(e.toNode))
            {
                if (--inDegree[e.toNode] == 0)
                    ready.append(e.toNode);
            }
        }
    }
    return visited != m_nodes.size();
}
