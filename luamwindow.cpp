#include "luamwindow.h"
#include "ui_luamwindow.h"

#include "GlobalHeaders/simplesettings.h"
#include "UniversalModels/include/genericeventfilter.h"

#include <QFileDialog>
#include <QAbstractItemView>
#include <QModelIndex>
#include <QMouseEvent>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QDebug>
#include <QSet>
#include <QUuid>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>
#include <algorithm>

luamwindow::luamwindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::luamwindow)
{
    ui->setupUi(this);
    SetupModelForFiles();
    SetupModelForFolders();


    LoadFromDatabase();
    LoadGuiStateFromSettings();
    connect(ui->pbDisable, &QPushButton::clicked, this, &luamwindow::ToggleCurrentFile);
    connect(ui->pbDelete, &QPushButton::clicked, this, &luamwindow::DeleteCurrentFile);
    connect(ui->pbOpenProtoFileSelector, &QPushButton::clicked, this, &luamwindow::SelectProtoFile);
    connect(ui->pbSelectOutputFolder, &QPushButton::clicked, this, &luamwindow::SelectOutputFolder);
    connect(ui->pbUndo, &QPushButton::clicked, this, &luamwindow::UndoLatestReplace);


}

luamwindow::~luamwindow()
{
    SaveGuiStateToSettings();
    SaveIntoDatabase();
    delete ui;
}

void luamwindow::SaveGuiStateToSettings()
{
    QSettings settings("Settings/settings.ini", QSettings::IniFormat);
    settings.setValue("Main/audioshield_folder", ui->leAudioshieldFolder->text());
    settings.setValue("Main/output_folder", ui->leOutputFolderFolder->text());
    settings.setValue("Main/output_file", ui->leOutputFilename->text());
    settings.setValue("Main/currentFile", ui->leProtoFile->text());
    settings.setValue("Splitters/spFoldersFiles", ui->spFoldersFiles->saveState());
    settings.setValue("Headers/files", ui->tbvFiles->horizontalHeader()->saveState());
    settings.setValue("Headers/folders", ui->tbvFolders->horizontalHeader()->saveState());
    settings.sync();
}

void luamwindow::LoadGuiStateFromSettings()
{
    QSettings settings("Settings/settings.ini", QSettings::IniFormat);
    ui->leAudioshieldFolder->setText(settings.value("Main/audioshield_folder", "").toString());
    ui->leOutputFolderFolder->setText(settings.value("Main/output_folder", "").toString());
    ui->leOutputFilename->setText(settings.value("Main/output_file", "").toString());
    ui->leProtoFile->setText(settings.value("Main/currentFile", "").toString());
    ui->spFoldersFiles->restoreState(settings.value("Splitters/spFoldersFiles", QByteArray()).toByteArray());


}

void luamwindow::SaveIntoDatabase()
{
    SaveFileListToDatabase();
    SaveFolderListToDatabase();
}

void luamwindow::LoadFromDatabase()
{
    ReadFileListFromDatabase();
    ReadFolderListFromDatabase();
}

luamwindow::FileSystemEntity luamwindow::EntityFromPath(QString folder)
{
    FileSystemEntity e;
    e.path = folder;
    e.exists = QFileInfo::exists(folder);
    e.content = true;
    e.fileName = "";
    e.filePath = folder;
    return e;
}

void luamwindow::AddFolderToCurrentSet(QString folder)
{
    folderListHolder->GetData().push_back(EntityFromPath(folder));
    folderModel->OnReloadDataFromInterface();
}

void luamwindow::OnFoldersTableDoubleClicked()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::DirectoryOnly);
    auto result = d.exec();
    if(result != 1)
        return;

    auto folder = d.selectedFiles().at(0);
    AddFolderToCurrentSet(folder);
    SaveFolderListToDatabase();
}

void luamwindow::OnFilesTableDoubleClicked()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::ExistingFiles);
    auto result = d.exec();
    if(result != 1)
        return;

    for(auto file: d.selectedFiles())
    {
        ProcessFileIntoList(file);
    }
    SaveFileListToDatabase();
}

