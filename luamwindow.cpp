#include "luamwindow.h"
#include "ui_luamwindow.h"
#include <QFileDialog>
#include <QModelIndex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
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
}

luamwindow::~luamwindow()
{
    SaveFileListIntoDatabase();
    SaveFolderListIntoDatabase();
    delete ui;
}

void luamwindow::AddFolderToCurrentSet(QString folder)
{

}

void luamwindow::OnFoldersTableDoubleClicked(const QModelIndex &index)
{
    QFileDialog d;
    d.setFileMode(QFileDialog::DirectoryOnly);
    auto result = d.exec();
    if(result != 1)
        return;

    auto folder = d.selectedFiles().at(0);
    AddFolderToCurrentSet(folder);
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
    QModelIndex index = GetCurrentIndex();
    FileSystemEntity* data = static_cast<FileSystemEntity*>(index.internalPointer());
    RaiseIndex(data->position);
}

void luamwindow::on_pdDown_clicked()
{
    QModelIndex index = GetCurrentIndex();
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
    SaveFileListIntoDatabase();
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
    fileToken.exists == QFileInfo::exists(file);
    fileToken.path = file;
    fileToken.position = GetLastFilePosition()+1;
    fileListHolder->GetData().push_back(fileToken);

}

int luamwindow::GetLastFilePosition()
{
    return -1;
}

void luamwindow::SaveFileListIntoDatabase()
{

}

void luamwindow::SaveFolderListIntoDatabase()
{

}

void luamwindow::MergeCurrentlyActiveFilesIntoFolder(QString inputFile, QString outputFolder, QString outputFileName)
{
    auto files = fileListHolder->GetData();
    std::sort(std::begin(files), std::end(files), [](const FileSystemEntity& f1, const FileSystemEntity& f2){
       return f1.position < f2.position;
    });

    QDir dir;
    dir.mkdir("Save_" + QUuid::createUuid().toString());

    if(HasDuplicates(files))
    {
        QMessageBox::warning(nullptr, "Warning", "Some of the files selected for merging have duplicate names.");
        return;
    }

    for(const FileSystemEntity& file: files)
    {
        if(!QFileInfo::exists(file.path) || !file.enabled)
            continue;
        QFileInfo fi(file.path);
        QFile::copy(file.path,outputFolder + "/" + fi.fileName());
    }
    QProcess proc;
    proc.execute("luam.exe", QStringList() << inputFile << outputFolder + "/" + outputFileName);
    auto errors = proc.readAllStandardError();
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

void luamwindow::SetupModelForFiles()
{
    fileListHolder = new TableDataListHolder<Builtin>();
    fileModel = new AdaptingTableModel();

    SetupModelAccess();

    fileListHolder->SetData(ReadFilesFromDatabase("base"));
    fileListHolder->SetColumns(QStringList() << "exists" << "enabled" << "name" << "path");

    fileTableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(fileListHolder));

    fileModel->SetInterface(fileTableInterface);

    ui->tbvFiles->setModel(fileModel);
}

void luamwindow::SaveFileListToDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery removalQuery(db);
    removalQuery.prepare("delete from files where set_id = :set_id");
    removalQuery.bindValue(":set_id", 0);
    removalQuery.exec();

    QSqlQuery inserterQuery(db);
    inserterQuery.prepare("begin insert into files(set_id, enabled, position) values(0, :enabled, :position); end;");
    for(auto bit : fileListHolder->GetData())
    {
        inserterQuery.bindValue(":enabled", bit.enabled);
        inserterQuery.bindValue(":position", bit.position);
        inserterQuery.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
    }
    db.commit();
}

void luamwindow::SaveFolderListToDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery removalQuery(db);
    removalQuery.prepare("delete from folders");
    removalQuery.bindValue(":set_id", 0);
    removalQuery.exec();

    QSqlQuery inserterQuery(db);
    inserterQuery.prepare("begin insert into folders(path) values(:path); end;");
    for(auto bit : folderListHolder->GetData())
    {
        inserterQuery.bindValue(":path", bit.path);
        inserterQuery.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
    }
    db.commit();
}

void luamwindow::ReadFileListFromDatabase()
{

}

void luamwindow::ReadFolderListFromDatabase()
{

}

QList<luamwindow::FileSystemEntity> luamwindow::ReadFilesFromDatabase(QString set)
{
    QList<luamwindow::FileSystemEntity> result;
    return result;
}

void luamwindow::on_pbPushIntoEasy_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leAudioshieldFolder->text(), "Easy ShieldVR.lua");
}

void luamwindow::on_pbPushIntoHard_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leAudioshieldFolder->text(), "Hard ShieldVR.lua");
}

void luamwindow::on_pbPushIntoElite_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leAudioshieldFolder->text(), "Elite ShieldVR.lua");
}

void luamwindow::on_pbPushIntoCasual_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leAudioshieldFolder->text(), "Casual ShieldVR.lua");
}

void luamwindow::on_pbPushIntoLegacyHard_clicked()
{
    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leAudioshieldFolder->text(), "Normal ShieldVR.lua");
}

void luamwindow::on_pbMergeToOutput_clicked()
{

    if(ui->leOutputFilename->text().empty())
    {
        QMessageBox::warning(nullptr, "Warning", "Please provide a file name.");
        return;
    }
    if(ui->leOutputFolderFolder->text().empty())
    {
        QMessageBox::warning(nullptr, "Warning", "Please provide a folder name.");
        return;
    }
    if(!QFileInfo::exists(ui->leOutputFolderFolder->text()))
    {
        QMessageBox::warning(nullptr, "Warning", "Provided folder does not exist.");
        return;
    }

    MergeCurrentlyActiveFilesIntoFolder(ui.leProtoFile->text(), ui->leOutputFolder->text(), ui->leOutputFilename->text());
}
//QFile data(folder + "/" + fileName);
// if (!data.open(QFile::WriteOnly | QFile::Truncate))
// {
//    QMessageBox::warning(nullptr, "Warning", "Was not able to open output file for writing!");
//     return;
// }

#define ADD_STRING_GETTER(HOLDER,ROW,ROLE,PARAM)  \
HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
[] (const Builtin* data) \
{ \
    if(data) \
        return QVariant(data->PARAM); \
    else \
        return QVariant(); \
} \
);


void luamwindow::SetupModelAccess()
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
