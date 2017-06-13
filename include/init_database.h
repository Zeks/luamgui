#pragma once
#include <QString>


namespace database{
    bool ReadDbFile(QString file, QString connectionName = "");
    bool ReindexTable(QString table);
    void BackupDatabase();
    void InstallCustomFunctions();
}
