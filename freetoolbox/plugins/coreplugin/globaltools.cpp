/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2010 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, MD <eric.maeker@free.fr>               *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#include "globaltools.h"

#include <coreplugin/icore.h>
#include <coreplugin/isettings.h>
#include <coreplugin/ftb_constants.h>

#include <utils/global.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QStringList>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTime>
#include <QVariant>
#include <QProgressDialog>
#include <QAbstractTableModel>

#include <QDebug>

#include <quazip/quazip/quazip.h>
#include <quazip/quazip/quazipfile.h>

#include <utils/log.h>

static inline Core::ISettings *settings()  { return Core::ICore::instance()->settings(); }
static inline QString databaseAbsPath() {return QDir::cleanPath(settings()->value(Core::Constants::S_DBOUTPUT_PATH).toString() + Core::Constants::MASTER_DATABASE_FILENAME);}
static inline QString masterDatabaseSqlSchema() {return settings()->value(Core::Constants::S_SVNFILES_PATH).toString() + QString(Core::Constants::FILE_MASTER_DATABASE_SCHEME);}
static inline QString routesCsvAbsFile() {return settings()->value(Core::Constants::S_SVNFILES_PATH).toString() + QString(Core::Constants::FILE_DRUGS_ROUTES);}


namespace Core {

namespace Tools
{

/** \todo code has moved to QuaZip/global */
const bool unzipFile( const QString & fileName, const QString & pathToUnZippedFiles )
{
     Q_ASSERT_X( QFile( fileName ).exists() , "Function unzipFile()",
                 qPrintable( QString( "File %1 does not exists" ).arg( fileName ) ) );

     Q_ASSERT_X( QDir( pathToUnZippedFiles ).exists() , "Function unzipFile()",
                 qPrintable( QString( "Dir %1 does not exists" ).arg( pathToUnZippedFiles ) ) );

//     QProcess unzip;
//     unzip.start("unzip", QStringList() << fileName << "-d " + pathToUnZippedFiles);
//
//     if (!unzip.waitForFinished(100000))
//         return false;
//
//     Utils::Log::addMessage("Tools", "unzip returned : " + unzip.readAll());
//     return true;

     QuaZip zip( fileName );
     if (!zip.open( QuaZip::mdUnzip ) ) {
          Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ));
          return false;
     }

     QuaZipFileInfo info;
     QuaZipFile file( &zip );
     QFile out;
     QString name;
     char c;

     for ( bool more = zip.goToFirstFile(); more; more = zip.goToNextFile() ) {
          if ( !file.open( QIODevice::ReadOnly ) ) {
               Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ));
               return false;
          }

          name = file.getActualFileName();

          if ( file.getZipError() != UNZ_OK ) {
               Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ));
               return false;
          }
          out.setFileName( pathToUnZippedFiles + QDir::separator() + name );

          // inform user
          Utils::Log::addMessage("Tools", QString( "Zip : extracting : %1" ).arg( out.fileName() ));

          if ( !out.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
               Utils::Log::addError("Tools", QString( "%1: %2" ).arg( out.fileName() ).arg( out.error() ));
               return false;
          }
          while ( file.getChar( &c ) ) out.putChar( c );
          out.close();

          if ( file.getZipError() != UNZ_OK ) {
               Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ));
               return false;
          }

          if ( !file.atEnd() ) {
               Utils::Log::addError("Tools", QString( "Zip : read all but not EOF : " ) + fileName );
               return false;
          }
          file.close();

          if ( file.getZipError() != UNZ_OK ) {
               Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ) );
               return false;
          }
     }
     zip.close();
     if ( zip.getZipError() != UNZ_OK ) {
          Utils::Log::addError("Tools", QString( "%1: %2" ).arg( fileName ).arg( zip.getZipError() ));
          return false;
     }

     return true;
}

