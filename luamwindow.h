#ifndef LUAMWINDOW_H
#define LUAMWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QMouseEvent>
#include "UniversalModels/include/AdaptingTableModel.h"
#include "UniversalModels/include/TableDataInterface.h"
#include "UniversalModels/include/TableDataListHolder.h"
namespace Ui {
class luamwindow;
}

class CustomDblClickTableview : public QTableView
{
  Q_OBJECT
public:
    CustomDblClickTableview(QWidget* parent = nullptr): QTableView(parent){}
  signals:
    void backgroundDoubleClickEvent(void);
  protected:
    void mouseDoubleClickEvent (QMouseEvent* e)
    {
      if (indexAt(e->pos()).isValid())
      {
          QTableView::mouseDoubleClickEvent(e);
      }
      else
      {
        e->accept();
        emit backgroundDoubleClickEvent();
      }

    }
};


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
    struct FileSaveToken{
        QString path;
        QString originalPath;
    };

public:
    explicit luamwindow(QWidget *parent = 0);
    ~luamwindow();
    void SaveGuiStateToSettings();
    void LoadGuiStateFromSettings();
    void SaveIntoDatabase();
    void LoadFromDatabase();
    FileSystemEntity EntityFromPath(QString);

    void AddFolderToCurrentSet(QString folder);
    void ProcessFileIntoList(QString file);
    void RaiseIndex(int index);
    void LowerIndex(int index);
    QModelIndex GetCurrentFileIndex();
    int GetLastFilePosition();
    void MergeCurrentlyActiveFilesIntoFolder(QString inputFile, QString outputFolder, QString outputFileName);
    bool HasDuplicates(const QList<FileSystemEntity> &list);
    void SetupModelAccessForFiles();
    void SetupModelAccessForFolders();
    void SetupModelForFiles();
    void SetupModelForFolders();
    void SaveFileListToDatabase();
    void SaveFolderListToDatabase();

    void ReadFileListFromDatabase();
    void ReadFolderListFromDatabase();

    QList<FileSystemEntity> ReadFilesFromDatabase(QString set);
    Ui::luamwindow *ui;
private:

    bool requireMode = true;

    // model part
    AdaptingTableModel* fileModel = nullptr;
    QSharedPointer<TableDataInterface> fileTableInterface;
    TableDataListHolder<FileSystemEntity>* fileListHolder = nullptr;

    AdaptingTableModel* folderModel = nullptr;
    QSharedPointer<TableDataInterface> folderTableInterface;
    TableDataListHolder<FileSystemEntity>* folderListHolder = nullptr;
    QList<FileSaveToken> undoBackups;

public slots:
    void OnFolderDoubleClicked(const QModelIndex& index);
    void OnFoldersTableDoubleClicked();
    void OnFilesTableDoubleClicked();
    void OnFileDoubleClicked(const QModelIndex& index );
    void ToggleCurrentFile();
    void DeleteCurrentFile();
    void SelectOutputFolder();
    void SelectProtoFile();
    void UndoLatestReplace();
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
    void on_pbAddFolder_clicked();
};

#endif // LUAMWINDOW_H
