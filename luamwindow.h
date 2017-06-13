#ifndef LUAMWINDOW_H
#define LUAMWINDOW_H

#include <QMainWindow>
#include "UniversalModels/include/AdaptingTableModel.h"
#include "UniversalModels/include/TableDataInterface.h"
#include "UniversalModels/include/TableDataListHolder.h"
namespace Ui {
class luamwindow;
}

class luamwindow : public QMainWindow
{
    Q_OBJECT
    struct FileSystemEntity{
        QString path;
        QString fileName;
        QString filePath;
        int position;
        bool exists = false;
        bool enabled = true;
        QString content;
    };

public:
    explicit luamwindow(QWidget *parent = 0);
    ~luamwindow();
    void AddFolderToCurrentSet(QString folder);
    void ProcessFileIntoList(QString file);
    void RaiseIndex(int index);
    void LowerIndex(int index);
    int GetLastFilePosition();
    void SaveFileListIntoDatabase();
    void SaveFolderListIntoDatabase();
    void MergeCurrentlyActiveFilesIntoFolder(QString inputFile, QString outputFolder, QString outputFileName);
    bool HasDuplicates(const QList<FileSystemEntity> &list);
    void SetupModelAccess();
    void SetupModelForFiles();
    void SaveFileListToDatabase();
    void SaveFolderListToDatabase();

    void ReadFileListFromDatabase();
    void ReadFolderListFromDatabase();

    QList<FileSystemEntity> ReadFilesFromDatabase(QString set);
private:
    Ui::luamwindow *ui;
    bool requireMode = true;

    // model part
    AdaptingTableModel* fileModel = nullptr;
    QSharedPointer<TableDataInterface> fileTableInterface;
    TableDataListHolder<FileSystemEntity>* fileListHolder = nullptr;

    AdaptingTableModel* folderModel = nullptr;
    QSharedPointer<TableDataInterface> folderTableInterface;
    TableDataListHolder<FileSystemEntity>* folderListHolder = nullptr;

public slots:
    void OnFoldersTableDoubleClicked(const QModelIndex &index);

private slots:
    void on_pbSelectASFolder_clicked();
    void on_pbUp_clicked();
    void on_pdDown_clicked();
    void on_pbAddFiles_clicked();

    void on_pbPushIntoEasy_clicked();
    void on_pbPushIntoHard_clicked();
    void on_pbPushIntoElite_clicked();
    void on_pbPushIntoCasual_clicked();
    void on_pbPushIntoLegacyHard_clicked();
    void on_pbMergeToOutput_clicked();
};

#endif // LUAMWINDOW_H
