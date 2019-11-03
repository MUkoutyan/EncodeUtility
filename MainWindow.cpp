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

#include <QDebug>

struct StrDefines
{
    static constexpr char projectExtention[] = ".encproj";
    static constexpr char projectVersion[] = "1.0.0";
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
const QStringList StrDefines::headerItems = {"No.", "Title", "Artist", "AlbumTitle", "AlbumArtist", "Composer", "Group", "Genre", "Year"};

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
    , qaacPath(qApp->applicationDirPath()+"/"+StrDefines::qaacEXEFileName)
    , lamePath(qApp->applicationDirPath()+"/"+StrDefines::lameEXEFileName)
    , processed_count(0)
    , checkQaacFile(true)
    , checkLameFile(true)
    , settings(new DialogAppSettings(this))
    , widgetListDisableDuringEncode()
{
    ui->setupUi(this);

    widgetListDisableDuringEncode = {
        this->ui->encodeButton, this->ui->tableWidget,
        this->ui->outputM4a,    this->ui->outputMp3,
        this->ui->outputWav,    this->ui->includeImage,
        this->ui->outputFolderPath
    };

    this->tableMenu = new QMenu(this->ui->tableWidget);
    this->artworkMenu = new QMenu(this->ui->artwork);

    this->aboutLabel->setAlignment(Qt::AlignCenter);

    this->setAcceptDrops(true); //D&D許可
    this->ui->verticalLayout->addWidget(this->aboutLabel, 6, Qt::AlignHCenter); //説明文の表示
    this->ui->tabWidget->setHidden(true);   //編集Widgetはこの時点で表示しない
    this->ui->artwork->setVisible(false);

    //メニュー
    connect(this->ui->actionSave_file, &QAction::triggered, this, &MainWindow::SaveProjectFile);

    connect(this->ui->actionLoad_file, &QAction::trigger, [this]()
    {
        auto openfilePath = QFileDialog::getOpenFileName(this, tr("Open Project File"),QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0],
                                                         tr("project file(*%1)").arg(StrDefines::projectExtention));
        this->loadProjectFile(openfilePath);
    });

    //出力先の取得と表示
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

    connect(this->ui->expandOption, &QPushButton::clicked, [this](){
        this->ui->outputM4a->setVisible(!this->ui->outputM4a->isVisible());
        this->ui->outputMp3->setVisible(!this->ui->outputMp3->isVisible());
        this->ui->outputWav->setVisible(!this->ui->outputWav->isVisible());
        this->ui->includeImage->setVisible(!this->ui->includeImage->isVisible());
    });

    auto CheckEnableEncodeButton = [this]()
    {
        if(this->ui->tableWidget->rowCount() == 0){ return; }
        bool isEnable = false;
        isEnable |= this->ui->outputM4a->isChecked();
        isEnable |= this->ui->outputMp3->isChecked();
        isEnable |= this->ui->outputWav->isChecked();
        isEnable |= this->ui->includeImage->isChecked();
        this->ui->encodeButton->setEnabled(isEnable);
    };
    connect(this->ui->outputM4a, &QCheckBox::clicked, CheckEnableEncodeButton);

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

    //設定ファイルの読み込み
    LoadSettingFile();

    //エンコーダーの存在チェック
    CheckEncoder();
}

