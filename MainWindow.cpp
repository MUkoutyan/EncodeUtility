/* =====================================================
* EncodeUtility
* Copyright (C) 2019 Koutyan
*
* EncodeUtility is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ===================================================== */

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QLabel>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QPixmap>
#include <QProcess>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTextStream>

#include <QDebug>

struct ProjectDefines
{
    static constexpr char projectExtention[] = ".encproj";
    static constexpr int  projectVersionNum  = 0x010001;
    static constexpr char projectVersion[]   = "1.0.1";
    static constexpr char settingOutputFolder[] = "OutputFolder";
    static constexpr char settingCheckQaacFile[] = "CheckqaacFile";
    static constexpr char settingCheckLameFile[] = "CheckLameFile";
#ifdef WIN64
    static constexpr char qaacEXEFileName[] = "qaac64.exe";
#else
    static constexpr char qaacEXEFileName[] = "qaac32.exe";
#endif
    static constexpr char lameEXEFileName[] = "lame.exe";
    static const QStringList headerItems;
};
const QStringList ProjectDefines::headerItems = {"No.", "Title", "Artist", "AlbumTitle", "AlbumArtist", "Composer", "Group", "Genre", "Year"};

enum TableColumn
{
    TrackNo = 0,
    Title,
    Artist,
    AlbumTitle,
    AlbumArtist,
    Composer,
    Group,
    Genre,
    Year,
    ALL
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , metadataTable(nullptr)
    , aboutLabel(new QLabel(tr("drag&drop .wav files \n or \n jacket(.png or .jpg) file here."), this))
    , settingFilePath(qApp->applicationDirPath()+"/setting.ini")
    , qaacPath(qApp->applicationDirPath()+"/"+ProjectDefines::qaacEXEFileName)
    , lamePath(qApp->applicationDirPath()+"/"+ProjectDefines::lameEXEFileName)
    , mp3OutputPath("mp3")
    , m4aOutputPath("m4a")
    , wavOutputPath("wav")
    , imageOutputPath("")
    , processed_count(0)
    , checkQaacFile(true)
    , checkLameFile(true)
    , settings(new DialogAppSettings(this))
    , widgetListDisableDuringEncode({})
    , lastLoadProject("")
{
    ui->setupUi(this);

    auto toolVer = this->ui->menuAbout->addAction("Version 1.0.2");
    toolVer->setEnabled(false);
    auto projVer = this->ui->menuAbout->addAction(QString("Project Version ") + ProjectDefines::projectVersion);
    projVer->setEnabled(false);

    //スタイルシートの設定
    QFile file(":qss/style/style.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString text;
        text = in.readAll();
        this->setStyleSheet(text);
    }
    file.close();

    widgetListDisableDuringEncode = {
        this->ui->encodeButton, this->ui->tableWidget,
        this->ui->outputMp3,    this->ui->baseFolder1, this->ui->mp3OutputPath,
        this->ui->outputM4a,    this->ui->baseFolder2, this->ui->m4aOutputPath,
        this->ui->outputWav,    this->ui->baseFolder3, this->ui->wavOutputPath,
        this->ui->includeImage, this->ui->baseFolder4, this->ui->imageOutputPath,
        this->ui->outputFolderPath,
        this->ui->check_addTrackNo, this->ui->label_2, this->ui->track_no_delimiter, this->ui->label_3, this->ui->num_of_digit
    };

    this->tableMenu = new QMenu(this->ui->tableWidget);
    this->artworkMenu = new QMenu(this->ui->artwork);

    this->aboutLabel->setAlignment(Qt::AlignCenter);

    this->setAcceptDrops(true); //D&D許可
    this->ui->verticalLayout->addWidget(this->aboutLabel, 6, Qt::AlignHCenter); //説明文の表示
    this->ui->tabWidget->setHidden(true);   //編集Widgetはこの時点で表示しない
    this->ui->artwork->setVisible(false);

    //メニュー
    connect(this->ui->actionSave_file, &QAction::triggered, [this]()
    {
        QString savefilePath = this->lastLoadProject;
        if(savefilePath.isEmpty()){
           savefilePath = QFileDialog::getSaveFileName(this, tr("Save Project File"),QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0],
                                                             tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        }
        this->SaveProjectFile(savefilePath);
    });
    connect(this->ui->actionSave_as, &QAction::triggered, [this]()
    {
        auto savefilePath = QFileDialog::getSaveFileName(this, tr("Save Project File"),QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0],
                                                               tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        this->SaveProjectFile(savefilePath);
    });


    connect(this->ui->actionLoad_file, &QAction::triggered, [this]()
    {
        auto location = this->lastLoadProject;
        if(location.isEmpty()){
            location = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
        }
        auto openfilePath = QFileDialog::getOpenFileName(this, tr("Open Project File"), location,
                                                               tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        settings->SetLastOpenedDirectory(openfilePath);
        this->openFiles({openfilePath});
    });

    //出力先の取得と表示
    connect(this->ui->outputFolderPath, &QLineEdit::textChanged, [this](QString path){
        this->ui->baseFolder1->setText(path+"/");
        this->ui->baseFolder2->setText(path+"/");
        this->ui->baseFolder3->setText(path+"/");
        this->ui->baseFolder4->setText(path+"/");
    });
    auto filePathList = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    QString homeDir = filePathList[0];
    this->ui->outputFolderPath->setText(homeDir+"/EncodeUtilityFolder");

    //メタデータが編集された
    connect(this->ui->tableWidget, &QTableWidget::itemChanged, [this](QTableWidgetItem* item)
    {
        //選択されている箇所を全て変更する
        auto items = this->ui->tableWidget->selectedItems();
        for(QTableWidgetItem* selectItem : items)
        {
            selectItem->setData(Qt::DisplayRole, item->data(Qt::DisplayRole));
        }
    });

    connect(this->ui->encodeButton, &QPushButton::clicked, this, &MainWindow::Encode);

    connect(this->ui->expandOption, &QPushButton::clicked, [this]()
    {
        auto ChangeVisible = [](QWidget* w){ w->setVisible(!w->isVisible()); };
        ChangeVisible(this->ui->outputMp3);
        ChangeVisible(this->ui->baseFolder1);
        ChangeVisible(this->ui->mp3OutputPath);

        ChangeVisible(this->ui->outputM4a);
        ChangeVisible(this->ui->baseFolder2);
        ChangeVisible(this->ui->m4aOutputPath);

        ChangeVisible(this->ui->outputWav);
        ChangeVisible(this->ui->baseFolder3);
        ChangeVisible(this->ui->wavOutputPath);

        ChangeVisible(this->ui->includeImage);
        ChangeVisible(this->ui->baseFolder4);
        ChangeVisible(this->ui->imageOutputPath);

        ChangeVisible(this->ui->check_addTrackNo);
        ChangeVisible(this->ui->label_2);
        ChangeVisible(this->ui->track_no_delimiter);
        ChangeVisible(this->ui->label_3);
        ChangeVisible(this->ui->num_of_digit);
    });

    connect(this->ui->m4aOutputPath, &QLineEdit::textEdited, [this](QString text){
        this->m4aOutputPath = std::move(text);
    });
    connect(this->ui->mp3OutputPath, &QLineEdit::textEdited, [this](QString text){
        this->mp3OutputPath = std::move(text);
    });
    connect(this->ui->wavOutputPath, &QLineEdit::textEdited, [this](QString text){
        this->wavOutputPath = std::move(text);
    });
    connect(this->ui->imageOutputPath, &QLineEdit::textEdited, [this](QString text){
        this->imageOutputPath = std::move(text);
    });
    this->ui->m4aOutputPath->setText(this->m4aOutputPath);
    this->ui->mp3OutputPath->setText(this->mp3OutputPath);
    this->ui->wavOutputPath->setText(this->wavOutputPath);
    this->ui->imageOutputPath->setText(this->imageOutputPath);

    connect(this->ui->outputM4a,    &QCheckBox::clicked, [this](bool checked){
        this->CheckEnableEncodeButton();
        this->ui->baseFolder2->setEnabled(checked);
        this->ui->m4aOutputPath->setEnabled(checked);
    });
    connect(this->ui->outputMp3,    &QCheckBox::clicked, [this](bool checked){
        this->CheckEnableEncodeButton();
        this->ui->baseFolder1->setEnabled(checked);
        this->ui->mp3OutputPath->setEnabled(checked);
    });
    connect(this->ui->outputWav,    &QCheckBox::clicked, [this](bool checked){
        this->CheckEnableEncodeButton();
        this->ui->baseFolder3->setEnabled(checked);
        this->ui->wavOutputPath->setEnabled(checked);
    });
    connect(this->ui->includeImage, &QCheckBox::clicked, [this](bool checked){
        this->CheckEnableEncodeButton();
        this->ui->baseFolder4->setEnabled(checked);
        this->ui->imageOutputPath->setEnabled(checked);
    });

    connect(this->ui->selectFolder, &QToolButton::clicked, [this]()
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                     this->ui->outputFolderPath->text(),
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        this->ui->outputFolderPath->setText(dir);
    });
    connect(this->ui->openFolder, &QToolButton::clicked, [this](){
        QDesktopServices::openUrl(QUrl(this->ui->outputFolderPath->text()));
    });

    QAction* delete_artwork_action = this->artworkMenu->addAction(tr("Delete Artwork"));
    this->ui->artwork->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->ui->artwork, &QLabel::customContextMenuRequested, [this, delete_artwork_action](const QPoint&)
    {
        //選択しているアイテムが無ければメニューを無効
        delete_artwork_action->setEnabled(this->artworkPath != "");
        this->artworkMenu->exec(QCursor::pos());
    });
    connect(delete_artwork_action, &QAction::triggered, [this]()
    {
        this->artworkPath = "";
        this->ui->artwork->setPixmap(QPixmap());
        this->ui->artwork->setVisible(false);
    });

