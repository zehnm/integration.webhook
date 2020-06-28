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
        placeholders.insert("ROOT", "api/");
        placeholders.insert("HOST", "localhost");
        placeholders.insert("ENDPOINT", "foo");

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
        QTest::newRow("base and cmd URL with placeholders") << "http://${HOST}/${ROOT}"
                                          << "${ENDPOINT}" << placeholders << QUrl("http://localhost/api/foo");
        QTest::newRow("unresolved placeholder") << "http://${HOST}/${ROOT}"
                                          << "${UNKNOWN}" << placeholders << QUrl("http://localhost/api/$%7BUNKNOWN%7D");
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
        placeholders.insert("RED", 8);
        placeholders.insert("GREEN", 15);
        placeholders.insert("BLUE", 240);
        placeholders.insert("PERCENT", QVariant(12.34));

        QTest::newRow("empty text") << "" << placeholders << "";
        QTest::newRow("text without variables") << "foo Bar 123" << placeholders << "foo Bar 123";
        QTest::newRow("single variable") << "${foo} Bar 123" << placeholders << "BAR Bar 123";
        QTest::newRow("multi variables") << "${foo} ${Bar} 123" << placeholders << "BAR FOO 123";
        QTest::newRow("hex number format") << "#${RED:%02X}${GREEN:%02X}${BLUE:%02X}" << placeholders << "#080FF0";
        QTest::newRow("decimal number format") << "#${RED:%d},${GREEN:%d},${BLUE:%d}" << placeholders << "#8,15,240";
        QTest::newRow("decimal zero padded number format") << "#${RED:%03d},${GREEN:%03d},${BLUE:%03d}" << placeholders << "#008,015,240";
        QTest::newRow("decimal space padded number format") << "#${RED:%3d},${GREEN:%3d},${BLUE:%3d}" << placeholders << "#  8, 15,240";

        QTest::newRow("not supported float format") << "${PERCENT:%f}" << placeholders << "${PERCENT:%f}";
    }
    void testResolveVariables();
};

void TestEntityHandler::testBuildUrl() {
    QFETCH(QString, baseUrl);
    QFETCH(QString, cmdUrl);
    QFETCH(QVariantMap, placeholders);
    QFETCH(QUrl, result);

    QString oldCmdUrl(cmdUrl);
    EntityHandlerImpl entityHandler("unitTest", baseUrl);

    QUrl url = entityHandler.buildUrl(cmdUrl, placeholders);

    QCOMPARE(cmdUrl, oldCmdUrl); // input url may not be changed
    QCOMPARE(url, result);
}

void TestEntityHandler::testResolveVariables() {
    QFETCH(QString, text);
    QFETCH(QVariantMap, placeholders);
    QFETCH(QString, result);

    QString oldText(text);
    EntityHandlerImpl entityHandler("unitTest", "");

    QString resolvedText = entityHandler.resolveVariables(text, placeholders);

    QCOMPARE(text, oldText); // input text may not be changed
    QCOMPARE(resolvedText, result);
}

QTEST_GUILESS_MAIN(TestEntityHandler)
#include "tst_entityhandler.moc"
