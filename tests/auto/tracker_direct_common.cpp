#include "tracker_direct_common.h"
#include "testhelpers.h"
#include <QtTest/QtTest>
#include <QtSparql/QtSparql>

namespace {
    void myMessageOutput(QtMsgType type, const char *msg)
    {
        switch (type) {
            case QtDebugMsg:
                if (testLogLevel <= 0)
                    fprintf(stderr, "QDEBUG : %s\n", msg);
                break;
            case QtWarningMsg:
                if (testLogLevel <= 1)
                    fprintf(stderr, "QWARN  : %s\n", msg);
                break;
            case QtCriticalMsg:
                if (testLogLevel <= 2)
                    fprintf(stderr, "QCRITICAL: %s\n", msg);
                break;
            case QtFatalMsg:
                if (testLogLevel <= 3)
                    fprintf(stderr, "QFATAL : %s\n", msg);
                abort();
        }
    }
} // end unnamed namespace


TrackerDirectCommon::TrackerDirectCommon()
{
}

TrackerDirectCommon::~TrackerDirectCommon()
{
}

QSparqlResult* TrackerDirectCommon::runQuery(QSparqlConnection &conn, const QSparqlQuery &q)
{
    QSparqlResult* r = execQuery(conn, q);
    if (!r) {
        qWarning() << "execQuery() returned empty result";
        return 0;
    }
    waitForQueryFinished(r);
    if (r->hasError()) {
        qWarning() << "QSparqlResult resulted with error: " << r->lastError().message();
        return 0;
    }
    return r;
}

void TrackerDirectCommon::installMsgHandler()
{
    qInstallMsgHandler(myMessageOutput);
}

void TrackerDirectCommon::query_contacts()
{
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery q("select ?u ?ng {?u a nco:PersonContact; "
                   "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                   "nco:nameGiven ?ng .}");
    QSparqlResult* r = runQuery(conn, q);
    QVERIFY(r != 0);
    QVERIFY(checkResultSize(r, 3));
    QHash<QString, QString> contactNames;
    while (r->next()) {
        QCOMPARE(r->current().count(), 2);
        QCOMPARE(r->stringValue(0), r->value(0).toString());
        QCOMPARE(r->stringValue(1), r->value(1).toString());

        contactNames[r->value(0).toString()] = r->value(1).toString();
    }
    QVERIFY(r->isFinished());
    QCOMPARE(contactNames.size(), 3);
    QCOMPARE(contactNames["uri001"], QString("name001"));
    QCOMPARE(contactNames["uri002"], QString("name002"));
    QCOMPARE(contactNames["uri003"], QString("name003"));

    CHECK_ERROR(r);
    delete r;
}
void TrackerDirectCommon::insert_and_delete_contact()
{

    // This test will leave unclean test data in tracker if it crashes.
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery add("insert { <addeduri001> a nco:PersonContact; "
                     "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                     "nco:nameGiven \"addedname001\" .}",
                     QSparqlQuery::InsertStatement);

    QSparqlResult* r = runQuery(conn, add);
    QVERIFY(r!=0);
    QVERIFY(r->isFinished());
    delete r;

    // Verify that the insertion succeeded
    QSparqlQuery q("select ?u ?ng {?u a nco:PersonContact; "
                   "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                   "nco:nameGiven ?ng .}");
    QHash<QString, QString> contactNames;
    r = runQuery(conn, q);
    QVERIFY(r!=0);

    QVERIFY(checkResultSize(r, 4));
    while (r->next()) {
        contactNames[r->value(0).toString()] =
            r->value(1).toString();
    }
    QVERIFY(r->isFinished());
    CHECK_ERROR(r);
    QCOMPARE(contactNames.size(), 4);
    QCOMPARE(contactNames["addeduri001"], QString("addedname001"));
    delete r;

    // Delete the uri
    QSparqlQuery del("delete { <addeduri001> a rdfs:Resource. }",
                     QSparqlQuery::DeleteStatement);

    r = runQuery(conn, del);
    QVERIFY(r!=0);
    QVERIFY(r->isFinished());
    delete r;

    // Verify that it got deleted
    contactNames.clear();
    r = runQuery(conn, q);
    QVERIFY(r!=0);
    QVERIFY(checkResultSize(r, 3));
    while (r->next()) {
        // A different way for retrieving the results
        contactNames[r->binding(0).value().toString()] =
            r->binding(1).value().toString();
    }
    QVERIFY(r->isFinished());
    QCOMPARE(contactNames.size(), 3);
    delete r;
}