const void unzipAllFilesIntoDirs( const QStringList & paths )
{
     foreach( QString p, paths ) {
          QDir d( p );
          if ( !d.exists() ) continue;

          // get all zip files in dir
          d.setNameFilters( QStringList() << "*.zip" );
          d.setFilter( QDir::Files | QDir::NoSymLinks | QDir::Readable );
          QStringList zipFiles = d.entryList();

          foreach( QString f, zipFiles ) {
               // if file if already unzipped dir with its baseName exists and is populated with files
               QDir dz( p );
               dz.setFilter( QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::NoDotAndDotDot );
               if ( ( dz.cd( QFileInfo( f ).baseName() ) ) &&
                         ( dz.entryList().count() ) )
                    continue;

               // d must not change here +++
               // file was not unzipped by me, so do it
               // in first create the output directory
               if ( !d.cd( QFileInfo( f ).baseName()  ) ) {
                    d.mkdir( QFileInfo( f ).baseName() );
                    dz.cd( QFileInfo( f ).baseName() );
               }
               else d.cdUp();

               // in second unzip file to the output directory
               unzipFile( d.absolutePath() + QDir::separator() + f, dz.absolutePath() );
          }
     }
}
/** \todo END */

QString noAccent(const QString & s)
{
    QString tmp = s;
    tmp.replace( QString::fromUtf8( "é" ) , "e" );
    tmp.replace( QString::fromUtf8( "è" ) , "e" );
    tmp.replace( QString::fromUtf8( "ê" ) , "e" );
    tmp.replace( QString::fromUtf8( "ë" ) , "e" );
    tmp.replace( QString::fromUtf8( "î" ) , "i" );
    tmp.replace( QString::fromUtf8( "ï" ) , "i" );
    tmp.replace( QString::fromUtf8( "É" ) , "E" );
    tmp.replace( QString::fromUtf8( "È" ) , "E" );
    tmp.replace( QString::fromUtf8( "Ê" ) , "E" );
    tmp.replace( QString::fromUtf8( "Ë" ) , "E" );
    tmp.replace( QString::fromUtf8( "Î" ) , "I" );
    tmp.replace( QString::fromUtf8( "Ï" ) , "I" );
    return tmp;
}

QString getBlock(const QString &content, const int posStart, int &posEnd, const QString &delimiter)
{
    // find first delimiter starting at pos posStart
    int begin = content.indexOf( delimiter, posStart );
    int end = content.indexOf( delimiter, begin + delimiter.length() + 1 );

    // modify posEnd for next block process
    if (begin == -1) {
        posEnd = posStart;
        return QString::null;
    } else {
        posEnd = end;
    }

    // return block
    QString tmp = content.mid( begin, end - begin );
    tmp.replace( "\r" , "" );
    tmp.replace( "\n" , "" );
    return tmp;
}

QString getBlock(const QString &content, const int posStart, int &posEnd, const QRegExp &delimiter)
{
    // find first delimiter starting at pos posStart
    int begin = content.indexOf( delimiter, posStart );
    int end = content.indexOf( delimiter, begin + 4 );

    // modify posEnd for next block process
    if (begin == -1) {
        posEnd = posStart;
        return QString::null;
    } else {
        posEnd = end;
    }

    // return block
    QString tmp = content.mid( begin, end - begin );
    tmp.replace( "\r" , "" );
    //      tmp.replace( "\n" , "" );
    return tmp;
}

bool executeProcess(const QString &proc)
{
    QProcess process;
    process.start(proc, QIODevice::ReadOnly);

    Utils::Log::addMessage("Tools", QString("Executing process : %1").arg(proc));

    if (!process.waitForStarted())
        Utils::Log::addError("Tools", QString("Process %1 can not start").arg(proc.left(20)), __FILE__, __LINE__);

    if (!process.waitForFinished(100000))
        Utils::Log::addError("Tools", QString("Process %1 does not end").arg(proc.left(20)), __FILE__, __LINE__);

    QString error = process.readAllStandardError();
    if (!error.isEmpty()) {
        Utils::Log::addError("Tools", QString("ERROR : %1").arg(proc), __FILE__, __LINE__);
        Utils::Log::addError("Tools", QString("ERROR : %1").arg(error), __FILE__, __LINE__);
        return false;
    }
    Utils::Log::addMessage("Tools", QString("Process done : %1, output : %2").arg(proc.left(20)).arg(QString(process.readAllStandardOutput())));
    return true;
}

