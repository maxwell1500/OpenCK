#include "nodegraphwidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>

namespace {
constexpr int kNodeWidth = 90;
constexpr int kNodeHeader = 18;
constexpr int kPortSpacing = 24;
constexpr int kPortRadius = 5;

const QColor kNodeFill(46, 52, 64);
const QColor kNodeBorder(94, 110, 140);
const QColor kPortIn(96, 188, 96);
const QColor kPortOut(232, 148, 60);
const QColor kEdge(120, 160, 200);
const QColor kEdgeHot(240, 120, 60);
}

NodeGraphWidget::NodeGraphWidget(QWidget* parent)
    : QWidget(parent)
    , m_graph(nullptr)
    , m_zoom(1.0)
    , m_pan(40, 40)
    , m_selectedNode(-1)
    , m_dragNode(-1)
    , m_panning(false)
    , m_connecting(false)
    , m_connectFromNode(-1)
    , m_connectFromPort(0)
{
    setMinimumSize(480, 320);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void NodeGraphWidget::setGraph(NodeGraph* graph)
{
    m_graph = graph;
    m_selectedNode = -1;
    update();
}

int NodeGraphWidget::addNode(const QString& type, const QString& label,
                             QPointF viewPos)
{
    if (!m_graph)
        return -1;
    // Place the node near the cursor (view -> scene).
    const QPointF scene = viewToScene(viewPos.toPoint());
    const int id = m_graph->addNode(type, label, scene, 1, 1);
    m_selectedNode = id;
    emit nodeAdded(id);
    emit graphChanged();
    update();
    return id;
}

QStringList NodeGraphWidget::paletteTypes()
{
    return {
        QStringLiteral("State_Machine"),
        QStringLiteral("Blend_Tree"),
        QStringLiteral("Blend"),
        QStringLiteral("Merge"),
        QStringLiteral("Switch"),
        QStringLiteral("Animation"),
        QStringLiteral("Locomotion_Blend"),
        QStringLiteral("Random_Animation"),
        QStringLiteral("Timer_Event"),
        QStringLiteral("Two_Bone_IK"),
        QStringLiteral("Look_At"),
        QStringLiteral("Direct_At"),
        QStringLiteral("Foot_IK"),
        QStringLiteral("Momentum_Animation"),
        QStringLiteral("Ragdoll_Get_Up"),
        QStringLiteral("Assign_Variable"),
        QStringLiteral("State_Variable_Control"),
        QStringLiteral("Dampen_Variable"),
        QStringLiteral("Linear_Variable"),
        QStringLiteral("Rotation_Variable"),
    };
}

QSize NodeGraphWidget::minimumSizeHint() const
{
    return QSize(480, 320);
}

QPointF NodeGraphWidget::viewToScene(const QPoint& p) const
{
    return QPointF((p.x() - m_pan.x()) / m_zoom, (p.y() - m_pan.y()) / m_zoom);
}

QPoint NodeGraphWidget::sceneToView(QPointF p) const
{
    return QPoint(static_cast<int>(p.x() * m_zoom + m_pan.x()),
                  static_cast<int>(p.y() * m_zoom + m_pan.y()));
}

QPointF NodeGraphWidget::nodeCenter(const GraphNode& n) const
{
    return QPointF(n.pos.x() + kNodeWidth / 2.0,
                   n.pos.y() + kNodeHeader + n.inputPorts * kPortSpacing / 2.0);
}

void NodeGraphWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 26, 30));

    // Grid.
    p.setPen(QColor(38, 42, 50));
    const int gridStep = 24;
    for (int x = static_cast<int>(m_pan.x()) % gridStep; x < width(); x += gridStep)
        p.drawLine(x, 0, x, height());
    for (int y = static_cast<int>(m_pan.y()) % gridStep; y < height(); y += gridStep)
        p.drawLine(0, y, width(), y);

    if (!m_graph)
        return;

    // Edges first (under nodes).
    p.setRenderHint(QPainter::Antialiasing);
    for (const GraphEdge& e : m_graph->edges())
        paintEdge(p, e);

    // In-progress connection.
    if (m_connecting)
    {
        const GraphNode* from = m_graph->node(m_connectFromNode);
        if (from)
        {
            const QPointF start(nodeCenter(*from).x() + kNodeWidth / 2.0,
                                from->pos.y() + 20.0 + m_connectFromPort * kPortSpacing);
            const QPoint startV = sceneToView(start);
            p.setPen(QPen(kEdgeHot, 2));
            p.drawLine(startV, m_connectCursorPos);
        }
    }

    // Nodes.
    for (const GraphNode& n : m_graph->nodes())
        paintNode(p, n, n.id == m_selectedNode);
}

void NodeGraphWidget::paintEdge(QPainter& p, const GraphEdge& e)
{
    const GraphNode* from = m_graph->node(e.fromNode);
    const GraphNode* to = m_graph->node(e.toNode);
    if (!from || !to)
        return;

    const QPointF out(nodeCenter(*from).x() + kNodeWidth / 2.0,
                      from->pos.y() + 20.0 + e.fromPort * kPortSpacing);
    const QPointF in(nodeCenter(*to).x(),
                     to->pos.y() + 20.0 + e.toPort * kPortSpacing);

    const QPoint outV = sceneToView(out);
    const QPoint inV = sceneToView(in);

    // Cubic bezier with horizontal control points for clean routing.
    const int dx = qAbs(outV.x() - inV.x()) / 2;
    QPainterPath path(outV);
    path.cubicTo(QPointF(outV.x() + dx, outV.y()),
                 QPointF(inV.x() - dx, inV.y()),
                 QPointF(inV));
    p.setPen(QPen(kEdge, 2));
    p.drawPath(path);

    // Direction arrowhead.
    QPainterPath tip;
    tip.moveTo(inV.x() - 5, inV.y() - 4);
    tip.lineTo(inV.x(), inV.y());
    tip.lineTo(inV.x() - 5, inV.y() + 4);
    p.setBrush(kEdge);
    p.setPen(Qt::NoPen);
    p.drawPath(tip);
}

