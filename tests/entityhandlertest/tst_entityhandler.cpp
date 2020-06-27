#include <QFile>
#include <QJsonDocument>
#include <QtTest>

#include "entityhandlerimpl.h"
#include "jsonpath.h"

class TestEntityHandler : public QObject {
    Q_OBJECT

 private slots:
    void testBuildUrl_data() {
        QTest::addColumn<QString>("baseUrl");
        QTest::addColumn<QString>("cmdUrl");
        QTest::addColumn<QVariantMap>("placeholders");
        QTest::addColumn<QUrl>("result");

        QVariantMap placeholders;

        QTest::newRow("empty base URL") << ""
                                        << "http://localhost/api/" << placeholders << QUrl("http://localhost/api/");
        QTest::newRow("empty cmd URL") << "http://localhost/api/"
                                       << "" << placeholders << QUrl("http://localhost/api/");
        QTest::newRow("base and cmd URL") << "http://localhost/api/"
                                          << "test" << placeholders << QUrl("http://localhost/api/test");
        QTest::newRow("base and root cmd URL") << "http://localhost/api/"
                                               << "/test" << placeholders << QUrl("http://localhost/test");
        QTest::newRow("base and relative cmd URL") << "http://localhost/api/"
                                                   << "../test" << placeholders << QUrl("http://localhost/test");
    }
    void testBuildUrl();

    void testResolveVariables_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QVariantMap>("placeholders");
        QTest::addColumn<QString>("result");

        QVariantMap placeholders;
        placeholders.insert("foo", "BAR");
        placeholders.insert("bar", "foo");
        placeholders.insert("Bar", "FOO");

        QTest::newRow("empty text") << "" << placeholders << "";
        QTest::newRow("text without variables") << "foo Bar 123" << placeholders << "foo Bar 123";
        QTest::newRow("single variable") << "${foo} Bar 123" << placeholders << "BAR Bar 123";
        QTest::newRow("multi variables") << "${foo} ${Bar} 123" << placeholders << "BAR FOO 123";
    }
    void testResolveVariables();
};

void TestEntityHandler::testBuildUrl() {
    QFETCH(QString, baseUrl);
    QFETCH(QString, cmdUrl);
    QFETCH(QVariantMap, placeholders);
    QFETCH(QUrl, result);

    EntityHandlerImpl entityHandler("unitTest", baseUrl);

    QUrl url = entityHandler.buildUrl(cmdUrl, placeholders);

    QCOMPARE(url, result);
}

void TestEntityHandler::testResolveVariables() {
    QFETCH(QString, text);
    QFETCH(QVariantMap, placeholders);
    QFETCH(QString, result);

    EntityHandlerImpl entityHandler("unitTest", "");

    QString resolvedText = entityHandler.resolveVariables(text, placeholders);

    QCOMPARE(resolvedText, result);
}

QTEST_GUILESS_MAIN(TestEntityHandler)
#include "tst_entityhandler.moc"