void luamwindow::OnFileDoubleClicked(const QModelIndex &index)
{
    if(!index.isValid())
        return;
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    if(data->exists == false)
        return;

    QProcess proc;
    proc.execute("notepad++.exe", QStringList() << data->path);
    auto errors = proc.readAllStandardError();
    ui->edtDiag->append(QString(errors));
}

void luamwindow::ToggleCurrentFile()
{
    auto index = GetCurrentFileIndex();
    if(!index.isValid())
        return;
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    data->enabled = !data->enabled;
    fileModel->OnReloadDataFromInterface();
    auto newIndex = fileModel->index(index.row(), index.column());
    bool isValid = newIndex.isValid();
    ui->tbvFiles->selectionModel()->select(newIndex, QItemSelectionModel::Rows);

}

void luamwindow::DeleteCurrentFile()
{
    auto index = GetCurrentFileIndex();
    if(!index.isValid())
        return;
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    QString path = data->path;
    auto it = std::remove_if(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()), [path](const FileSystemEntity& f){
        return f.path == path;
    });
    fileListHolder->GetData().erase(it, std::end(fileListHolder->GetData()));
    fileModel->OnReloadDataFromInterface();
}

void luamwindow::SelectOutputFolder()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::DirectoryOnly);
    auto result = d.exec();
    if(result != 1)
        return;
    ui->leOutputFolderFolder->setText(d.selectedFiles().at(0));
}

void luamwindow::SelectProtoFile()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::ExistingFiles);
    auto result = d.exec();
    if(result != 1)
        return;
    ui->leProtoFile->setText(d.selectedFiles().at(0));
}

void luamwindow::UndoLatestReplace()
{
    if(undoBackups.size() == 0)
        return;
    auto backupFile = undoBackups.last();
    undoBackups.pop_back();
    if(!QFileInfo::exists(backupFile.path))
        return;

    QFile::copy(backupFile.path, backupFile.originalPath);
}


void luamwindow::on_pbSelectASFolder_clicked()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::DirectoryOnly);
    auto result = d.exec();
    if(result != 1)
        return;
    ui->leAudioshieldFolder->setText(d.selectedFiles().at(0));
}

void luamwindow::on_pbUp_clicked()
{
    QModelIndex index = GetCurrentFileIndex();
    if(!index.isValid())
        return;
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    RaiseIndex(data->position);
}

void luamwindow::on_pdDown_clicked()
{
    QModelIndex index = GetCurrentFileIndex();
    if(!index.isValid())
        return;
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    LowerIndex(data->position);
}

void luamwindow::on_pbAddFiles_clicked()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::ExistingFiles);
    auto result = d.exec();
    if(result != 1)
        return;
    for(auto file: d.selectedFiles())
    {
        ProcessFileIntoList(file);
    }
    SaveFileListToDatabase();
}

void luamwindow::ProcessFileIntoList(QString file)
{
    //auto exists = QFileInfo::exists(file);
    auto it = std::find_if(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()), [file](const FileSystemEntity& f){
        return f.path == file;
    });
    if(it != std::end(fileListHolder->GetData()))
    {
        QMessageBox::information(nullptr, "Info", "File is already on the list");
        return;
    }
    FileSystemEntity fileToken;
    fileToken.exists = QFileInfo::exists(file);
    fileToken.path = file;
    fileToken.position = GetLastFilePosition()+1;

    QFileInfo fi(file);
    fileToken.fileName = fi.fileName();
    fileToken.filePath = fi.absolutePath();

    fileListHolder->GetData().push_back(fileToken);
    fileModel->OnReloadDataFromInterface();
}

void luamwindow::RaiseIndex(int index)
{
    std::sort(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()),
              [](const FileSystemEntity& f1,const FileSystemEntity& f2 ){
            return f1.position < f2.position;
        });

    auto it = std::find_if(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()), [index](const FileSystemEntity& f){
        return f.position == index;
    });
    if(it == std::begin(fileListHolder->GetData()))
        return;
    int other = (it-1)->position;
    it->position = other;
    (it-1)->position = index;
    std::iter_swap(it, it-1);
}