bool executeSqlFile(const QString &connectionName, const QString &fileName, QProgressDialog *dlg)
{
    if (!QFile::exists(fileName)) {
        Utils::Log::addError("Tools", QString("ERROR: missing database schema file : %1.").arg(fileName), __FILE__, __LINE__);
        return false;
    }

    // execute all sql queries
    QString req = Utils::readTextFile(fileName);
    if (req.isEmpty()) {
        Utils::Log::addError("Tools", "File is empty : " + fileName, __FILE__, __LINE__);
        return false;
    }

    req.replace("\n\n", "\n");
    req.replace("\n\n", "\n");
    req.replace("\n\n", "\n");
    req.replace("\n\n", "\n");
    req.replace("\n\n", "\n");
    req.remove("AUTO_INCREMENT"); // SQLite compatibility

    QStringList list = req.split("\n");
    QSqlDatabase DB = QSqlDatabase::database(connectionName);
    if (!DB.isOpen()) {
        if (!DB.open()) {
            Utils::Log::addError("Tools", "Database not opened", __FILE__, __LINE__);
            return false;
        }
    }
//    if (!DB.transaction()) {
//        Utils::Log::addError("Tools", "Can not create transaction. Tools::executeSqlFile()", __FILE__, __LINE__);
//        return false;
//    }

    req.clear();
    QStringList queries;
    // Reconstruct req : removes comments
    foreach(const QString &s, list) {
        if (s.startsWith("--")) {
            Utils::Log::addMessage("Tools", s);
            continue;
        }
        req += s + " \n";
        if (s.endsWith(";")) {
            queries.append(req);
            req.clear();
        }
    }

    // Execute queries
    if (dlg)
        dlg->setRange(0, queries.count());

    foreach(const QString &sql, queries) {
        QTime time;
        time.start();

        QString q = sql.simplified();
        // Do not processed empty strings
        if (q.isEmpty())
            continue;

        // No SQLite extra commands
        if (q.startsWith("."))
            continue;

        // No BEGIN, No COMMIT
        if (q.startsWith("BEGIN", Qt::CaseInsensitive) || q.startsWith("COMMIT", Qt::CaseInsensitive))
            continue;

        qWarning() << sql;

        QSqlQuery query(sql, DB);
        if (!query.isActive()) {
            Utils::Log::addError("Tools", QString("SQL ERROR : %1 \"%2\"").arg(query.lastError().text(), sql), __FILE__, __LINE__);
//            DB.rollback();
            return false;
        } else {
            Utils::Log::addMessage("Tools", QString("Query correctly done (%1 ms)").arg(time.elapsed()));
        }

        if (dlg)
            dlg->setValue(dlg->value()+1);
    }
//    DB.commit();
    return true;
}

bool executeSqlQuery(const QString &sql, const QString &dbName, const QString &file, int line)
{
    QSqlDatabase DB = QSqlDatabase::database(dbName);
    if (!DB.isOpen()) {
        if (file.isEmpty())
            Utils::Log::addError("Tools", "Unable to connect to " + dbName, __FILE__, __LINE__);
        else
            Utils::Log::addError("Tools", "Unable to connect to " + dbName, file, line);
        return false;
    }
    QSqlQuery query(sql, DB);
    if (!query.isActive()) {
        if (file.isEmpty())
            Utils::Log::addError("Tools", "Query Error : " + sql + " // " + query.lastError().text(), __FILE__, __LINE__);
        else
            Utils::Log::addError("Tools", "Query Error : " + sql + " // " + query.lastError().text(), file, line);
        return false;
    }
    return true;
}

bool connectDatabase(const QString &connection, const QString &fileName)
{
    // create empty sqlite database
    // test driver
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        Utils::Log::addError("Tools", QString("ERROR : SQLite driver is not available"), __FILE__, __LINE__);
        return false;
    }

    QSqlDatabase DB;

    // test drugs connection
    if (QSqlDatabase::contains(connection)) {
        DB = QSqlDatabase::database(connection);
    }  else {
        DB = QSqlDatabase::addDatabase("QSQLITE" , connection);
        if (QFileInfo(fileName).isRelative())
            DB.setDatabaseName(QDir::cleanPath(qApp->applicationDirPath() + Constants::MACBUNDLE + fileName));
        else
            DB.setDatabaseName(fileName);

        if (!DB.open()) {
            Utils::Log::addError("Tools", QString("ERROR : %1 // %2").arg(DB.lastError().text()).arg(fileName), __FILE__, __LINE__);
            return false;
        } else {
            Utils::Log::addMessage("Tools", QString("Connection to database created : %1 %2")
                    .arg(DB.connectionName(), DB.databaseName()));
        }
    }
    return true;
}