    //セルクリック時の右クリックメニュー
    QAction* delete_row_action = this->tableMenu->addAction(tr("Delete Row"));
    this->ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->ui->tableWidget, &QTableWidget::customContextMenuRequested, [this, delete_row_action](const QPoint&)
    {
        //選択しているアイテムが無ければメニューを無効
        delete_row_action->setEnabled(this->ui->tableWidget->selectedItems().size() > 0);
        this->tableMenu->exec(QCursor::pos());
    });
    connect(delete_row_action, &QAction::triggered, [this]()
    {
        auto items = this->ui->tableWidget->selectedItems();
        for(QTableWidgetItem* selectItem : items){
            this->ui->tableWidget->removeRow(selectItem->row());
        }
    });

    connect(this->ui->actionClear_All_Items, &QAction::triggered, [this]()
    {
        int size = this->ui->tableWidget->rowCount();
        for(int row=0; row<size; ++row){
            this->ui->tableWidget->removeRow(0);
        }
    });

    connect(this->ui->actionSettings, &QAction::triggered, [this](){
        this->settings->show();
        this->settings->raise();
    });

    connect(this->ui->actionCheck_Encoder, &QAction::triggered, [this](){
        if(this->CheckEncoder()){
            //エンコーダーを正しく認識しています。
            QMessageBox::information(this, tr("Check Encoder"), tr("The encoder is correctly identified."));
        }
    });

    //設定ファイルの読み込み
    LoadSettingFile();

    //エンコーダーの存在チェック
    CheckEncoder();
}