void TrackerDirectCommon::query_with_error()
{
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery q("this is not a valid query");
    QSparqlResult* r = execQuery(conn, q);
    QVERIFY(r!=0);
    waitForQueryFinished(r);
    QVERIFY(r->isFinished());
    QVERIFY(r->hasError());
    QCOMPARE(r->lastError().type(), QSparqlError::StatementError);
    delete r;
}

void TrackerDirectCommon::iterate_result()
{
    // This test will print out warnings
    testLogLevel = QtCriticalMsg;
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery q("select ?u ?ng {?u a nco:PersonContact; "
                    "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                    "nco:nameGiven ?ng .}");
    QSparqlResult* r = runQuery(conn, q);
    QVERIFY(r!=0);
    QVERIFY(checkResultSize(r, 3));

    QVERIFY(r->pos() == QSparql::BeforeFirstRow);
    // This is not a valid position
    for (int i=-1; i <= 2; ++i) {
        QCOMPARE(r->binding(i), QSparqlBinding());
        QVERIFY(r->value(i).isNull());
    }
    QCOMPARE(r->current(), QSparqlResultRow());

    for (int i=0; i<3; ++i) {
        QVERIFY(r->next());
        QCOMPARE(r->pos(), i);

        QVERIFY(r->binding(-1).value().isNull());
        QVERIFY(r->binding(0).value().isNull() == false);
        QVERIFY(r->binding(1).value().isNull() == false);
        QVERIFY(r->binding(2).value().isNull());

        QVERIFY(r->value(-1).isNull());
        QVERIFY(r->value(0).isNull() == false);
        QVERIFY(r->value(1).isNull() == false);
        QVERIFY(r->value(2).isNull());
    }
    QVERIFY(!r->next());
    QVERIFY(r->pos() == QSparql::AfterLastRow);
    // This is not a valid position
    for (int i=-1; i <= 2; ++i) {
        QCOMPARE(r->binding(i), QSparqlBinding());
        QVERIFY(r->value(i).isNull());
    }
    QCOMPARE(r->current(), QSparqlResultRow());
    QVERIFY(r->isFinished());

    delete r;
}

void TrackerDirectCommon::special_chars()
{
    // This test will leave unclean test data in tracker if it crashes.
    QSparqlConnection conn("QTRACKER_DIRECT");
    QString withSpecialChars("foo\u2780\u2781\u2782");
    QSparqlQuery add(QString("insert { <addeduri002> a nco:PersonContact; "
                             "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                             "nco:nameGiven \"%1\" .}").arg(withSpecialChars),
                             QSparqlQuery::InsertStatement);

    QSparqlResult* r = runQuery(conn, add);
    QVERIFY(r!=0);
    QVERIFY(r->isFinished());
    delete r;

    // Verify that the insertion succeeded
    QSparqlQuery q("select ?u ?ng {?u a nco:PersonContact; "
                   "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
                   "nco:nameGiven ?ng .}");
    QHash<QString, QString> contactNames;
    r = runQuery(conn, q);
    QVERIFY(r!=0);
    QVERIFY(checkResultSize(r, 4));
    while (r->next()) {
        contactNames[r->value(0).toString()] =
            r->value(1).toString();
    }
    CHECK_ERROR(r);
    QVERIFY(r->isFinished());
    QCOMPARE(contactNames.size(), 4);
    QCOMPARE(contactNames["addeduri002"], withSpecialChars);
    delete r;

    // Delete the uri
    QSparqlQuery del("delete { <addeduri002> a rdfs:Resource. }",
                     QSparqlQuery::DeleteStatement);

    r = runQuery(conn, del);
    QVERIFY(r!=0);
    delete r;
}