bool createMasterDrugInteractionDatabase()
{
    if (!connectDatabase(Core::Constants::MASTER_DATABASE_NAME, databaseAbsPath())) {
        Utils::Log::addError("Tools", "Unable to create master database");
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database(Core::Constants::MASTER_DATABASE_NAME);
    if (db.tables().count() < 10) {
        executeSqlFile(Core::Constants::MASTER_DATABASE_NAME, masterDatabaseSqlSchema());
        addRoutesToDatabase(Core::Constants::MASTER_DATABASE_NAME, routesCsvAbsFile());
    }
    return true;
}

bool signDatabase(const QString &connectionName)
{
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    if (!db.isOpen()) {
        if (!db.open()) {
            return false;
        }
    }

    QHash<QString, QString> tables;
    foreach(const QString &table, db.tables()) {
        QString req = QString("SELECT count(*) FROM %1;").arg(table);
        QSqlQuery query(db);
        if (query.exec(req)) {
            if (query.next())
                tables.insert(table, query.value(0).toString());
        }
    }
    if (tables.count() != db.tables().count())
        return false;

    QFileInfo info(db.databaseName());
    QString tag = info.fileName() + "(";
    tag += QString::number(info.size()) + "," + info.created().toString(Qt::ISODate) + ")@";
    foreach(const QString &table, tables.keys()) {
        tag += table + ":" + tables.value(table) + "/";
    }
    tag.chop(1);
    tag += "\n";

    QString fileName = QFileInfo(db.databaseName()).absolutePath() + "/check.db";
    QString content = QString(QByteArray::fromBase64(Utils::readTextFile(fileName, Utils::DontWarnUser).toUtf8()));
    QStringList linesToKeep;
    foreach(const QString &line, content.split("\n", QString::SkipEmptyParts)) {
        if (line.startsWith(QFileInfo(db.databaseName()).fileName() + "@")) {
            continue;
        }
        linesToKeep << line;
    }
    linesToKeep << tag;
    content.clear();
    content = linesToKeep.join("\n");

    Utils::saveStringToFile(content.toUtf8().toBase64(), fileName, Utils::Overwrite, Utils::DontWarnUser);

    return true;
}

int getSourceId(const QString &connection, const QString &dbUid)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            Utils::Log::addError("Tools","Unable to connection database", __FILE__, __LINE__);
            return -1;
        }
    }

    QString req;
    QSqlQuery query(db);

    // Source exists ?
    req = QString("SELECT SID FROM SOURCES WHERE (DATABASE_UID='%1')").arg(dbUid);
    if (query.exec(req)) {
        if (query.next()) {
            return query.value(0).toInt();;
        }
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
        return -1;
    }
    query.finish();
    return -1;
}

/** \brief Create a new drugs source in the Master database. Return -1 is an error occured, or the SID */
int createNewDrugsSource(const QString &connection, const QString &uid, QMultiHash<QString, QVariant> trLabels)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            Utils::Log::addError("Tools","Unable to connection database", __FILE__, __LINE__);
            return -1;
        }
    }

    QString req;
    QSqlQuery query(db);

    // Source exists ?
    req = QString("SELECT SID, MASTER_LID FROM SOURCES WHERE (DATABASE_UID='%1')")
          .arg(uid);
    if (query.exec(req)) {
        if (query.next()) {
            int sid = query.value(0).toInt();
            return sid;
        }
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
        return -1;
    }
    query.finish();

    // insert labels
    int masterLid = addLabels(connection, -1, trLabels);
    if (masterLid == -1) {
        Utils::Log::addError("Tools","Unable to add source", __FILE__, __LINE__);
        return -1;
    }

    // insert source
    req = QString("INSERT INTO SOURCES (SID, MASTER_LID, DATABASE_UID) VALUES (NULL, %1, '%2')")
          .arg(masterLid)
          .arg(uid)
          ;
    if (query.exec(req)) {
        return query.lastInsertId().toInt();
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
    }

    return -1;
}

