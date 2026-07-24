#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDomDocument>
#include <QTemporaryDir>

class TestDataExporter : public QObject
{
    Q_OBJECT

private slots:
    void testStateString_Base();
    void testStateString_Modified();
    void testStateString_ModifiedOnly();
    void testStateString_Deleted();
    void testStateString_Erased();
};

QString getStateString(State state)
{
    switch (state)
    {
    case State_Base: return "Base";
    case State_Modified: return "Modified";
    case State_ModifiedOnly: return "ModifiedOnly";
    case State_Deleted: return "Deleted";
    case State_Erased: return "Erased";
    }
    return "Unknown";
}

void TestDataExporter::testStateString_Base()
{
    QCOMPARE(getStateString(State_Base), QString("Base"));
}

void TestDataExporter::testStateString_Modified()
{
    QCOMPARE(getStateString(State_Modified), QString("Modified"));
}

void TestDataExporter::testStateString_ModifiedOnly()
{
    QCOMPARE(getStateString(State_ModifiedOnly), QString("ModifiedOnly"));
}

void TestDataExporter::testStateString_Deleted()
{
    QCOMPARE(getStateString(State_Deleted), QString("Deleted"));
}

void TestDataExporter::testStateString_Erased()
{
    QCOMPARE(getStateString(State_Erased), QString("Erased"));
}

QTEST_MAIN(TestDataExporter)
#include "test_dataexporter.moc"
