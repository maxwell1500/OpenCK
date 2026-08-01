#include <QTest>

#include "../../src/model/tools/nodegraph.hpp"

class TestNodeGraph : public QObject
{
    Q_OBJECT

private slots:
    void testAddAndFindNodes();
    void testConnect();
    void testRejectSelfLoopAndDuplicates();
    void testRemoveNodeDropsEdges();
    void testNodeAt();
    void testNodePortAt();
    void testHasCycles();
};

void TestNodeGraph::testAddAndFindNodes()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("State_Machine"), QStringLiteral("SMA"), QPointF(10, 20));
    const int b = graph.addNode(QStringLiteral("Blend_Tree"), QStringLiteral("BT"), QPointF(300, 20));
    QCOMPARE(graph.nodeCount(), 2);
    QVERIFY(graph.node(a) != nullptr);
    QCOMPARE(graph.node(a)->type, QStringLiteral("State_Machine"));
    QCOMPARE(graph.node(a)->label, QStringLiteral("SMA"));
    QCOMPARE(graph.node(a)->pos, QPointF(10, 20));
    QCOMPARE(graph.node(b)->inputPorts, 1);
    QCOMPARE(graph.node(b)->outputPorts, 1);
    QVERIFY(graph.node(999) == nullptr);
}

void TestNodeGraph::testConnect()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(0, 0));
    const int b = graph.addNode(QStringLiteral("B"), QStringLiteral("B"), QPointF(200, 0));
    const int edge = graph.connect(a, 0, b, 0);
    QVERIFY(edge > 0);
    QCOMPARE(graph.edgeCount(), 1);
    QVERIFY(graph.edge(edge) != nullptr);
    QCOMPARE(graph.edge(edge)->fromNode, a);
    QCOMPARE(graph.edge(edge)->toNode, b);
    QCOMPARE(graph.edge(edge)->fromPort, 0);
}

void TestNodeGraph::testRejectSelfLoopAndDuplicates()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(0, 0));
    const int b = graph.addNode(QStringLiteral("B"), QStringLiteral("B"), QPointF(200, 0));

    QCOMPARE(graph.connect(a, 0, a, 0), -1);          // self-loop
    QVERIFY(graph.connect(a, 0, b, 0) > 0);
    QCOMPARE(graph.connect(a, 0, b, 0), -1);          // duplicate
    QCOMPARE(graph.connect(a, 5, b, 0), -1);          // bad output port
    QCOMPARE(graph.connect(a, 0, b, 5), -1);          // bad input port
    QCOMPARE(graph.connect(a, 0, 999, 0), -1);        // missing node
}

void TestNodeGraph::testRemoveNodeDropsEdges()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(0, 0));
    const int b = graph.addNode(QStringLiteral("B"), QStringLiteral("B"), QPointF(200, 0));
    const int c = graph.addNode(QStringLiteral("C"), QStringLiteral("C"), QPointF(400, 0));
    graph.connect(a, 0, b, 0);
    graph.connect(b, 0, c, 0);

    QVERIFY(graph.removeNode(b));
    QCOMPARE(graph.nodeCount(), 2);
    QCOMPARE(graph.edgeCount(), 0);  // both edges touched b
    QVERIFY(!graph.removeNode(999));
}

void TestNodeGraph::testNodeAt()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(50, 60));
    QCOMPARE(graph.nodeAt(QPointF(55, 65), 10.0), a);
    QCOMPARE(graph.nodeAt(QPointF(200, 200), 10.0), -1);
}

void TestNodeGraph::testNodePortAt()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(0, 0), 1, 2);

    // Output port 1 is at (80, 20 + 24).
    int port = -1;
    const int hit = graph.nodePortAt(QPointF(80, 44), true, 12.0, &port);
    QCOMPARE(hit, a);
    QCOMPARE(port, 1);

    // Input port 0 is at (0, 20).
    int inPort = -1;
    const int inHit = graph.nodePortAt(QPointF(0, 20), false, 12.0, &inPort);
    QCOMPARE(inHit, a);
    QCOMPARE(inPort, 0);

    QCOMPARE(graph.nodePortAt(QPointF(500, 500), true), -1);
}

void TestNodeGraph::testHasCycles()
{
    NodeGraph graph;
    const int a = graph.addNode(QStringLiteral("A"), QStringLiteral("A"), QPointF(0, 0));
    const int b = graph.addNode(QStringLiteral("B"), QStringLiteral("B"), QPointF(200, 0));
    const int c = graph.addNode(QStringLiteral("C"), QStringLiteral("C"), QPointF(400, 0));

    graph.connect(a, 0, b, 0);
    graph.connect(b, 0, c, 0);
    QVERIFY(!graph.hasCycles());

    graph.connect(c, 0, a, 0);
    QVERIFY(graph.hasCycles());
}

QTEST_MAIN(TestNodeGraph)
#include "test_nodegraph.moc"