/** \brief Return the used MASTER_LID or -1 in case of an error */
int addLabels(const QString &connection, const int masterLid, QMultiHash<QString, QVariant> trLabels)
{
    QString req;
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            Utils::Log::addError("Tools","Unable to connection database", __FILE__, __LINE__);
            return -1;
        }
    }
    QSqlQuery query(db);
    int mid = masterLid;
    if (mid == -1) {
        // get new master_lid
        req = "SELECT max(MASTER_LID) FROM `LABELS_LINK`;";
        if (query.exec(req)) {
            if (query.next())
                mid = query.value(0).toInt();
            ++mid;
        } else {
            Utils::Log::addQueryError("Drugs", query, __FILE__, __LINE__);
            return -1;
        }
    }

    // insert all translated labels
    foreach(const QString &lang, trLabels.uniqueKeys()) {
        foreach(const QVariant &value, trLabels.values(lang)) {
            QString t = value.toString();

            // Check is couple already exists
            req = QString("SELECT LID FROM LABELS WHERE (LANG='%1' AND LABEL=\"%2\")")
                  .arg(lang)
                  .arg(t);
            if (query.exec(req)) {
                if (query.next()) {
                    int lid = query.value(0).toInt();
                    query.finish();

                    req = QString("INSERT INTO `LABELS_LINK` (MASTER_LID, LID) VALUES "
                                  "(%1  ,%2)")
                            .arg(mid)
                            .arg(lid)
                            ;
                    if (!query.exec(req)) {
                        Utils::Log::addQueryError("Drugs", query, __FILE__, __LINE__);
                        return false;
                    }
                    query.finish();
                    return mid;
                }
            } else {
                Utils::Log::addQueryError("Drugs", query, __FILE__, __LINE__);
                return -1;
            }
            query.finish();

            req = QString("INSERT INTO `LABELS` (LID,LANG,LABEL) VALUES ("
                          "NULL,'%1','%2')")
                    .arg(lang)
                    .arg(t.replace("'","''"))
                    ;
            if (!query.exec(req)) {
                Utils::Log::addQueryError("Drugs", query, __FILE__, __LINE__);
                return -1;
            }
            int id = query.lastInsertId().toInt();
            query.finish();

            req = QString("INSERT INTO `LABELS_LINK` (MASTER_LID, LID) VALUES "
                          "(%1  ,%2)")
                    .arg(mid)
                    .arg(id)
                    ;
            if (!query.exec(req)) {
                Utils::Log::addQueryError("Drugs", query, __FILE__, __LINE__);
                return -1;
            }
            query.finish();
        }
    }
    return mid;
}

bool addRoutesToDatabase(const QString &connection, const QString &absFileName)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            Utils::Log::addError("Tools","Unable to connection database", __FILE__, __LINE__);
            return false;
        }
    }
    QString content = Utils::readTextFile(absFileName);
    if (content.isEmpty()) {
        Utils::Log::addError("Tools","Routes file does not exist.\n" + absFileName, __FILE__, __LINE__);
        return false;
    }
    db.transaction();
    // Read file
    foreach(const QString &line, content.split("\n", QString::SkipEmptyParts)) {
        if (line.startsWith("--"))
            continue;
        int id = 0;
        int rid = 0;
        QMultiHash<QString, QVariant> trLabels;
        // Parse line
        foreach(QString value, line.split(",")) {
            value = value.trimmed();
            if (id==0) {
                // is RID
                rid = value.toInt();
                ++id;
                continue;
            }
            ++id;
            value = value.remove("\"");
            int sep = value.indexOf(":");
            QString lang = value.left(sep);
            if (lang.compare("systemic") == 0) {
                /** \todo Code systemic extraction of routes */
            } else {
                trLabels.insertMulti(lang, value.mid(sep + 1));
            }
        }
        // Push to database
        int masterLid = Tools::addLabels(connection, -1, trLabels);
        if (masterLid == -1) {
            Utils::Log::addError("Tools", "Route not integrated", __FILE__, __LINE__);
            continue;
        }
        QString req = QString("INSERT INTO ROUTES (RID, MASTER_LID) VALUES (NULL, %1)")
                      .arg(masterLid)
                      ;
        Tools::executeSqlQuery(req, connection, __FILE__, __LINE__);
    }
    db.commit();
    return true;
}

