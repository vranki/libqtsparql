/****************************************************************************
**
** Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (ivan.frade@nokia.com)
**
** This file is part of the test suite of the QtSparql module (not yet part of the Qt Toolkit).
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at ivan.frade@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>

int main(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    QString dirPath = QDir::homePath() + QDir::separator();
    QDir myDir(dirPath);
    QStringList fileList = myDir.entryList(QStringList() << "benchmark-*.xml");
    QString pageOutput;
    pageOutput = "<html>"
    "<head>"
    "<title>Benchmark result comparison</title>"
    "</head>"
    "<style>body, table {font-size:12px;}"
    "td.row1{ background-color:#FFFFFF;}"
    "td.row2{ background-color:#E8E8E8;}"
    "sup.t1{color:red;}"
    "sup.t2{color:green;}"
    ".b{color:#006699;}"
    ".o{color:#CC6633;}"
    "</style>"
    "<body><h1>Report generated by Benchmark Comparison Tool (part of QSparql test library)</h1>";
    QHash<QString, QStringList> results;
    QHash<QString, QString> tracker_ver;
    QHash<QString, QString> qsparql_ver;
    QStringList testNames;

    //lets iterate through files, read them and parse
    for(int dirIterator=0; dirIterator < fileList.count(); dirIterator++)
    {
        QString filename(dirPath+fileList.at(dirIterator));
        QDomDocument doc("xmlResult");
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Couldn't open file "<< filename;
            pageOutput.append("Error: Couldn't open file "+filename);
        }
        else
        if (!doc.setContent(&file)) {
            file.close();
            qDebug() << "Couldn't set file content for  QDomDocument "<< filename;
            pageOutput.append("Couldn't set file content for  QDomDocument  "+filename);
        }
        else
        {
            file.close();

            QDomNode asset = doc.elementsByTagName("benchmark").at(0).firstChild();
            QDomNode assetIterator = asset.firstChild();
            QString tracker, qsparql, created;
            for(QDomNode assetIterator = asset.firstChild(); !assetIterator.isNull();
                                                assetIterator = assetIterator.nextSibling())
            {
                QString tagName = assetIterator.toElement().tagName();
                if(tagName == "tracker")
                    tracker = assetIterator.toElement().text();
                else if(tagName == "qsparql")
                    qsparql = assetIterator.toElement().text();
                else if(tagName == "created")
                    created = assetIterator.toElement().text();
            }

            //QString description = assetIterator.toElement().text();
            tracker_ver[fileList.at(dirIterator)]=tracker;
            qsparql_ver[fileList.at(dirIterator)]=qsparql;
            QDomNodeList testList = doc.elementsByTagName("benchmark").at(0).lastChild().
                                    toElement().elementsByTagName("test");
            for(int i=0; i< testList.count(); i++)
            {
                QString name = testList.at(i).toElement().attribute("name");
                if(!testNames.contains(name))
                    testNames << name;
                QDomNode median = testList.at(i).toElement().firstChild();
                QString medianValue = median.toElement().attribute("value");
                QDomNode mean = median.nextSibling();
                QString meanValue = mean.toElement().attribute("value");
                QDomNode total = mean.nextSibling();
                QString totalValue = total.toElement().attribute("value");
                QStringList runResults;
                runResults << medianValue << meanValue << totalValue;
                results[fileList.at(dirIterator)+name] = runResults;
            }
        }
    }

    pageOutput.append("<br /><table><tr><td>Test name</td>");
    for(int dirIterator=0; dirIterator < fileList.count(); dirIterator++)
    {
        QStringList nameparts = fileList.at(dirIterator).split(".");
        pageOutput.append("<td>" + nameparts[0] + "<br>" +nameparts[1] + "<br><span class=\"b\">"+
                          qsparql_ver[fileList.at(dirIterator)]+"</span><br>"+
                          "<span class=\"o\">"+tracker_ver[fileList.at(dirIterator)]+"</span></td>");
    }
    pageOutput.append("</tr>\n");
    QStringList previousResult;
    for(int testIterator=0; testIterator < testNames.count(); testIterator++)
    {
        previousResult.clear();
        pageOutput.append(QString("<tr><td class=\"row%1").arg(testIterator%2+1)+
                          "\" style=\"width:230px;\"><b>"+testNames[testIterator].remove(".xml")+
                          "</b><br /><small>median<br />mean<br />total</small></td>");
        for(int dirIterator=0; dirIterator < fileList.count(); dirIterator++)
        {
            pageOutput.append("<td class=\"row"+QString("%1").arg(testIterator%2+1)+"\">");
            for(int partResultIterator=0; partResultIterator < results[fileList.at(dirIterator)+
                                            testNames[testIterator]].count(); partResultIterator++)
            {
                int currentValue=results[fileList.at(dirIterator)+testNames[testIterator]].
                                                            at(partResultIterator).toInt();
                pageOutput.append(QString("%1").arg(currentValue));
                if(previousResult.count() == results[fileList.at(dirIterator)+
                   testNames[testIterator]].count() && previousResult.count()-1 == partResultIterator)
                {
                    int previousValue=previousResult[partResultIterator].toInt();
                    int diff = (previousValue?(((previousValue-currentValue)*100)/previousValue):100);
                    pageOutput.append(QString("  <sup class=\"t%2\">%1%</sup>").arg(diff*-1).
                                                                        arg(diff<0?"1":"2"));
                }
                pageOutput.append("<br />");
            }
            pageOutput.append("</td>\n");
            previousResult = results[fileList.at(dirIterator)+testNames[testIterator]];
        }
        pageOutput.append("</tr>\n\n");
    }
    pageOutput.append("</tr></table>");
    pageOutput.append("</body></html>");
    QFile data(dirPath+"index.html");
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        out << pageOutput;
        qDebug() << "Report saved in " << dirPath;
    }
    else
        qDebug() << "Couldn't save report in " << dirPath << "Check writing permissions!";
    return 0;
}