void luamwindow::LowerIndex(int index)
{
    std::sort(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()),
              [](const FileSystemEntity& f1,const FileSystemEntity& f2 ){
            return f1.position < f2.position;
        });

    auto it = std::find_if(std::begin(fileListHolder->GetData()), std::end(fileListHolder->GetData()), [index](const FileSystemEntity& f){
        return f.position == index;
    });
    if(it == std::end(fileListHolder->GetData()))
        return;
    int other = (it+1)->position;
    (it)->position = other;
    (it+1)->position = index;
    std::iter_swap(it, it+1);
}

QModelIndex luamwindow::GetCurrentFileIndex()
{
    return ui->tbvFiles->currentIndex();
}

int luamwindow::GetLastFilePosition()
{
    return -1;
}

void luamwindow::MergeCurrentlyActiveFilesIntoFolder(QString inputFile, QString outputFolder, QString outputFileName)
{
    auto files = fileListHolder->GetData();
    std::sort(std::begin(files), std::end(files), [](const FileSystemEntity& f1, const FileSystemEntity& f2){
       return f1.position < f2.position;
    });

    QDir dir;
    QString tempOutputFolder = outputFolder + "/Temp_" + QUuid::createUuid().toString();
    dir.mkdir(tempOutputFolder);
    if(HasDuplicates(files))
    {
        QMessageBox::warning(nullptr, "Warning", "Some of the files selected for merging have duplicate names.");
        return;
    }
    QString fullOutputPath = outputFolder + "/" + outputFileName;
    QString backupPath = tempOutputFolder;
    QFileInfo fOld(fullOutputPath);
    if(fOld.exists())
    {
        FileSaveToken token;
        token.path = backupPath + "/" + outputFileName;
        token.originalPath = fullOutputPath;
        QFile::copy(fullOutputPath,token.path);
        undoBackups.push_back(token);
    }

    QFileInfo fi(inputFile);



    QFile::copy(inputFile,tempOutputFolder + "/" + fi.fileName());

    for(const FileSystemEntity& file: files)
    {
        if(!QFileInfo::exists(file.path) || !file.enabled)
            continue;
        QFileInfo fi(file.path);
        QFile::copy(file.path, tempOutputFolder + "/" + fi.fileName());
    }
    //auto storedPath = QDir::currentPath();
    QProcess proc;
    proc.execute("luam.exe", QStringList() << tempOutputFolder + "/" + fi.fileName() << outputFolder + "/" + outputFileName);
    auto errors = proc.readAllStandardError();
    QFile::copy(tempOutputFolder + "/" + outputFileName, outputFolder + "/" + outputFileName);
    ui->edtDiag->append(QString(errors));

}

bool luamwindow::HasDuplicates(const QList<luamwindow::FileSystemEntity>& list)
{
    QSet<QString> test;
    for(auto bit: list)
    {
        if(test.contains(bit.path))
            return true;
        test.insert(bit.path);
    }
    return false;
}
//static bool DoubleClickCatcher(QObject* /*obj*/, QEvent *event, QTableView* view, std::function<void()>callBack)
//{
//    if(event->type() == QEvent::MouseButtonDblClick)
//    {
//        QMouseEvent* me = (QMouseEvent *)event;
//        if(view->rowAt(me->pos().y()) == -1)
//            callBack();
//        return true;
//    }
//    return false;
//}


void luamwindow::SetupModelForFiles()
{
    fileListHolder = new TableDataListHolder<FileSystemEntity>();
    fileModel = new AdaptingTableModel();

    SetupModelAccessForFiles();

    fileListHolder->SetData(ReadFilesFromDatabase("base"));
    fileListHolder->SetColumns(QStringList() << "name" << "path" << "exists" << "enabled" );

    fileTableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(fileListHolder));

    fileModel->SetInterface(fileTableInterface);

    ui->tbvFiles->setModel(fileModel);