bool MainWindow::CheckEncoder()
{
    auto CheckExistsFile = [](QStringList list){
        bool hit = true;
        for(auto& url : list){ hit &= QFile::exists(qApp->applicationDirPath()+"/"+url); }
        return hit;
    };

    const QStringList lameFiles = {ProjectDefines::lameEXEFileName, "lame_enc.dll"};
    if(this->checkLameFile)
    {
        bool isExistsLame = CheckExistsFile(lameFiles);
        if(isExistsLame == false)
        {
            //mp3エンコードにはLameが必要です。 ダウンロードリンクを開きますか？
            QCheckBox* checkBox = new QCheckBox(tr("Don't show again."));
            connect(checkBox, &QCheckBox::stateChanged, this, [this](int state){
                this->checkLameFile = static_cast<Qt::CheckState>(state) != Qt::CheckState::Checked;
                this->SaveSettingFile(ProjectDefines::settingCheckLameFile, this->checkLameFile);
            });
            QMessageBox msg;
            msg.setWindowTitle(tr("Not found Lame"));
            msg.setText(tr("Lame is required for mp3 encoding. Do you want to open a download link?"));
            msg.setIcon(QMessageBox::Icon::Question);
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::No);
            msg.setDefaultButton(QMessageBox::Yes);
            msg.setCheckBox(checkBox);

            if(msg.exec() == QMessageBox::Yes)
            {
                QString url = "http://rarewares.org/mp3-lame-bundle.php";
                if(QDesktopServices::openUrl(QUrl(url))){
                    QDesktopServices::openUrl(QUrl(qApp->applicationDirPath()));
                    QMessageBox::information(this, "", tr("Place \"lame.exe\" and \"lame_enc.dll\" in the same location as EncodeUtility.exe."), QMessageBox::Ok);
                }
                else{
                    QMessageBox::critical(this, tr("Can't open URL"), tr("Can't open URL. ") + url, QMessageBox::Ok);
                }
            }
        }
    }
    const bool existsLame = CheckExistsFile(lameFiles);
    this->ui->outputMp3->setEnabled(existsLame);
    this->ui->outputMp3->setChecked(existsLame);

    const QStringList qaacList = {"libsoxconvolver64.dll", "libsoxr64.dll", ProjectDefines::qaacEXEFileName};
    if(this->checkQaacFile)
    {
        bool isExistsqaac = CheckExistsFile(qaacList);
        if(isExistsqaac == false)
        {
            QCheckBox* checkBox = new QCheckBox(tr("Don't show again."));
            connect(checkBox, &QCheckBox::stateChanged, this, [this](int state){
                this->checkQaacFile = static_cast<Qt::CheckState>(state) != Qt::CheckState::Checked;
                this->SaveSettingFile(ProjectDefines::settingCheckQaacFile, this->checkQaacFile);
            });
            QMessageBox msg;
            msg.setWindowTitle(tr("Not found qaac"));
            msg.setText(tr("qaac is required for m4a encoding. Do you want to open a download link?"));
            msg.setIcon(QMessageBox::Icon::Question);
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::No);
            msg.setDefaultButton(QMessageBox::Yes);
            msg.setCheckBox(checkBox);
            //mp3エンコードにはLameが必要です。 ダウンロードリンクを開きますか？
            if(msg.exec() == QMessageBox::Yes)
            {
                QString url = "https://sites.google.com/site/qaacpage/cabinet";
                if(QDesktopServices::openUrl(QUrl(url))){
                    QDesktopServices::openUrl(QUrl(qApp->applicationDirPath()));
                    QMessageBox::information(this, "", tr("Place \"libsoxconvolver64.dll\", \"libsoxr64.dll\" and \"%1\"in the same location as EncodeUtility.exe.").arg(ProjectDefines::qaacEXEFileName), QMessageBox::Ok);
                }
                else{
                    QMessageBox::critical(this, tr("Can't open URL"), tr("Can't open URL. ") + url, QMessageBox::Ok);
                }
            }
        }
    }
    const bool existsQaac = CheckExistsFile(qaacList);
    this->ui->outputM4a->setEnabled(existsQaac);
    this->ui->outputM4a->setChecked(existsQaac);

    return existsLame && existsQaac;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    //URLがあれば通す
    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (QUrl url : urlList){
            pathList.append(url.toLocalFile());
        }
        openFiles(pathList);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    //URLがあれば通す
    if(mimeData->hasUrls()){
        event->acceptProposedAction();
    }
}