QHash<int, QString> generateMids(const QStringList &molnames, const int sid, const QString &connection)
{
    QHash<int, QString> mids;
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen())
        return mids;

    QString req;
    db.transaction();
    QSqlQuery query(db);

    foreach(const QString &name, molnames) {

        // Ask for an existing MID
        req = QString("SELECT MID FROM MOLS WHERE NAME=\"%1\" AND SID=\"%2\";").arg(name).arg(sid);
        if (query.exec(req)) {
            if (query.next()) {
                // is already in the table MOLS
                mids.insert(query.value(0).toInt(), name);
                continue;
            }
        } else {
            Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
            continue;
        }
        query.finish();

        // create a new MID
        req = QString("INSERT INTO MOLS (MID, SID, NAME) VALUES (NULL,%1,\"%2\");")
              .arg(sid)
              .arg(name);

        if (query.exec(req)) {
            mids.insert(query.lastInsertId().toInt(), name);
            continue;
        } else {
            Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
            continue;
        }
        query.finish();
    }
    db.commit();
    return mids;
}

bool createAtc(const QString &connection, const QString &code, const QMultiHash<QString, QVariant> &trLabels, const int forceAtcId)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen())
        return false;
    db.transaction();
    QSqlQuery query(db);
    int id = 0;
    QString req;
    if (forceAtcId == -1)
        req = QString("INSERT INTO ATC  (ATC_ID, CODE) "
                  "VALUES (NULL, '%2') ").arg(code);
    else
        req = QString("INSERT INTO ATC  (ATC_ID, CODE) "
                  "VALUES (%1, '%2') ").arg(forceAtcId).arg(code);

    if (query.exec(req)) {
        id = query.lastInsertId().toInt();
        if (forceAtcId!=-1) {
            if (forceAtcId!=id) {
                Utils::Log::addError("Tools", QString("Wrong ATC_ID Db=%1 / Asked=%2").arg(id).arg(forceAtcId), __FILE__, __LINE__);
                db.rollback();
                return false;
            }
        }
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
    }
    query.finish();

    // Create labels
    int masterLid = Core::Tools::addLabels(Core::Constants::MASTER_DATABASE_NAME, -1, trLabels);
    if (masterLid == -1) {
        db.rollback();
        return false;
    }

    // Create ATC_LABELS link
    req = QString("INSERT INTO ATC_LABELS (ATC_ID, MASTER_LID) VALUES (%1, %2) ").arg(id).arg(masterLid);
    if (!query.exec(req)) {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
        db.rollback();
        return false;
    }
    query.finish();
    db.commit();
    return true;
}

bool addInteraction(const QString &connection, const QStringList &atc1, const QStringList &atc2, const QString &type, const QMultiHash<QString, QVariant> &risk, const QMultiHash<QString, QVariant> &management)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen())
        return false;
    QSqlQuery query(db);
    QString req;
    int iak_id = -1;
    QList<int> ia_ids;

    foreach(const QString &a1, atc1) {
        foreach(const QString &a2, atc2) {
            req = QString("INSERT INTO INTERACTIONS (ATC_ID1, ATC_ID2) VALUES (%1, %2);")
                    .arg(QString("(SELECT ATC_ID FROM ATC WHERE CODE=\"%1\")").arg(a1))
                    .arg(QString("(SELECT ATC_ID FROM ATC WHERE CODE=\"%1\")").arg(a2));
            if (query.exec(req)) {
                ia_ids << query.lastInsertId().toInt();
            } else {
                Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
                return false;
            }
            query.finish();
        }
    }
    // Check errors
    if (!ia_ids.count()) {
        Utils::Log::addError("Tools", QString("Interaction not added : %1   //  %2").arg(atc1.join(",")).arg(atc2.join(",")), __FILE__, __LINE__);
        return false;
    }
    // Add labels
    int riskMasterLid = addLabels(Core::Constants::MASTER_DATABASE_NAME, -1, risk);
    int manMasterLid = addLabels(Core::Constants::MASTER_DATABASE_NAME, -1, management);
    if (riskMasterLid==-1 || manMasterLid==-1)
        return false;

    // Add IAK
    /** \todo add bibliography */
    req = QString("INSERT INTO IAKNOWLEDGE (IAKID,TYPE,RISK_MASTER_LID,MAN_MASTER_LID) VALUES "
                  "(NULL, \"%1\", %2, %3)")
            .arg(type)
            .arg(riskMasterLid)
            .arg(manMasterLid);
    if (query.exec(req)) {
        iak_id = query.lastInsertId().toInt();
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
        return false;
    }
    query.finish();
    if (iak_id==-1)
        return false;

    // Add to IA_IAK
    foreach(const int ia, ia_ids) {
        req = QString("INSERT INTO IA_IAK (IAID,IAKID) VALUES (%1,%2)")
              .arg(ia)
              .arg(iak_id)
              ;
        if (!query.exec(req)) {
            Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
            return false;
        }
        query.finish();
    }

    return true;
}