//    GenericEventFilter* eventFilter = new GenericEventFilter(this);
//    std::function<void()> callback = std::bind(&luamwindow::OnFilesTableDoubleClicked, this);
//    eventFilter->SetEventProcessor(std::bind(DoubleClickCatcher,std::placeholders::_1, std::placeholders::_2,
//                                             ui->tbvFiles,callback));
    //ui->tbvFiles->installEventFilter(eventFilter);
    connect(ui->tbvFiles, &CustomDblClickTableview::backgroundDoubleClickEvent, this, &luamwindow::OnFilesTableDoubleClicked);
    connect(ui->tbvFiles, &QTableView::doubleClicked, this, &luamwindow::OnFileDoubleClicked);
}

void luamwindow::SetupModelForFolders()
{
    folderListHolder = new TableDataListHolder<FileSystemEntity>();
    folderModel = new AdaptingTableModel();

    SetupModelAccessForFolders();

    //folderListHolder->SetData(ReadFilesFromDatabase("base"));
    folderListHolder->SetColumns(QStringList() << "path");

    folderTableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(folderListHolder));

    folderModel->SetInterface(folderTableInterface);

    ui->tbvFolders->setModel(folderModel);
    connect(ui->tbvFolders, &CustomDblClickTableview::backgroundDoubleClickEvent, this, &luamwindow::OnFoldersTableDoubleClicked);
    connect(ui->tbvFolders, &QTableView::doubleClicked, this, &luamwindow::OnFolderDoubleClicked);
}

void luamwindow::SaveFileListToDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery removalQuery(db);
    removalQuery.prepare("delete from files where set_id = :set_id");
    removalQuery.bindValue(":set_id", 0);
    removalQuery.exec();
    if(removalQuery.lastError().isValid())
        qDebug() << removalQuery.lastError();
    QSqlQuery inserterQuery(db);
    inserterQuery.prepare("insert into FILES (set_id, path, enabled, position)"
                          " VALUES (0,:path, :enabled, :position)");
    for(auto bit : fileListHolder->GetData())
    {
        inserterQuery.bindValue(":enabled", bit.enabled);
        inserterQuery.bindValue(":path", bit.path);
        inserterQuery.bindValue(":position", bit.position);
        inserterQuery.exec();
        if(inserterQuery.lastError().isValid())
            qDebug() << inserterQuery.lastError();
    }
    db.commit();
}

void luamwindow::SaveFolderListToDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery removalQuery(db);
    removalQuery.prepare("delete from folders");
    removalQuery.exec();
    if(removalQuery.lastError().isValid())
        qDebug() << removalQuery.lastError();
    QSqlQuery inserterQuery(db);
    inserterQuery.prepare("insert into folders (path) values (:path);");
    for(auto bit : folderListHolder->GetData())
    {
        inserterQuery.bindValue(":path", bit.path);
        inserterQuery.exec();
        if(inserterQuery.lastError().isValid())
            qDebug() << inserterQuery.lastError();
    }
    db.commit();
}
//(set_id integer default 0, path VARCHAR NOT NULL, enabled integer default 1, position integer);
void luamwindow::ReadFileListFromDatabase()
{
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("select * from files order by set_id, position asc");
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
    while(q.next())
    {
        FileSystemEntity e;
        e.enabled = q.value("ENABLED").toBool();
        e.path = q.value("PATH").toString();
        e.position = q.value("POSITION").toInt();
        e.exists = QFile::exists(e.path);
        QFileInfo fi(e.path);
        e.fileName = fi.fileName();
        e.filePath = fi.absolutePath();

        fileListHolder->GetData().push_back(e);
    }
    fileModel->SetInterface(fileTableInterface);
    fileModel->OnReloadDataFromInterface();
    ui->tbvFiles->setModel(fileModel);
    QSettings settings("Settings/settings.ini", QSettings::IniFormat);
    ui->tbvFiles->horizontalHeader()->restoreState(settings.value("Headers/files", QByteArray()).toByteArray());
}

void luamwindow::ReadFolderListFromDatabase()
{
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("select * from folders order by path asc");
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
    while(q.next())
    {
        folderListHolder->GetData().push_back(EntityFromPath(q.value("PATH").toString()));
    }
    folderModel->SetInterface(folderTableInterface);
    folderModel->OnReloadDataFromInterface();
    ui->tbvFolders->setModel(folderModel);
    QSettings settings("Settings/settings.ini", QSettings::IniFormat);
    ui->tbvFolders->horizontalHeader()->restoreState(settings.value("Headers/folders", QByteArray()).toByteArray());
}