void MainWindow::openFiles(QStringList pathList)
{
    this->aboutLabel->setHidden(true);
    this->ui->tabWidget->setHidden(false);
    int row = this->ui->tableWidget->rowCount();
    for(QString path : pathList)
    {
        const QString extension = QFileInfo(path).suffix().toUpper();

        if(extension == "ENCPROJ"){
            this->loadProjectFile(std::move(path));
            break;
        }

        if(extension == "PNG" || extension == "JPG")
        {
            this->artworkPath = path;
            QPixmap artwork = QPixmap(this->artworkPath);
            this->ui->artwork->setVisible(true);
            this->ui->artwork->setPixmap(artwork.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            continue;
        }

        if(extension != "WAV"){ continue; }

        QString track_no = QString("%1").arg(row);
        QString title = path.mid(path.lastIndexOf("/")+1).section(".", 0, 0);
        QString albumTitle = "";
        QString artist = "";
        QString genre = "";

        //区切り文字で分けられればメタデータをファイル名から取得
        QStringList metaDatas = title.split('_');
        const int size = metaDatas.size();
        if(size > 0)
        {
            //意図的なフォールスルー
            switch(size-1)
            {
            case TableColumn::Genre:
                genre = metaDatas[4];
            case TableColumn::AlbumTitle:
                albumTitle = metaDatas[3];
            case TableColumn::Artist:
                artist = metaDatas[2];
            case TableColumn::Title:
                title = metaDatas[1];
            case TableColumn::TrackNo:
            {
                QString buf = metaDatas[0];
                bool isOk = false;
                int num = buf.toInt(&isOk);
                if(isOk){
                    track_no = QString("%1").arg(num);
                }
            }
                break;
            default:
                break;
            }
        }

        //行挿入 & 列挿入 選択項目の一斉変更が効かなくなるのでアイテムは空でも必ずセットする
        this->ui->tableWidget->insertRow(row);
        this->ui->tableWidget->setItem(row, TableColumn::TrackNo, new QTableWidgetItem(track_no));
        {
            QTableWidgetItem* item = new QTableWidgetItem(title);
            item->setData(Qt::UserRole, QVariant(path));
            this->ui->tableWidget->setItem(row, TableColumn::Title,  item);
        }
        this->ui->tableWidget->setItem(row, TableColumn::Artist,     new QTableWidgetItem(artist));
        this->ui->tableWidget->setItem(row, TableColumn::AlbumTitle, new QTableWidgetItem(albumTitle));
        this->ui->tableWidget->setItem(row, TableColumn::AlbumArtist,new QTableWidgetItem());
        this->ui->tableWidget->setItem(row, TableColumn::Group,      new QTableWidgetItem());
        this->ui->tableWidget->setItem(row, TableColumn::Genre,      new QTableWidgetItem(genre));
        this->ui->tableWidget->setItem(row, TableColumn::Composer,   new QTableWidgetItem());
        this->ui->tableWidget->setItem(row, TableColumn::Year,       new QTableWidgetItem());
        row++;
    }

    this->ui->tableWidget->resizeColumnsToContents();

    //項目があればエンコードボタンを有効
    if(this->ui->tableWidget->rowCount() > 0){
        this->ui->encodeButton->setEnabled(true);
        this->ui->actionSave_as->setEnabled(true);
        this->ui->actionSave_file->setEnabled(true);
    }
}

void MainWindow::SaveProjectFile(QString saveFilePath)
{
    // project version
    // output path
    // image path

    // ### Ver.1.0.2
    // addTrackNo Flag
    // delimiter
    // fill digit

    // no., title, artist, albumtitle...
    // no., title, artist, albumtitle...
    if(saveFilePath.isEmpty()){ return; }

    QStringList tableList;
    tableList.append(QString::number(ProjectDefines::projectVersionNum));
    tableList.append(this->ui->outputFolderPath->text());
    tableList.append(this->artworkPath);

    tableList.append(QVariant(this->ui->check_addTrackNo->isChecked()).toString());
    tableList.append(this->ui->track_no_delimiter->text());
    tableList.append(QString::number(this->ui->num_of_digit->value()));

    const int row = this->ui->tableWidget->rowCount();
    const int col = this->ui->tableWidget->columnCount();
    for(int i=0; i<row; ++i)
    {
        for(int j=0; j<col; ++j)
        {
            if(j == Title){
                tableList.append(this->ui->tableWidget->item(i,j)->data(Qt::UserRole).toString());
            }
            else{
                tableList.append(this->ui->tableWidget->item(i,j)->text());
            }
        }
    }

    QFile file(saveFilePath);
    if(file.open(QFile::WriteOnly)){
        file.write(tableList.join(',').toLocal8Bit());
    }
    this->lastLoadProject = saveFilePath;

    this->ui->statusBar->showMessage(tr("File saved."), 3000);
}

void MainWindow::CheckEnableEncodeButton()
{
    if(this->ui->tableWidget->rowCount() == 0){ return; }
    bool isEnable = false;
    isEnable |= this->ui->outputM4a->isChecked();
    isEnable |= this->ui->outputMp3->isChecked();
    isEnable |= this->ui->outputWav->isChecked();
    isEnable |= this->ui->includeImage->isChecked();
    this->ui->encodeButton->setEnabled(isEnable);
}

void MainWindow::SaveSettingFile(QString key, QVariant value)
{
    QSettings settingfile(settingFilePath, QSettings::IniFormat);
    settingfile.setValue(key, value);
}

void MainWindow::LoadSettingFile()
{
    QSettings settingfile(settingFilePath, QSettings::IniFormat);
    this->ui->outputFolderPath->setText(settingfile.value(ProjectDefines::settingOutputFolder, this->ui->outputFolderPath->text()).toString());
    this->checkQaacFile = settingfile.value(ProjectDefines::settingCheckQaacFile, true).toBool();
    this->checkLameFile = settingfile.value(ProjectDefines::settingCheckLameFile, true).toBool();
}

void MainWindow::loadProjectFile(QString projFilePath)
{
    if(projFilePath.isEmpty()){ return; }

    QFile file(projFilePath);
    if(file.open(QFile::ReadOnly) == false){ return; }

    this->ui->tableWidget->clear();
    this->ui->tableWidget->setRowCount(0);
    this->ui->outputFolderPath->setText("");
    this->artworkPath = "";

    this->ui->tableWidget->setColumnCount(ALL);
    this->ui->tableWidget->setHorizontalHeaderLabels(ProjectDefines::headerItems);

    auto data = file.readAll();
    auto strList = QString::fromLocal8Bit(data).split(",");
    int index = 0;
    auto projVersion = strList[index++].toInt();
    this->ui->outputFolderPath->setText(strList[index++]);
    this->artworkPath = strList[index++];

    if(0x010001 <= projVersion){
        this->ui->check_addTrackNo->setChecked(QVariant(strList[index++]).toBool());
        this->ui->track_no_delimiter->setText(strList[index++]);
        this->ui->num_of_digit->setValue(strList[index++].toInt());
    }

    int row = 0;
    const int size = strList.size();
    while(index < size)
    {
        this->ui->tableWidget->insertRow(row);
        for(int i=0; i<ALL; ++i)
        {
            QTableWidgetItem* item = nullptr;
            if(i == Title){
                auto path = strList[index];
                auto title = path.mid(path.lastIndexOf("/")+1).section(".", 0, 0);
                item = new QTableWidgetItem();

                item->setText(title);
                QStringList metaDatas = title.split('_');
                if(metaDatas.size() > 1){
                    item->setText(metaDatas[1]);
                }
                item->setData(Qt::UserRole, path);
            }
            else{
                item = new QTableWidgetItem(strList[index]);
            }
            this->ui->tableWidget->setItem(row, i, item);
            ++index;
        }
        row++;
    }

    this->lastLoadProject = projFilePath;
}

void MainWindow::Encode()
{
    this->ui->statusBar->showMessage(tr("Start Encoding."));

    this->processed_count = 0;
    //エンコード中にエンコードさせないようにするためボタンを無効
    for(auto widget : widgetListDisableDuringEncode){ widget->setEnabled(false); }
    this->ui->tabWidget->setCurrentIndex(1);    //ログウィジェットを表示

    const QString outputFolder = this->ui->outputFolderPath->text();
    //出力先フォルダを作成 エンコーダーの有無・出力チェックの状態に応じてフォルダを作成
    QDir().mkdir(outputFolder);
    if(this->ui->outputM4a->isChecked()){ QDir().mkdir(outputFolder+"/m4a"); }
    if(this->ui->outputWav->isChecked()){ QDir().mkdir(outputFolder+"/"+wavOutputPath); }
    if(this->ui->outputMp3->isChecked()){ QDir().mkdir(outputFolder+"/mp3"); }
    if(this->ui->includeImage->isChecked())
    {
        QDir().mkdir(outputFolder+"/"+imageOutputPath);
        //jacketのコピー
        QFile::copy(this->artworkPath, outputFolder+"/"+imageOutputPath+this->artworkPath.mid(this->artworkPath.lastIndexOf("/")));
    }

#if defined(Q_OS_MAC)
    MacEncodeProcess();
#elif defined(Q_OS_WIN)
    WindowsEncodeProcess();
#endif
}

void MainWindow::EncodeProcess(QString execute, QString title, int num_encoding_music)
{
    QProcess* process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardOutput, [=](){
        QByteArray arr = process->readAllStandardOutput();
        this->ui->logWidget->insertPlainText(QString(arr));
    });
    connect(process, &QProcess::readyReadStandardError, [=](){
        QByteArray arr = process->readAllStandardError();
        this->ui->logWidget->insertPlainText(QString(arr));
        this->ui->logWidget->verticalScrollBar()->setValue(this->ui->logWidget->verticalScrollBar()->maximum());
    });
    connect(process, &QProcess::readChannelFinished, [=]()
    {
        this->processed_count++;

        this->ui->statusBar->showMessage(tr("Finish Encoding. %1/%2").arg(this->processed_count+1).arg(num_encoding_music));
        this->ui->logWidget->insertPlainText("finish : "+title+"\n");

        if(this->processed_count >= num_encoding_music)
        {
            //全部エンコードしたらエンコードボタンを有効にしてメタテーブルに表示を戻す
            this->ui->statusBar->showMessage(tr("Complete."));
            this->ui->logWidget->insertPlainText(tr("Complete."));
            for(auto widget : widgetListDisableDuringEncode){ widget->setEnabled(true); }
            this->ui->tabWidget->setCurrentIndex(0);
        }
    });

    qDebug() << "execute : " << execute;
    process->start(execute);
    if (!process->waitForStarted(-1)) {
        qWarning() << process->errorString();
    }
}

