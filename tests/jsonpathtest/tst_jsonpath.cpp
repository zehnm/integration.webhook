#include <QFile>
#include <QJsonDocument>
#include <QtTest>

#include "jsonpath.h"

class TestJsonPath : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();

    void testEmptyDocument();
    void testRootObject();
    void testChildObject();
    void testNestedObject();
    void testIndex();

    void testMyStromBulbColorResponse();
    void testMyStromSwitchReportResponse();

 private:
    QJsonDocument load(const QString &resource);
    QJsonDocument m_jsonDoc;
};

void TestJsonPath::initTestCase() {
    m_jsonDoc = load(":/testdata/JsonPath.json");
}

void TestJsonPath::testEmptyDocument() {
    JsonPath jsonPath(QJsonDocument::fromVariant(QVariant()));

    QCOMPARE(QVariant(), jsonPath.value("store.electronics[2].title"));
}

void TestJsonPath::testRootObject() {
    JsonPath jsonPath(m_jsonDoc);

    QVariant result = jsonPath.value("store");
    QVERIFY(result.isValid());
    QVERIFY(result.type() == QVariant::Map);
    QVERIFY(result.toMap().contains("electronics"));
    QVERIFY(result.toMap().contains("snack"));
}

void TestJsonPath::testChildObject() {
    JsonPath jsonPath(m_jsonDoc);

    QVariant result = jsonPath.value("store.snack");
    QVERIFY(result.isValid());
    QVERIFY(result.type() == QVariant::Map);
    QVERIFY(result.toMap().contains("title"));
    QVERIFY(result.toMap().contains("price"));
}

void TestJsonPath::testNestedObject() {
    JsonPath jsonPath(m_jsonDoc);

    QVariant result = jsonPath.value("store.snack.price");
    QVERIFY(result.isValid());
    QCOMPARE(QVariant(12.95), result);
}

void TestJsonPath::testIndex() {
    JsonPath jsonPath(m_jsonDoc);

    QCOMPARE("Ryzen 9 3950XT", jsonPath.value("store.electronics[1].title"));
}

void TestJsonPath::testMyStromBulbColorResponse() {
    JsonPath jsonPath(load(":/testdata/myStrom-bulb-setcolor-response.json"));

    QVariant result = jsonPath.value("6001942C4FDD.color");
    QVERIFY(result.isValid());
    QCOMPARE("FFFF0000", result);
}

void TestJsonPath::testMyStromSwitchReportResponse() {
    JsonPath jsonPath(load(":/testdata/myStrom-switch-report-response.json"));

    QVariant result = jsonPath.value("relay");
    QVERIFY(result.isValid());
    QCOMPARE(true, result);
}

QJsonDocument TestJsonPath::load(const QString &resource) {
    QFile file(resource);
    file.open(QFile::OpenModeFlag::ReadOnly);
    QByteArray ba = file.readAll();
    return QJsonDocument::fromJson(ba);
}

QTEST_GUILESS_MAIN(TestJsonPath)
#include "tst_jsonpath.moc"