void NodeGraphWidget::paintNode(QPainter& p, const GraphNode& n, bool selected)
{
    const QPoint topLeft = sceneToView(n.pos);
    const int w = static_cast<int>(kNodeWidth * m_zoom);
    const int header = static_cast<int>(kNodeHeader * m_zoom);
    const int portGap = static_cast<int>(kPortSpacing * m_zoom);
    const int bodyH = static_cast<int>(n.inputPorts * kPortSpacing * m_zoom);

    // Header.
    p.setPen(Qt::NoPen);
    p.setBrush(selected ? kNodeBorder.lighter(130) : kNodeBorder);
    p.drawRect(topLeft.x(), topLeft.y(), w, header);
    // Body.
    p.setBrush(kNodeFill);
    p.drawRect(topLeft.x(), topLeft.y() + header, w, bodyH);

    p.setPen(Qt::white);
    p.setFont(QFont(font().family(), 8));
    p.drawText(QRect(topLeft.x(), topLeft.y(), w, header),
               Qt::AlignCenter, n.type);

    // Ports.
    const int nIn = qMax(1, n.inputPorts);
    const int nOut = qMax(1, n.outputPorts);
    for (int i = 0; i < nIn; ++i)
    {
        const int cy = topLeft.y() + header + i * portGap + portGap / 2;
        p.setBrush(kPortIn);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(topLeft.x(), cy), static_cast<int>(kPortRadius * m_zoom),
                      static_cast<int>(kPortRadius * m_zoom));
    }
    for (int i = 0; i < nOut; ++i)
    {
        const int cy = topLeft.y() + header + i * portGap + portGap / 2;
        p.setBrush(kPortOut);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(topLeft.x() + w, cy), static_cast<int>(kPortRadius * m_zoom),
                      static_cast<int>(kPortRadius * m_zoom));
    }

    // Label under the header.
    p.setPen(QColor(200, 205, 215));
    p.drawText(QRect(topLeft.x(), topLeft.y() + header + 2, w, 14),
               Qt::AlignHCenter | Qt::AlignTop, n.label);
}

void NodeGraphWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_graph)
        return;

    const QPointF scene = viewToScene(event->pos());

    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton
            && event->modifiers() & Qt::AltModifier))
    {
        m_panning = true;
        m_panStart = event->pos();
        m_panOrigin = m_pan;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        // Output port drag starts a connection.
        int port = -1;
        const int outNode = m_graph->nodePortAt(scene, true, 12.0, &port);
        if (outNode >= 0)
        {
            startConnect(*m_graph->node(outNode), port);
            m_connectCursorPos = event->pos();
            setCursor(Qt::CrossCursor);
            return;
        }

        const int nodeId = m_graph->nodeAt(scene, 10.0);
        if (nodeId >= 0)
        {
            m_selectedNode = nodeId;
            m_dragNode = nodeId;
            m_dragStartPos = event->pos();
            m_dragNodeOrigin = m_graph->node(nodeId)->pos;
            emit nodeSelected(nodeId);
            update();
            return;
        }

        m_selectedNode = -1;
        update();
    }
}

void NodeGraphWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning)
    {
        m_pan = m_panOrigin + QPointF(event->pos() - m_panStart);
        update();
        return;
    }

    if (m_connecting)
    {
        m_connectCursorPos = event->pos();
        update();
        return;
    }

    if (m_dragNode >= 0 && m_graph)
    {
        const QPointF scene = viewToScene(event->pos());
        const QPointF delta = scene - viewToScene(m_dragStartPos);
        GraphNode* n = const_cast<GraphNode*>(m_graph->node(m_dragNode));
        if (n)
        {
            n->pos = m_dragNodeOrigin + delta;
            emit graphChanged();
            update();
        }
    }
}

void NodeGraphWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning)
    {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (m_connecting)
    {
        const QPointF scene = viewToScene(event->pos());
        int port = -1;
        const int inNode = m_graph ? m_graph->nodePortAt(scene, false, 12.0, &port) : -1;
        if (inNode >= 0)
        {
            const int edge = m_graph->connect(m_connectFromNode, m_connectFromPort,
                                              inNode, port);
            if (edge >= 0)
            {
                emit edgeAdded(m_connectFromNode, inNode);
                emit graphChanged();
            }
        }
        m_connecting = false;
        m_connectFromNode = -1;
        setCursor(Qt::ArrowCursor);
        update();
        return;
    }

    if (m_dragNode >= 0)
    {
        m_dragNode = -1;
    }
}

void NodeGraphWidget::wheelEvent(QWheelEvent* event)
{
    const double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    const double newZoom = qBound(0.25, m_zoom * factor, 3.0);

    // Zoom about the cursor.
    const QPointF before = viewToScene(event->position().toPoint());
    m_zoom = newZoom;
    const QPointF after = viewToScene(event->position().toPoint());
    m_pan += QPointF(after.x() - before.x(), after.y() - before.y()) * m_zoom;
    update();
}

void NodeGraphWidget::startConnect(const GraphNode& from, int fromPort)
{
    m_connecting = true;
    m_connectFromNode = from.id;
    m_connectFromPort = fromPort;
}