QList<luamwindow::FileSystemEntity> luamwindow::ReadFilesFromDatabase(QString set)
{
    //todo
    QList<luamwindow::FileSystemEntity> result;
    return result;
}

void luamwindow::OnFolderDoubleClicked(const QModelIndex &index)
{
    QFileDialog d;
    d.setDirectory(index.data().toString());
    d.setFileMode(QFileDialog::ExistingFiles);
    auto result = d.exec();
    if(result != 1)
        return;
    for(auto file: d.selectedFiles())
    {
        ProcessFileIntoList(file);
    }
    SaveFileListToDatabase();
}

void luamwindow::on_pbPushIntoEasy_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(),
                                        ui->leAudioshieldFolder->text() + "/mods/Easy ShieldVR", "Easy ShieldVR.lua");
}

void luamwindow::on_pbPushIntoHard_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(),
                                        ui->leAudioshieldFolder->text() + "/mods/Hard ShieldVR", "Hard ShieldVR.lua");
}

void luamwindow::on_pbPushIntoElite_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(),
                                        ui->leAudioshieldFolder->text() + "/mods/Elite ShieldVR", "Elite ShieldVR.lua");
}

void luamwindow::on_pbPushIntoCasual_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(),
                                        ui->leAudioshieldFolder->text() + "/mods/Casual ShieldVR", "Casual ShieldVR.lua");
}

void luamwindow::on_pbPushIntoLegacyHard_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(),
                                        ui->leAudioshieldFolder->text() + "/mods/Normal ShieldVR", "Normal ShieldVR.lua");
}

void luamwindow::on_pbMergeToOutput_clicked()
{

    if(ui->leOutputFilename->text().isEmpty())
    {
        QMessageBox::warning(nullptr, "Warning", "Please provide a file name.");
        return;
    }
    if(ui->leOutputFolderFolder->text().isEmpty())
    {
        QMessageBox::warning(nullptr, "Warning", "Please provide a folder name.");
        return;
    }
    if(!QFileInfo::exists(ui->leOutputFolderFolder->text()))
    {
        QMessageBox::warning(nullptr, "Warning", "Provided folder does not exist.");
        return;
    }

    MergeCurrentlyActiveFilesIntoFolder(ui->leProtoFile->text(), ui->leOutputFolderFolder->text(), ui->leOutputFilename->text());
}
//QFile data(folder + "/" + fileName);
// if (!data.open(QFile::WriteOnly | QFile::Truncate))
// {
//    QMessageBox::warning(nullptr, "Warning", "Was not able to open output file for writing!");
//     return;
// }

#define ADD_STRING_GETTER(HOLDER,ROW,ROLE,PARAM)  \
HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
[] (const FileSystemEntity* data) \
{ \
    if(data) \
        return QVariant(data->PARAM); \
    else \
        return QVariant(); \
} \
);


void luamwindow::SetupModelAccessForFiles()
{
    ADD_STRING_GETTER(fileListHolder, 0, 0, fileName);
    ADD_STRING_GETTER(fileListHolder, 1, 0, filePath);
    ADD_STRING_GETTER(fileListHolder, 2, 0, exists);
    ADD_STRING_GETTER(fileListHolder, 3, 0, enabled);

    fileListHolder->AddFlagsFunctor(
                [](const QModelIndex& index)
    {
        Qt::ItemFlags result;
        result |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return result;
    }
    );
}

void luamwindow::SetupModelAccessForFolders()
{
    ADD_STRING_GETTER(folderListHolder, 0, 0, path);

    folderListHolder->AddFlagsFunctor(
                [](const QModelIndex& index)
    {
        Qt::ItemFlags result;
        result |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return result;
    }
    );
}

void luamwindow::on_pbAddFolder_clicked()
{
    QFileDialog d;
    d.setFileMode(QFileDialog::DirectoryOnly);
    auto result = d.exec();
    if(result != 1)
        return;

    auto folder = d.selectedFiles().at(0);
    AddFolderToCurrentSet(folder);
}