void MainWindow::CheckEncoder()
{
    auto CheckExistsFile = [](QStringList list){
        bool hit = true;
        for(auto& url : list){ hit &= QFile::exists(qApp->applicationDirPath()+"/"+url); }
        return hit;
    };

    const QStringList lameFiles = {StrDefines::lameEXEFileName, "lame_enc.dll"};
    if(this->checkLameFile)
    {
        bool isExistsLame = CheckExistsFile(lameFiles);
        if(isExistsLame == false)
        {
            //mp3エンコードにはLameが必要です。 ダウンロードリンクを開きますか？
            QCheckBox* checkBox = new QCheckBox(tr("Don't show again."));
            connect(checkBox, &QCheckBox::stateChanged, this, [this](int state){
                this->checkLameFile = static_cast<Qt::CheckState>(state) != Qt::CheckState::Checked;
                this->SaveSettingFile(StrDefines::settingCheckLameFile, this->checkLameFile);
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
    this->ui->outputMp3->setEnabled(CheckExistsFile(lameFiles));

    const QStringList qaacList = {"libsoxconvolver64.dll", "libsoxr64.dll", StrDefines::qaacEXEFileName};
    if(this->checkQaacFile)
    {
        bool isExistsqaac = CheckExistsFile(qaacList);
        if(isExistsqaac == false)
        {
            QCheckBox* checkBox = new QCheckBox(tr("Don't show again."));
            connect(checkBox, &QCheckBox::stateChanged, this, [this](int state){
                this->checkQaacFile = static_cast<Qt::CheckState>(state) != Qt::CheckState::Checked;
                this->SaveSettingFile(StrDefines::settingCheckQaacFile, this->checkQaacFile);
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
                    QMessageBox::information(this, "", tr("Place \"libsoxconvolver64.dll\", \"libsoxr64.dll\" and \"%1\"in the same location as EncodeUtility.exe.").arg(StrDefines::qaacEXEFileName), QMessageBox::Ok);
                }
                else{
                    QMessageBox::critical(this, tr("Can't open URL"), tr("Can't open URL. ") + url, QMessageBox::Ok);
                }
            }
        }
    }
    this->ui->outputM4a->setEnabled(CheckExistsFile(qaacList));
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
        QString extension = QFileInfo(path).suffix().toUpper();

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

        if(extension != "WAV"){
            continue;
        }
        QString track_no = QString("%1").arg(row);
        QString title = path.mid(path.lastIndexOf("/")+1).section(".", 0, 0);
        QString albumTitle = "";
        QString artist = "";
        QString genre = "";

        //区切り文字で分けられればメタデータをファイル名から取得
        QStringList metaDatas = title.split('_');
        int size = metaDatas.size();
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
        this->ui->actionSave_file->setEnabled(true);
    }
}

void MainWindow::SaveProjectFile()
{
    // project version
    // output path
    // image path
    // no., title, artist, albumtitle...
    // no., title, artist, albumtitle...
    QStringList tableList;
    tableList.append(StrDefines::projectVersion);
    tableList.append(this->ui->outputFolderPath->text());
    tableList.append(this->artworkPath);

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

    auto savefilePath = QFileDialog::getSaveFileName(this, tr("Save Project File"),QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0],
                                                     tr("project file(*%1)").arg(StrDefines::projectExtention));
    if(savefilePath.isEmpty()){ return; }
    QFile file(savefilePath);
    if(file.open(QFile::WriteOnly)){
        file.write(tableList.join(',').toLocal8Bit());
    }
}

void MainWindow::SaveSettingFile(QString key, QVariant value)
{
    QSettings settingfile(settingFilePath, QSettings::IniFormat);
    settingfile.setValue(key, value);
}

void MainWindow::LoadSettingFile()
{
    QSettings settingfile(settingFilePath, QSettings::IniFormat);
    this->ui->outputFolderPath->setText(settingfile.value(StrDefines::settingOutputFolder, this->ui->outputFolderPath->text()).toString());
    this->checkQaacFile = settingfile.value(StrDefines::settingCheckQaacFile, true).toBool();
    this->checkLameFile = settingfile.value(StrDefines::settingCheckLameFile, true).toBool();
}

void MainWindow::loadProjectFile(QString projFilePath)
{
    if(projFilePath.isEmpty()){ return; }

    QFile file(projFilePath);
    if(file.open(QFile::ReadOnly) == false){ return; }

    this->ui->tableWidget->clear();
    this->ui->outputFolderPath->setText("");
    this->artworkPath = "";

    this->ui->tableWidget->setColumnCount(ALL);
    this->ui->tableWidget->setHorizontalHeaderLabels(StrDefines::headerItems);

    auto data = file.readAll();
    auto strList = QString::fromLocal8Bit(data).split(",");
    int index = 1;
    this->ui->outputFolderPath->setText(strList[index++]);
    this->artworkPath = strList[index++];

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
                if(metaDatas.size() > 0){
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
}

void MainWindow::Encode()
{
    this->ui->statusBar->showMessage(tr("Start Encoding."));

    this->processed_count = 0;
    //エンコード中にエンコードさせないようにするためボタンを無効
    for(auto widget : widgetListDisableDuringEncode){ widget->setEnabled(false); }
    this->ui->tabWidget->setCurrentIndex(1);    //ログウィジェットを表示

    const QString outputFolder = this->ui->outputFolderPath->text();
    const bool isExistsqaac = QFile::exists(qaacPath);
    const bool isExistsLame = QFile::exists(lamePath);
    //出力先フォルダを作成 エンコーダーの有無・出力チェックの状態に応じてフォルダを作成
    QDir().mkdir(outputFolder);
    if(isExistsqaac || this->ui->outputM4a->isChecked()){ QDir().mkdir(outputFolder+"/m4a"); }
    if(this->ui->outputWav->isChecked()){ QDir().mkdir(outputFolder+"/wav"); }
    if(isExistsLame || this->ui->outputMp3->isChecked()){ QDir().mkdir(outputFolder+"/mp3"); }
    if(this->ui->includeImage->isChecked())
    {
        QDir().mkdir(outputFolder+"/image");
        //jacketのコピー
        QFile::copy(this->artworkPath, outputFolder+"/image"+this->artworkPath.mid(this->artworkPath.lastIndexOf("/")));
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
    const int num_encoding_music = [&](){
        if(isExistsLame && isExistsqaac){ return size*2; }
        else if(isExistsLame == false && isExistsqaac == false){ return 0; }
        return size;
    }();

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
           this->ui->statusBar->showMessage(tr("Not found %1").arg(StrDefines::qaacEXEFileName), 3000);
        }
        else if(this->ui->outputM4a->isChecked())
        {
            QString outputFile = outputFolder+"/m4a/"+title+".m4a";
            if(this->settings->IsAddTrackNoForTitle()){
                outputFile = outputFolder+"/m4a/"+QString("%1%2%3").arg(i+1, this->settings->GetNumOfDigit()).arg(this->settings->DelimiterForTrackNo()).arg(title)+".m4a";
            }

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
        }
        else if(this->ui->outputMp3->isChecked())
        {
            QString outputFile = outputFolder+"/mp3/"+title+".mp3";
            if(this->settings->IsAddTrackNoForTitle()){
                outputFile = outputFolder+"/mp3/"+QString("%1%2%3").arg(i+1).arg(this->settings->DelimiterForTrackNo()).arg(title)+".mp3";
            }
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
            QString outputFile = outputFolder+"/wav/"+title+".wav";
            if(this->settings->IsAddTrackNoForTitle()){
                outputFile = outputFolder+"/wav/"+QString("%1%2%3").arg(i+1).arg(this->settings->DelimiterForTrackNo()).arg(title)+".wav";
            }
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
        QString aac_args = this->settings->GetAACEncodeSetting().arg(path).arg(meta.join(' ')).arg(outputFolder+"/m4a/"+title+".m4a");
        QString aac_execute = this->settings->GetFFmpegPath() + " " + aac_args;
        EncodeProcess(aac_execute, title, num_encoding_music);
        this->ui->logWidget->insertPlainText(tr("start aac encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));

        QString mp3_args = this->settings->GetMP3EncodeSetting().arg(path).arg(meta.join(' ')).arg(outputFolder+"/mp3/"+title+".mp3");
        QString mp3_execute = this->settings->GetFFmpegPath() + " " + mp3_args;
        EncodeProcess(mp3_execute, title, num_encoding_music);
        this->ui->logWidget->insertPlainText(tr("start mp3 encoding : %1(%2/%3)\n").arg(title).arg(i+1).arg(num_encoding_music));

        QFile::copy(path, outputFolder+"/wav/"+title+".wav");
    }
}