void TrackerDirectCommon::data_types()
{
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery dataTypes("select "
                           "<qsparql-tracker-direct-tests> "
                           "80 "
                           "23.4 "
                           "true "
                           "false "
                           "\"a string\" "
                           "{ }");
    QSparqlResult* r = runQuery(conn, dataTypes);
    QVERIFY(r!=0);

    QVERIFY(r->next());
    QCOMPARE(r->value(0),
             QVariant(QUrl::fromEncoded("qsparql-tracker-direct-tests")));
    QCOMPARE(r->value(1), QVariant(80));
    QCOMPARE(r->value(2), QVariant(23.4));
    QCOMPARE(r->value(3), QVariant(true));
    QCOMPARE(r->value(4), QVariant(false));
    QCOMPARE(r->value(5), QVariant(QString::fromLatin1("a string")));

    // urls don't have data type uris
    QCOMPARE(r->binding(0).dataTypeUri(),
            QUrl::fromEncoded(""));

    QCOMPARE(r->binding(1).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#integer"));

    QCOMPARE(r->binding(2).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#double"));

    QCOMPARE(r->binding(3).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#boolean"));
    QCOMPARE(r->binding(4).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#boolean"));

    QCOMPARE(r->binding(5).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#string"));

    delete r;
}

void TrackerDirectCommon::explicit_data_types()
{
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlQuery explicitTypes("select "
                               "<qsparql-tracker-direct-tests> "
                               "\"80\"^^xsd:integer "
                               "\"-7\"^^xsd:int "
                               "\"23.4\"^^xsd:double "
                               "\"true\"^^xsd:boolean "
                               "\"false\"^^xsd:boolean "
                               "\"a string\"^^xsd:string "
                               "\"2011-03-28T09:36:00+02:00\"^^xsd:dateTime "
                               "{ }");
    QSparqlResult* r = runQuery(conn, explicitTypes);
    QVERIFY(r!=0);

    QVERIFY(r->next());
    QCOMPARE(r->value(0),
             QVariant(QUrl::fromEncoded("qsparql-tracker-direct-tests")));
    QCOMPARE(r->value(1), QVariant(80));
    QCOMPARE(r->value(2), QVariant(-7));
    QCOMPARE(r->value(3), QVariant(23.4));
    QCOMPARE(r->value(4), QVariant(true));
    QCOMPARE(r->value(5), QVariant(false));
    QCOMPARE(r->value(6), QVariant(QString::fromLatin1("a string")));

    // Tracker seems to return the datetime as a string
    QCOMPARE(r->value(7),
             QVariant(QDateTime::fromString("2011-03-28T09:36:00+02:00", Qt::ISODate)));

    // urls don't have data type uris
    QCOMPARE(r->binding(0).dataTypeUri(),
            QUrl::fromEncoded(""));

    QCOMPARE(r->binding(1).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#integer"));
    QCOMPARE(r->binding(2).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#integer"));

    QCOMPARE(r->binding(3).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#double"));

    QCOMPARE(r->binding(4).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#boolean"));
    QCOMPARE(r->binding(5).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#boolean"));

    QCOMPARE(r->binding(6).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#string"));

    QCOMPARE(r->binding(7).dataTypeUri(),
             QUrl::fromEncoded("http://www.w3.org/2001/XMLSchema#dateTime"));

    delete r;
}

void TrackerDirectCommon::large_integer()
{
    QSparqlQuery insert("insert {<mydataobject> a nie:DataObject, nie:InformationElement ; "
                        "nie:isLogicalPartOf <qsparql-tracker-live-tests> ;"
                        "nie:byteSize 5000000000 .}", QSparqlQuery::InsertStatement);
    QSparqlConnection conn("QTRACKER_DIRECT");
    QSparqlResult* r = runQuery(conn, insert);
    QVERIFY(r!=0);
    delete r;
    QSparqlQuery select("select ?do ?bs "
                        "{ ?do a nie:DataObject ; "
                        "nie:isLogicalPartOf <qsparql-tracker-live-tests> ;"
                        "nie:byteSize ?bs .}");
    r = runQuery(conn, select);
    QVERIFY(r!=0);

    QVERIFY(r->next());
    QCOMPARE(r->value(1), QVariant(Q_UINT64_C(5000000000)));
    delete r;

    QSparqlQuery clean("delete {<mydataobject> a rdfs:Resource . }",
                       QSparqlQuery::DeleteStatement);
    r = runQuery(conn, clean);
    QVERIFY(r!=0);
    delete r;
}


void TrackerDirectCommon::datatype_string_data()
{
    QString property = "nie:mimeType";
    QString classes = "";
    QString dataType = "http://www.w3.org/2001/XMLSchema#string";

    QTest::newRow("emptyString") <<
        property <<
        classes <<
        "\"\"" <<
        QVariant("") <<
        dataType;
    QTest::newRow("nonemptyString") <<
        property <<
        classes <<
        "\"nonempty\"" <<
        QVariant("nonempty") <<
        dataType;
}

void TrackerDirectCommon::datatype_int_data()
{
    QString property = "nie:byteSize";
    QString classes = ", nie:DataObject";
    QString dataType = "http://www.w3.org/2001/XMLSchema#integer";

    QTest::newRow("typicalNegativeInteger") <<
        property <<
        classes <<
        "-600" <<
        QVariant(qlonglong(-600)) <<
        dataType;
    QTest::newRow("typicalPositiveInteger") <<
        property <<
        classes <<
        "800" <<
        QVariant(qlonglong(800)) <<
        dataType;
    QTest::newRow("zeroInterger") <<
        property <<
        classes <<
        "0" <<
        QVariant(qlonglong(0)) <<
        dataType;
    QTest::newRow("bigInterger") <<
        property <<
        classes <<
        "9223372036854775807" <<
        QVariant(qlonglong(Q_UINT64_C(9223372036854775807))) <<
        dataType;
    QTest::newRow("smallInterger") <<
        property <<
        classes <<
        "-9223372036854775808" <<
        QVariant(qlonglong(Q_UINT64_C(-9223372036854775808))) <<
        dataType;
}

void TrackerDirectCommon::datatype_double_data()
{
    QString property = "slo:latitude";
    QString classes = ", slo:GeoLocation";
    QString dataType = "http://www.w3.org/2001/XMLSchema#double";

    QTest::newRow("typicalNegativeDouble") <<
        property <<
        classes <<
        "-300.123456789" <<
        QVariant(-300.123456789) <<
        dataType;
    QTest::newRow("typicalPositiveDouble") <<
        property <<
        classes <<
        "987.09877" <<
        QVariant(987.09877) <<
        dataType;
    QTest::newRow("zeroDouble") <<
        property <<
        classes <<
        "0" <<
        QVariant(0.0) <<
        dataType;
    QTest::newRow("maxSignificantDigitsDouble") <<
        property <<
        classes <<
        "-0.12345678901234" <<
        QVariant(-0.12345678901234) <<
        dataType;
    QTest::newRow("maxSignificantDigitsDouble2") <<
        property <<
        classes <<
        "0.12345678901234" <<
        QVariant(0.12345678901234) <<
        dataType;
    QTest::newRow("withExponentDouble") <<
        property <<
        classes <<
        "0.12345678901234e+19" <<
        QVariant(0.12345678901234e+19) <<
        dataType;
    QTest::newRow("withExponentDouble3") <<
        property <<
        classes <<
        "-0.12345678901234e+19" <<
        QVariant(-0.12345678901234e+19) <<
        dataType;
    QTest::newRow("withExponentDouble4") <<
        property <<
        classes <<
        "-0.12345678901234e-19" <<
        QVariant(-0.12345678901234e-19) <<
        dataType;
}

void TrackerDirectCommon::datatype_datetime_data()
{
    QString property = "nco:birthDate";
    QString classes = ", nco:PersonContact";
    QString dataType = "http://www.w3.org/2001/XMLSchema#dateTime";
    QTest::newRow("typicalDateTime") <<
        property <<
        classes <<
        "\"1953-03-16T03:20:12Z\"" <<
        QVariant(QDateTime::fromString("1953-03-16T03:20:12Z", Qt::ISODate)) <<
        dataType;
}

void TrackerDirectCommon::datatype_boolean_data()
{
    QString property = "tracker:available";
    QString classes = ", nie:DataObject";
    QString dataType = "http://www.w3.org/2001/XMLSchema#boolean";

    QTest::newRow("trueBool") <<
        property <<
        classes <<
        "true" <<
        QVariant(true) <<
        dataType;
    QTest::newRow("falseBool") <<
        property <<
        classes <<
        "false" <<
        QVariant(false) <<
        dataType;
}

void TrackerDirectCommon::datatypes_as_properties_data()
{
    // which property in the ontology can be used for inserting this type of
    // data
    QTest::addColumn<QString>("property");
    // what classes the test resource needs to belong to (in addition to
    // nie:InformationElement)
    QTest::addColumn<QString>("classes");
    // string representation of the value, inserted into the query
    QTest::addColumn<QString>("string");
    // Expected value: what QVariant we should get back
    QTest::addColumn<QVariant>("value");
    // Expected value: data type uri of the constructed QSparqlBinding
    QTest::addColumn<QString>("dataTypeUri");

    // different data sets
    datatype_string_data();
    datatype_int_data();
    datatype_double_data();
    datatype_datetime_data();
    datatype_boolean_data();
}

void TrackerDirectCommon::datatypes_as_properties()
{
    QFETCH(QString, property);
    QFETCH(QString, classes);
    QFETCH(QString, string);
    QFETCH(QVariant, value);
    QFETCH(QString, dataTypeUri);

    QSparqlConnection conn("QTRACKER_DIRECT");

    // Insert the test data.
    QSparqlQuery insertQuery(
        QString("insert { "
        "<testresource> a nie:InformationElement %1 ; "
        "nie:isLogicalPartOf <qsparql-tracker-direct-tests> ;"
        "%2 %3 . }").arg(classes).arg(property).arg(string),
        QSparqlQuery::InsertStatement);

    QSparqlResult* r = runQuery(conn, insertQuery);
    QVERIFY(r!=0);
    CHECK_ERROR(r);
    delete r;

    QSparqlQuery query(QString("select ?value { "
                                "<testresource> %1 ?value . "
                                "}").arg(property));

    r = runQuery(conn, query);
    QVERIFY(r!=0);
    CHECK_ERROR(r);

    QVERIFY(r->next());
    QVariant val = r->value(0);
    QSparqlBinding binding = r->binding(0);
    QVERIFY(!r->next());

    QSparqlQuery cleanString("delete { <testresource> a rdfs:Resource . }",
                             QSparqlQuery::DeleteStatement);
    QSparqlResult* deleteResult = runQuery(conn, cleanString);
    QVERIFY(deleteResult!=0);
    CHECK_ERROR(deleteResult);
    delete deleteResult;

    QCOMPARE(val, value);
    QCOMPARE(val.type(), value.type());
    QCOMPARE(binding.value(), value);
    QCOMPARE(binding.value().type(), value.type());
    QCOMPARE(binding.name(), QString("value"));
    QCOMPARE(binding.dataTypeUri().toString(), dataTypeUri);
    delete r;
}
