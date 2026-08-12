#include "nodegraph.hpp"

#include <cmath>
#include <algorithm>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

QJsonObject NodeGraph::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;

    QJsonArray nodes;
    for (const GraphNode& n : m_nodes)
    {
        QJsonObject o;
        o[QStringLiteral("id")] = n.id;
        o[QStringLiteral("type")] = n.type;
        o[QStringLiteral("label")] = n.label;
        o[QStringLiteral("x")] = n.pos.x();
        o[QStringLiteral("y")] = n.pos.y();
        o[QStringLiteral("inPorts")] = n.inputPorts;
        o[QStringLiteral("outPorts")] = n.outputPorts;
        nodes.append(o);
    }
    root[QStringLiteral("nodes")] = nodes;

    QJsonArray edges;
    for (const GraphEdge& e : m_edges)
    {
        QJsonObject o;
        o[QStringLiteral("id")] = e.id;
        o[QStringLiteral("fromNode")] = e.fromNode;
        o[QStringLiteral("fromPort")] = e.fromPort;
        o[QStringLiteral("toNode")] = e.toNode;
        o[QStringLiteral("toPort")] = e.toPort;
        edges.append(o);
    }
    root[QStringLiteral("edges")] = edges;
    return root;
}

bool NodeGraph::save(const QString& filePath, QString* error) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (error)
            *error = QStringLiteral("Could not write %1").arg(filePath);
        return false;
    }
    file.write(QJsonDocument(toJson()).toJson());
    return true;
}

bool NodeGraph::load(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = QStringLiteral("Could not read %1").arg(filePath);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        if (error)
            *error = QStringLiteral("Invalid graph file: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("version")).toInt(1) > 1)
    {
        if (error)
            *error = QStringLiteral("Unsupported graph file version");
        return false;
    }
    if (!root.contains(QStringLiteral("nodes")))
    {
        if (error)
            *error = QStringLiteral("File is not a behavior graph");
        return false;
    }

    // Parse into staging copies and only swap the live graph in once the whole
    // payload validates, so a bad file cannot leave a partial graph behind.
    QVector<GraphNode> nodes;
    const QJsonArray nodeArr = root.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& v : nodeArr)
    {
        const QJsonObject o = v.toObject();
        GraphNode n;
        n.id = o.value(QStringLiteral("id")).toInt(-1);
        if (n.id <= 0)
            continue;
        n.type = o.value(QStringLiteral("type")).toString();
        n.label = o.value(QStringLiteral("label")).toString();
        n.pos = QPointF(o.value(QStringLiteral("x")).toDouble(),
                        o.value(QStringLiteral("y")).toDouble());
        n.inputPorts = qMax(1, o.value(QStringLiteral("inPorts")).toInt(1));
        n.outputPorts = qMax(1, o.value(QStringLiteral("outPorts")).toInt(1));
        nodes.append(n);
    }

    QMap<int, int> nodeIdx;
    int nextNode = 1;
    for (int i = 0; i < nodes.size(); ++i)
    {
        nodeIdx.insert(nodes[i].id, i);
        nextNode = qMax(nextNode, nodes[i].id + 1);
    }

    QVector<GraphEdge> edges;
    QMap<int, int> edgeIdx;
    int nextEdge = 1;
    const QJsonArray edgeArr = root.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue& v : edgeArr)
    {
        const QJsonObject o = v.toObject();
        GraphEdge e;
        e.id = o.value(QStringLiteral("id")).toInt(-1);
        e.fromNode = o.value(QStringLiteral("fromNode")).toInt(-1);
        e.fromPort = o.value(QStringLiteral("fromPort")).toInt(0);
        e.toNode = o.value(QStringLiteral("toNode")).toInt(-1);
        e.toPort = o.value(QStringLiteral("toPort")).toInt(0);
        if (e.id <= 0 || e.fromNode == e.toNode
            || !nodeIdx.contains(e.fromNode) || !nodeIdx.contains(e.toNode)
            || edgeIdx.contains(e.id))
            continue;
        const GraphNode& from = nodes[nodeIdx.value(e.fromNode)];
        const GraphNode& to = nodes[nodeIdx.value(e.toNode)];
        if (e.fromPort < 0 || e.fromPort >= from.outputPorts)
            continue;
        if (e.toPort < 0 || e.toPort >= to.inputPorts)
            continue;
        const int index = edges.size();
        edges.append(e);
        edgeIdx.insert(e.id, index);
        nextEdge = qMax(nextEdge, e.id + 1);
    }

    m_nodes = nodes;
    m_edges = edges;
    m_nodeIndexById = nodeIdx;
    m_edgeIndexById = edgeIdx;
    m_nextNodeId = nextNode;
    m_nextEdgeId = nextEdge;
    return true;
}