void MainWindow::WindowsEncodeProcess()
{
    const bool isExistsqaac = QFile::exists(qaacPath);
    const bool isExistsLame = QFile::exists(lamePath);
    const QString outputFolder = this->ui->outputFolderPath->text();
    const int size = this->ui->tableWidget->rowCount();
    const int num_encoding_music = [&]()
    {
        int result = 0;
        if(this->ui->outputMp3->isChecked()){
            result += size;
        }
        if(this->ui->outputM4a->isChecked()){
            result += size;
        }
        return result;
    }();

    auto GetOutputPath = [&](QString encodePath, QString title, QString extension, int i) -> QString
    {
        QString outputFile = outputFolder+"/"+ encodePath +"/"+title+extension;
        if(this->ui->check_addTrackNo->isChecked()){
            outputFile = outputFolder+"/"+ encodePath +"/"+QString("%1%2%3").arg(i+1, this->ui->num_of_digit->value()).arg(this->ui->track_no_delimiter->text()).arg(title)+extension;
        }
        return outputFile;
    };

    for(int i=0; i<size; ++i)
    {
        QString inputPath   = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::UserRole).toString();
        QString title       = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::DisplayRole).toString();
        QString track_no    = this->ui->tableWidget->item(i, TableColumn::TrackNo)->data(Qt::DisplayRole).toString();
        QString artist      = this->ui->tableWidget->item(i, TableColumn::Artist)->data(Qt::DisplayRole).toString();
        QString album_title = this->ui->tableWidget->item(i, TableColumn::AlbumTitle)->data(Qt::DisplayRole).toString();
        QString genre       = this->ui->tableWidget->item(i, TableColumn::Genre)->data(Qt::DisplayRole).toString();
        QString album_artist= this->ui->tableWidget->item(i, TableColumn::AlbumArtist)->data(Qt::DisplayRole).toString();
        QString group       = this->ui->tableWidget->item(i, TableColumn::Group)->data(Qt::DisplayRole).toString();
        QString composer    = this->ui->tableWidget->item(i, TableColumn::Composer)->data(Qt::DisplayRole).toString();
        QString year        = this->ui->tableWidget->item(i, TableColumn::Year)->data(Qt::DisplayRole).toString();

        //トラック番号をファイル名に含めるか

        if(isExistsqaac == false){
            this->ui->statusBar->showMessage(tr("Not found %1").arg(ProjectDefines::qaacEXEFileName), 3000);
            this->processed_count++;
        }
        else if(this->ui->outputM4a->isChecked())
        {
            QString outputFile = GetOutputPath(m4aOutputPath, title, ".m4a", i);

            // ======== AAC ========
            QStringList aac_option;
            aac_option << " -c " << QString::number(320);
            if(title!=""){ aac_option << " --title " << "\"" <<  title <<  "\""; }
            if(artist!=""){ aac_option << " --artist " <<      "\""<<  artist <<  "\""; }
            if(album_title!=""){ aac_option << " --album " <<       "\""<<  album_title<<  "\""; }
            if(album_artist!=""){ aac_option << " --band " <<        "\""<<  album_artist<<  "\""; }
            if(composer!=""){ aac_option << " --composer " <<    "\""<<  composer<<  "\""; }
            if(genre!=""){ aac_option << " --genre " <<       "\""<<  genre<<  "\""; }
            if(year!=""){ aac_option << " --date " <<        "\""<<  year<<  "\""; }
            aac_option << " --track " <<       "\""<<  track_no<<  "/"<<  QString("%1").arg(size)<<  "\"";
            aac_option << " --disk " <<        "\"1/1\"";
            if(this->artworkPath!=""){ aac_option << " --artwork " <<     "\""<<  this->artworkPath<<  "\""; }
            aac_option << " \"" <<  inputPath <<      "\" ";
            aac_option << " -o " <<  "\""<< outputFile <<  "\"";

            //実行
            QString aac_args = qaacPath + aac_option.join("");
            EncodeProcess(aac_args, title, num_encoding_music);
            this->ui->logWidget->insertPlainText(tr("start aac encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));
        }

        // ======== MP3 ========
        //トラック番号をファイル名に含めるか
        if(isExistsLame == false){
            this->ui->statusBar->showMessage(tr("Not found lame.exe"), 3000);
            this->processed_count++;
        }
        else if(this->ui->outputMp3->isChecked())
        {
            QString outputFile = GetOutputPath(mp3OutputPath, title, ".mp3", i);
            QStringList mp3_option;
            mp3_option << " -b " << QString::number(320);
            if(title!=""){ mp3_option << " --tt " << "\"" <<  title <<  "\""; }
            if(artist!=""){ mp3_option << " --ta " <<      "\""<<  artist <<  "\""; }
            if(album_title!=""){ mp3_option << " --tl " <<       "\""<<  album_title<<  "\""; }
            if(genre!=""){ mp3_option << " --tg " <<       "\""<<  genre<<  "\""; }
            if(year!=""){ mp3_option << " --ty " <<        "\""<<  year<<  "\""; }
            mp3_option << " --tn " <<       "\""<<  track_no<<  "/"<<  QString("%1").arg(size)<<  "\"";
            if(this->artworkPath!=""){ mp3_option << " --ti " <<     "\""<<  this->artworkPath<<  "\""; }
            mp3_option << " \"" <<  inputPath <<      "\" ";
            mp3_option << " -o " <<  "\""<< outputFile <<  "\"";


            QString mp3_execute = lamePath + mp3_option.join("");
            EncodeProcess(mp3_execute, title, num_encoding_music);
            this->ui->logWidget->insertPlainText(tr("start mp3 encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));
        }
        // ======== WAV ========
        if(this->ui->outputWav->isChecked())
        {
            QString outputFile = GetOutputPath(wavOutputPath, title, ".wav", i);
            QFile::copy(inputPath, outputFile);
        }
    }
}

void MainWindow::MacEncodeProcess()
{
    const bool isExistsqaac = QFile::exists(qaacPath);
    const bool isExistsLame = QFile::exists(lamePath);
    const QString outputFolder = this->ui->outputFolderPath->text();
    const int size = this->ui->tableWidget->rowCount();
    const int num_encoding_music = [&](){
        if(isExistsLame && isExistsqaac){ return size*2; }
        else if(isExistsLame == false && isExistsqaac == false){ return 0; }
        return size;
    }();

    for(int i=0; i<size; ++i)
    {
        QString path  = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::UserRole).toString();
        QString title = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::DisplayRole).toString();
        QString track_no    = this->ui->tableWidget->item(i, TableColumn::TrackNo)->data(Qt::DisplayRole).toString();
        QString artist      = this->ui->tableWidget->item(i, TableColumn::Artist)->data(Qt::DisplayRole).toString();
        QString album_title = this->ui->tableWidget->item(i, TableColumn::AlbumTitle)->data(Qt::DisplayRole).toString();
        QString genre       = this->ui->tableWidget->item(i, TableColumn::Genre)->data(Qt::DisplayRole).toString();
        QString album_artist= this->ui->tableWidget->item(i, TableColumn::AlbumArtist)->data(Qt::DisplayRole).toString();
        QString group       = this->ui->tableWidget->item(i, TableColumn::Group)->data(Qt::DisplayRole).toString();
        QString composer    = this->ui->tableWidget->item(i, TableColumn::Composer)->data(Qt::DisplayRole).toString();
        QString year        = this->ui->tableWidget->item(i, TableColumn::Year)->data(Qt::DisplayRole).toString();
        QStringList meta = QStringList() << "-metadata title=\""  + title + "\"" <<
                                            "-metadata track=\""  + track_no + "/" + QString("%1").arg(size) + "\"" <<
                                            "-metadata author=\"" + artist + "\"" <<
                                            "-metadata artist=\"" + artist + "\"" <<
                                            "-metadata album=\""  + album_title + "\"" <<
                                            "-metadata genre=\""  + genre + "\"" <<
                                            "-metadata album_artist=\""  + album_artist + "\"" <<
                                            "-metadata composer=\""  + composer + "\"" <<
                                            "-metadata publisher=\""  + group + "\"" <<
                                            "-metadata date=\""   + year  + "\"";

        //トラック番号をファイル名に含めるか
        if(this->settings->IsAddTrackNoForTitle()){
            title = QString("%1%2%3").arg(i+1).arg(this->settings->DelimiterForTrackNo()).arg(title);
        }

        //AAC
        QString aac_args = this->settings->GetAACEncodeSetting().arg(path).arg(meta.join(' ')).arg(outputFolder+"/"+m4aOutputPath+"/"+title+".m4a");
        QString aac_execute = this->settings->GetFFmpegPath() + " " + aac_args;
        EncodeProcess(aac_execute, title, num_encoding_music);
        this->ui->logWidget->insertPlainText(tr("start aac encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));

        QString mp3_args = this->settings->GetMP3EncodeSetting().arg(path).arg(meta.join(' ')).arg(outputFolder+"/"+mp3OutputPath+"/"+title+".mp3");
        QString mp3_execute = this->settings->GetFFmpegPath() + " " + mp3_args;
        EncodeProcess(mp3_execute, title, num_encoding_music);
        this->ui->logWidget->insertPlainText(tr("start mp3 encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));

        QFile::copy(path, outputFolder+"/"+wavOutputPath+"/"+title+".wav");
    }
}

