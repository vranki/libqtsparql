#include "tracker_direct_common.h"
#include "testhelpers.h"
#include <QtTest/QtTest>
#include <QtSparql/QtSparql>

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
