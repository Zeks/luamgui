#include "luamwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QDebug>
#include "include/init_database.h"


void CreateIndex(QString value)
{
    QSqlDatabase db = QSqlDatabase::database("filesystem.sqlite");
    QSqlQuery q(db);
    q.prepare(value);
    q.exec();
    qDebug() << q.lastError();
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("luamgui");
    //database::BackupDatabase();
    QString path = "filesystem.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();

    database::ReadDbFile("dbcode/dbinit.sql");
    //database::ReindexTable("tags");
    luamwindow w;
    w.show();

    database::InstallCustomFunctions();

    return a.exec();
}