int addBibliography(const QString &connection, const QString &type, const QString &link, const QString &reference, const QString &abstract, const QString &explain)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            return false;
        }
    }
    QString req;
    db.transaction();
    QSqlQuery query(db);

    // Bibliography exists ?
    int bib_id = -1;
    req = QString("SELECT BIB_ID FROM BIBLIOGRAPHY WHERE LINK=\"%1\"").arg(link);
    if (query.exec(req)) {
        if (query.next()) {
            bib_id = query.value(0).toInt();
        } else {
            // Create the bib and retreive the bib_id
            QString t = type;
            QString l = link;
            QString r = reference;
            QString a = abstract;
            QString e = explain;
            req = QString("INSERT INTO BIBLIOGRAPHY "
                          "(BIB_ID,TYPE,LINK,TEXTUAL_REFERENCE,ABSTRACT,EXPLANATION) VALUES"
                          "(NULL, '%1', '%2', '%3', '%4', '%5')")
                    .arg(t.replace("'","''"))
                    .arg(l.replace("'","''"))
                    .arg(r.replace("'","''"))
                    .arg(a.replace("'","''"))
                    .arg(e.replace("'","''"));
            query.finish();
            if (query.exec(req)) {
                bib_id = query.lastInsertId().toInt();
            } else {
                Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
                db.rollback();
                return false;
            }
            query.finish();
        }
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
        db.rollback();
        return false;
    }
    query.finish();
    db.commit();
    return bib_id;
}

bool addComponentAtcLinks(const QString &connection, const QMultiHash<int, int> &mol_atc, const int sid)
{
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            return false;
        }
    }
    db.transaction();
    QSqlQuery query(db);
    // Save to links to drugs database
    foreach(int mol, mol_atc.uniqueKeys()) {
        QList<int> atcCodesSaved;
        foreach(int atc, mol_atc.values(mol)) {
            if (atcCodesSaved.contains(atc))
                continue;
            atcCodesSaved.append(atc);
            QString req = QString("INSERT INTO LK_MOL_ATC (MID,ATC_ID,SID) VALUES (%1, %2, %3)")
                          .arg(mol).arg(atc).arg(sid);
            if (!query.exec(req)) {
                Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
                db.rollback();
                return false;
            }
            query.finish();
        }
    }
    db.commit();
    return true;
}

QVector<int> getAtcIds(const QString &connection, const QString &label)
{
    QVector<int> ret;
    QString req;
    QSqlDatabase db = QSqlDatabase::database(connection);
    if (!db.isOpen()) {
        if (!db.open()) {
            return ret;
        }
    }
    QSqlQuery query(db);

    req = QString("SELECT DISTINCT ATC.ATC_ID FROM ATC "
                  "JOIN ATC_LABELS ON ATC_LABELS.ATC_ID=ATC.ATC_ID "
                  "JOIN LABELS_LINK ON LABELS_LINK.MASTER_LID=ATC_LABELS.MASTER_LID "
                  "JOIN LABELS ON LABELS_LINK.LID=LABELS.LID "
                  "WHERE LABELS.LABEL like \"meropenem\";");
    if (query.exec(req)) {
        while (query.next()) {
            ret << query.value(0).toInt();
        }
    } else {
        Utils::Log::addQueryError("Tools", query, __FILE__, __LINE__);
    }
    return ret;
}

} // end namespace Tools

} // end namespace DrugsDbCreator
