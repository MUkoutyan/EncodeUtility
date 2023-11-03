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
#include "ProjectDefines.hpp"

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
#include <QPainter>
#include <QProgressDialog>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"

#include <QDebug>

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

bool downloadAndExtract(const QUrl &url, const QUrl &hashUrl, const QString &destinationDir)
{
    QNetworkAccessManager manager;

    // 進捗ダイアログの初期化
    QProgressDialog progressDialog;
    progressDialog.setLabelText("downloading ffmpeg...");
    progressDialog.setRange(0, 100);
    progressDialog.setModal(true);
    progressDialog.show();

    auto DownloadHash = [&](){
        // 1. SHA-256ハッシュ値のテキストファイルをダウンロード。
        QNetworkReply *hashReply = manager.get(QNetworkRequest(hashUrl));
        QEventLoop hashLoop;
        QObject::connect(hashReply, &QNetworkReply::finished, &hashLoop, &QEventLoop::quit);
        hashLoop.exec();
        QByteArray hashData = hashReply->readAll();
        return QString(hashData).trimmed(); // 改行やスペースを取り除く
    };

    const auto zipHash = DownloadHash();

    QString zipPath = QDir::tempPath() + "/encodeutility-ffmpeg.zip";
#ifdef QT_DEBUG
    if(QFile::exists(zipPath) == false)
#endif
    {
        QNetworkReply *reply = manager.get(QNetworkRequest(url));

        if (reply->error()) {
            qDebug() << "Download error:" << reply->errorString();
            return false;
        }

        QObject::connect(reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal) {
             int progress = static_cast<int>(100.0 * bytesReceived / bytesTotal);
             progressDialog.setValue(progress);

             QString labelText = QString("downloading ffmpeg...\nDownloading: %1/%2 bytes").arg(bytesReceived).arg(bytesTotal);
             progressDialog.setLabelText(labelText);
        });

        QEventLoop loop;  // ダウンロードが終わるまで待機するためのイベントループ
        QObject::connect(&progressDialog, &QProgressDialog::canceled, [&]() {
            reply->abort();
            loop.quit();
        });
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if(progressDialog.wasCanceled()){
            qDebug() << "Download Canceled";
            return false;
        }

        QByteArray zipData = reply->readAll();

        // 3. ダウンロードしたZIPファイルのSHA-256ハッシュ値を計算。
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(zipData);
        QString calculatedHash = hash.result().toHex();

        // 4. ダウンロードしたハッシュ値テキストと計算されたハッシュ値を比較。
        if (calculatedHash != zipHash) {
            qDebug() << "Hash mismatch!";
            return false;
        }

        QFile file(zipPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(zipData);
            file.close();
        }
    }
#ifdef QT_DEBUG
    else
    {
        QFile file(zipPath);
        if (file.open(QIODevice::ReadOnly)) {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(file.readAll());
            QString calculatedHash = hash.result().toHex();

            // 4. ダウンロードしたハッシュ値テキストと計算されたハッシュ値を比較。
            if (calculatedHash != zipHash) {
                qDebug() << "Hash mismatch!";
                return false;
            }
        }
        else{
            return true;
        }
    }
#endif

    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Error opening the zip file";
        progressDialog.setValue(100);
        progressDialog.hide();
        return false;
    }

    QString ffmpegExePath = "";
    {
        auto fileNameList = zip.getFileNameList();
        for(const auto& name : fileNameList){
            if(name.contains("ffmpeg.exe")){
                ffmpegExePath = name;
                break;
            }
        }
    }

    if(ffmpegExePath.isEmpty()){
        qDebug() << "Not found ffmpeg.exe in zip";
        progressDialog.setValue(100);
        progressDialog.hide();
        zip.close();
        return false;
    }

    progressDialog.setLabelText("Extracting...");
    progressDialog.setValue(0);

    QDir dir(destinationDir);
    QuaZipFile fileInsideZip(zipPath, ffmpegExePath);

    fileInsideZip.open(QIODevice::ReadOnly);
    QByteArray fileData = fileInsideZip.readAll();
    fileInsideZip.close();

    QFile outputFile(dir.absoluteFilePath("ffmpeg.exe"));
    if (outputFile.open(QIODevice::WriteOnly)) {
        outputFile.write(fileData);
        outputFile.close();
    }
    progressDialog.setValue(100);
    progressDialog.hide();

    return true;
}


bool MainWindow::CheckEncoder()
{
    auto CheckExistsRequiredFiles = [](){
        bool hit = true;
        QStringList requiredFiles = {
            "ffmpeg.exe"
        };
        for(auto& url : requiredFiles){ hit &= QFile::exists(qApp->applicationDirPath()+"/"+url); }
        return hit;
    };

    if(CheckExistsRequiredFiles()){
        return true;
    }

    QMessageBox msg(this);
    msg.setWindowTitle(tr("Not found ffmpeg"));
    msg.setTextFormat(Qt::RichText);
    msg.setText(tr("ffmpeg is required for encoding.\nDo you want to open a ffmpeg download link?\n(from: <a href=\"https://www.gyan.dev/ffmpeg/builds\">https://www.gyan.dev/ffmpeg/builds</a> -> ffmpeg-release-essentials.zip)"));
    msg.setIcon(QMessageBox::Icon::Question);
    msg.addButton(QMessageBox::Yes);
    msg.addButton(QMessageBox::No);
    msg.setDefaultButton(QMessageBox::Yes);
    //エンコードにはffmpegが必要です。 ダウンロードリンクを開きますか？
    if(msg.exec() == QMessageBox::Yes)
    {
        bool result = downloadAndExtract(QUrl("https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"),
                                         QUrl("https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip.sha256"),
                                         qApp->applicationDirPath());

        if(result == false)
        {
            QString url = "https://www.gyan.dev/ffmpeg/builds";
            if(QDesktopServices::openUrl(QUrl(url))){
                QDesktopServices::openUrl(QUrl(qApp->applicationDirPath()));
                QMessageBox msg2(this);
                msg2.setModal(true);
                msg2.setWindowTitle(tr("Can't download ffmpeg"));
                msg2.setIcon(QMessageBox::Warning);
                //ffmpegのダウンロードとzipの展開に失敗しました。
                //ffmpegのzipを手動でダウンロード・展開し、
                //ffmpeg.exeをEncodeUtility.exeと同じ場所に置いて"OK"を押してください。
                msg2.setText(tr("Download and extract of ffmpeg zip failed.\nDownload and extract the ffmpeg zip manually,\n") + tr("Place \"ffmpeg.exe\"in the same location as EncodeUtility.exe."));
                msg2.addButton(QMessageBox::Ok);
                msg2.exec();
            }
            else{
                QMessageBox::critical(this, tr("Can't open URL"), tr("Can't open URL. ") + url, QMessageBox::Ok);
                return false;
            }
        }

    }

    if(CheckExistsRequiredFiles() == false){
        //エンコードに必要なファイルが見つかりません。出力にチェックを入れたときに再度確認します。
        QMessageBox::critical(this, tr("Not found ffmpeg"), tr("Cannot find file needed for encoding. Check again when output is checked."), QMessageBox::Ok);
        return false;
    }
    return true;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , metadataTable(nullptr)
    , aboutLabel(new QLabel(tr("drag&drop .wav files \n or \n jacket(.png or .jpg) file here."), this))
    , wavOutputPath("wav")
    , imageOutputPath("")
    , processedCount(0)
    , numEncodingMusic(0)
    , numEncodingFile(0)
    , settings(new DialogAppSettings(this))
    , widgetListDisableDuringEncode({})
    , lastLoadProject("")
    , currentWorkDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0])
    , batchEntryWidget(new QWidget(this, Qt::Popup))
    , showAtFirst(true)
{
    ui->setupUi(this);

    encoderComponents = {
        EncoderComponents{std::make_shared<AACEncoder>(),  this->ui->outputM4a,  this->ui->baseFolderM4a,   this->ui->m4aOutputPath},
        EncoderComponents{std::make_shared<MP3Encoder>(),  this->ui->outputFlac, this->ui->baseFolderFlac,  this->ui->flacOutputPath},
        EncoderComponents{std::make_shared<FlacEncoder>(), this->ui->outputMp3,  this->ui->baseFolderMp3,   this->ui->mp3OutputPath},
    };

    QApplication::setStyle("fusion");

    auto toolVer = this->ui->menuAbout->addAction(ProjectDefines::applicationVersion);
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
        this->ui->outputMp3,    this->ui->baseFolderMp3,    this->ui->mp3OutputPath,
        this->ui->outputM4a,    this->ui->baseFolderM4a,    this->ui->m4aOutputPath,
        this->ui->outputFlac,   this->ui->baseFolderFlac,   this->ui->flacOutputPath,
        this->ui->outputWav,    this->ui->baseFolderWav,    this->ui->wavOutputPath,
        this->ui->includeImage, this->ui->baseFolderImage,  this->ui->imageOutputPath,
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

    //一括入力関係
    batchEntryWidget->hide();
    batchParameters.resize(ProjectDefines::headerItems.size());
    this->CreateBatchEntryWidgets();
    connect(this->ui->batchInputButton, &QToolButton::toggled, this, [this](bool checked)
    {
        if(checked){
            this->ui->gridLayout_3->addWidget(this->batchEntryWidget, 0, 2);
            this->batchEntryWidget->setFocus();
            this->batchEntryWidget->show();
        }
        else{
            this->batchEntryWidget->hide();
            this->ui->tableWidget->setFocus();
        }
    });

    //メニュー
    connect(this->ui->actionSave_file, &QAction::triggered, this, [this]()
    {
        QString savefilePath = this->lastLoadProject;
        if(savefilePath.isEmpty()){
           savefilePath = QFileDialog::getSaveFileName(this, tr("Save Project File"),currentWorkDirectory,
                                                             tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        }
        this->SaveProjectFile(savefilePath);
    });
    connect(this->ui->actionSave_as, &QAction::triggered, this, [this]()
    {
        auto savefilePath = QFileDialog::getSaveFileName(this, tr("Save Project File"),currentWorkDirectory,
                                                               tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        this->SaveProjectFile(savefilePath);
    });


    connect(this->ui->actionLoad_file, &QAction::triggered, this, [this]()
    {
        auto location = this->lastLoadProject;
        if(location.isEmpty()){
            location = currentWorkDirectory;
        }
        auto openfilePath = QFileDialog::getOpenFileName(this, tr("Open Project File"), location,
                                                               tr("project file(*%1)").arg(ProjectDefines::projectExtention));
        settings->SetLastOpenedDirectory(openfilePath);
        this->openFiles({openfilePath});
    });

    //出力先の取得と表示
    connect(this->ui->outputFolderPath, &QLineEdit::textChanged, this, [this](QString path){
        this->ui->baseFolderMp3->setText(path+"/");
        this->ui->baseFolderM4a->setText(path+"/");
        this->ui->baseFolderFlac->setText(path+"/");
        this->ui->baseFolderWav->setText(path+"/");
        this->ui->baseFolderImage->setText(path+"/");
    });
    this->ui->outputFolderPath->setText(currentWorkDirectory+"/EncodeUtilityFolder");

    //メタデータが編集された
    connect(this->ui->tableWidget, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item)
    {
        //選択されている箇所を全て変更する
        auto items = this->ui->tableWidget->selectedItems();
        for(QTableWidgetItem* selectItem : items){
            selectItem->setData(Qt::DisplayRole, item->data(Qt::DisplayRole));
        }
    });

    connect(this->ui->encodeButton, &QPushButton::clicked, this, &MainWindow::Encode);

    connect(this->ui->expandOption, &QPushButton::clicked, this, [this]()
    {
        auto ChangeVisible = [](QWidget* w){ w->setVisible(!w->isVisible()); };
        ChangeVisible(this->ui->label_4);
        ChangeVisible(this->ui->filenameDelimiter);

        ChangeVisible(this->ui->outputMp3);
        ChangeVisible(this->ui->baseFolderMp3);
        ChangeVisible(this->ui->mp3OutputPath);

        ChangeVisible(this->ui->outputM4a);
        ChangeVisible(this->ui->baseFolderM4a);
        ChangeVisible(this->ui->m4aOutputPath);

        ChangeVisible(this->ui->outputFlac);
        ChangeVisible(this->ui->baseFolderFlac);
        ChangeVisible(this->ui->flacOutputPath);

        ChangeVisible(this->ui->outputWav);
        ChangeVisible(this->ui->baseFolderWav);
        ChangeVisible(this->ui->wavOutputPath);

        ChangeVisible(this->ui->includeImage);
        ChangeVisible(this->ui->baseFolderImage);
        ChangeVisible(this->ui->imageOutputPath);

        ChangeVisible(this->ui->option_addTrackNo);
    });


    connect(this->ui->wavOutputPath, &QLineEdit::textEdited, this, [this](QString text){
        this->wavOutputPath = std::move(text);
    });
    connect(this->ui->imageOutputPath, &QLineEdit::textEdited, this, [this](QString text){
        this->imageOutputPath = std::move(text);
    });

    this->ui->wavOutputPath->setText(this->wavOutputPath);
    this->ui->wavOutputPath->setEnabled(this->ui->outputWav->isChecked());

    this->ui->imageOutputPath->setText(this->imageOutputPath);
    this->ui->imageOutputPath->setEnabled(this->ui->includeImage->isChecked());

    for(auto& component : encoderComponents)
    {
        connect(component.outputPath, &QLineEdit::textEdited, this, [&component](QString text){
            component.encoder->SetCodecFolderName(std::move(text));
        });

        component.outputPath->setText(component.encoder->GetCodecFolderName());
        component.outputPath->setEnabled(component.enableCheck->isChecked());
        component.baseFolder->setEnabled(component.enableCheck->isChecked());

        connect(component.enableCheck, &QCheckBox::clicked, this, [&](bool checked){
            if(this->CheckEncoder() == false){
                component.enableCheck->setChecked(false);
                return;
            }
            component.encoder->SetIsEnableEncoder(checked);
            this->CheckEnableEncodeButton();
        });
        connect(component.enableCheck, &QCheckBox::toggled, component.baseFolder, &QLineEdit::setEnabled);
        connect(component.enableCheck, &QCheckBox::toggled, component.outputPath, &QLineEdit::setEnabled);
    }

    connect(this->ui->outputWav,    &QCheckBox::clicked, this, [this](bool checked){
        this->CheckEnableEncodeButton();
    });
    connect(this->ui->outputWav,&QCheckBox::toggled, this->ui->baseFolderWav, &QLineEdit::setEnabled);
    connect(this->ui->outputWav,&QCheckBox::toggled, this->ui->wavOutputPath, &QLineEdit::setEnabled);

    connect(this->ui->includeImage, &QCheckBox::clicked, this, [this](bool checked){
        this->CheckEnableEncodeButton();
    });
    connect(this->ui->includeImage,&QCheckBox::toggled, this->ui->baseFolderImage, &QLineEdit::setEnabled);
    connect(this->ui->includeImage,&QCheckBox::toggled, this->ui->imageOutputPath, &QLineEdit::setEnabled);

    connect(this->ui->selectFolder, &QToolButton::clicked, this, [this]()
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                     this->ui->outputFolderPath->text(),
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        this->ui->outputFolderPath->setText(dir);
    });
    connect(this->ui->openFolder, &QToolButton::clicked, this, [this](){
        QDesktopServices::openUrl(QUrl("file:///"+this->ui->outputFolderPath->text()));
    });

    QAction* delete_artwork_action = this->artworkMenu->addAction(tr("Delete Artwork"));
    this->ui->artwork->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->ui->artwork, &QLabel::customContextMenuRequested, this, [this, delete_artwork_action](const QPoint&)
    {
        //選択しているアイテムが無ければメニューを無効
        delete_artwork_action->setEnabled(this->artworkPath != "");
        this->artworkMenu->exec(QCursor::pos());
    });
    connect(delete_artwork_action, &QAction::triggered, this, [this]()
    {
        this->artworkPath = "";
        this->ui->artwork->setPixmap(QPixmap());
        this->ui->artwork->setVisible(false);
    });

    //セルクリック時の右クリックメニュー
    QAction* delete_row_action = this->tableMenu->addAction(tr("Delete Row"));
    this->ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->ui->tableWidget, &QTableWidget::customContextMenuRequested, this, [this, delete_row_action](const QPoint&)
    {
        //選択しているアイテムが無ければメニューを無効
        delete_row_action->setEnabled(this->ui->tableWidget->selectedItems().size() > 0);
        this->tableMenu->exec(QCursor::pos());
    });
    connect(delete_row_action, &QAction::triggered, this, [this]()
    {
        auto items = this->ui->tableWidget->selectedItems();
        for(QTableWidgetItem* selectItem : items){
            this->ui->tableWidget->removeRow(selectItem->row());
        }
    });

    connect(this->ui->actionClear_All_Items, &QAction::triggered, this, [this]()
    {
        int size = this->ui->tableWidget->rowCount();
        for(int row=0; row<size; ++row){
            this->ui->tableWidget->removeRow(0);
        }
    });

    connect(this->ui->actionSettings, &QAction::triggered, this, [this](){
        this->settings->show();
        this->settings->raise();
    });

    connect(this->ui->actionCheck_Encoder, &QAction::triggered, this, [this](){
        if(this->CheckEncoder()){
            //エンコーダーを正しく認識しています。
            QMessageBox::information(this, tr("Check Encoder"), tr("The encoder is correctly identified."));
        }
    });

    const auto InitEncodeProcess = [this](const auto& process)
    {
        connect(process.get(), &EncoderInterface::readStdOut, this, [this](QString arr){
            this->ui->logWidget->insertPlainText("\n"+arr);
        });
        connect(process.get(), &EncoderInterface::readStdError, this, [this](QString arr){
            this->ui->logWidget->insertPlainText("\n"+arr);
            this->ui->logWidget->verticalScrollBar()->setValue(this->ui->logWidget->verticalScrollBar()->maximum());
        });

        connect(process.get(), &EncoderInterface::encodeFinish, this, [this](const QString inputPath, const AudioMetaData&){
            this->processedCount++;

            this->ui->statusBar->showMessage(tr("Finish Encoding. %1/%2").arg(this->processedCount+1).arg(this->numEncodingMusic));
            this->ui->logWidget->insertPlainText("\nfinish : "+inputPath+"\n");

            if(this->processedCount >= this->numEncodingMusic)
            {
                //全部エンコードしたらエンコードボタンを有効にしてメタテーブルに表示を戻す
                this->ui->statusBar->showMessage(tr("Complete."));
                this->ui->logWidget->insertPlainText(tr("Complete."));
                for(auto widget : widgetListDisableDuringEncode){ widget->setEnabled(true); }
                this->ui->tabWidget->setCurrentIndex(0);
            }
        });
    };
    for(auto& component : encoderComponents){
        InitEncodeProcess(component.encoder);
    }

    //設定ファイルの読み込み
    LoadSettingFile();
}

void MainWindow::CreateBatchEntryWidgets()
{
    auto* vLayout = new QVBoxLayout();

    QLineEdit* firstLineEdit = nullptr;
    QLineEdit* beforeLineEdit = nullptr;
    for(int i=1; i<ProjectDefines::headerItems.size(); ++i)
    {
        const auto& h = ProjectDefines::headerItems[i];
        auto* hLayout = new QHBoxLayout();
        auto* label = new QLabel(h);
        label->setMinimumWidth(120);

        auto* text = new QLineEdit(batchParameters[i-1]);
        connect(text, &QLineEdit::textEdited, this, [this, i](const QString& t){
            this->batchParameters[i-1] = t;
            this->batchEntryWidget->nextInFocusChain();
        });

        if(beforeLineEdit){
            this->batchEntryWidget->setTabOrder(beforeLineEdit, text);
        }
        else{
            firstLineEdit = text;
        }
        beforeLineEdit = text;

        text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        hLayout->addWidget(label);
        hLayout->addWidget(text);
        vLayout->addLayout(hLayout);
    }
    this->batchEntryWidget->setTabOrder(beforeLineEdit, firstLineEdit);

    auto buttonLayout = new QHBoxLayout();
    auto spacer = new QSpacerItem(1,1,QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto applyButton = new QPushButton(tr("Apply"));
    buttonLayout->addItem(spacer);
    buttonLayout->addWidget(applyButton);

    connect(applyButton, &QPushButton::clicked, this, [this]()
    {
        int row = this->ui->tableWidget->rowCount();
        for(int i=1; i<ProjectDefines::headerItems.size(); ++i)
        {
            const auto& text = this->batchParameters[i-1];
            if(text.isEmpty()){ continue; }
            for(int r=0; r<row; ++r)
            {
                auto* item = new QTableWidgetItem(text);
                this->ui->tableWidget->setItem(r, i, item);
            }
        }
    });

    vLayout->addLayout(buttonLayout);

    batchEntryWidget->setLayout(vLayout);
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

        for (const QUrl& url : urlList){
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
            break;  //プロジェクトファイル読み込みは別関数に任せるのでbreak
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

        QString track_no = QString("%1").arg(row+1);    //iTunesが1開始なので準拠させる
        QString title = path.mid(path.lastIndexOf("/")+1).section(".", 0, 0);
        QString albumTitle = "";
        QString artist = "";
        QString genre = "";

        //区切り文字で分けられればメタデータをファイル名から取得
        QStringList metaDatas = title.split(this->ui->filenameDelimiter->text());
        const int size = metaDatas.size();
        if(size > 0)
        {
            //意図的なフォールスルー
            switch(size-1)
            {
            case TableColumn::Genre:
                genre = metaDatas[4];
                [[fallthrough]];
            case TableColumn::AlbumTitle:
                albumTitle = metaDatas[3];
                [[fallthrough]];
            case TableColumn::Artist:
                artist = metaDatas[2];
                [[fallthrough]];
            case TableColumn::Title:
                title = metaDatas[1];
                [[fallthrough]];
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
        this->CheckEnableEncodeButton();
        this->ui->actionSave_as->setEnabled(true);
        this->ui->actionSave_file->setEnabled(true);
        this->ui->batchInputButton->setEnabled(true);
    }
}

void MainWindow::CheckEnableEncodeButton()
{
    if(this->ui->tableWidget->rowCount() == 0){
        this->ui->encodeButton->setEnabled(false);
        return;
    }
    bool isEnable = false;
    for(const auto& component : encoderComponents){
        isEnable |= component.encoder->GetIsEnableEncoder();
    }
    isEnable |= this->ui->outputWav->isChecked();
    isEnable |= this->ui->includeImage->isChecked();
    this->ui->encodeButton->setEnabled(isEnable);
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);

    //エンコーダーの存在チェック
    if(showAtFirst){
        auto enable = CheckEncoder();
        this->ui->outputMp3->setChecked(enable);
        this->ui->outputM4a->setChecked(enable);
        this->ui->outputFlac->setChecked(enable);

        showAtFirst = false;
    }
}

void MainWindow::SaveProjectFile(QString saveFilePath)
{
    // ### Ver.1.0.0
    // project version
    // output path
    // image path

    // ### Ver.1.0.1
    // addTrackNo Flag
    // delimiter
    // fill digit

    // ### Ver.1.0.2
    // filenameDelimiter

    // ### Common
    // no., title, artist, albumtitle...
    // no., title, artist, albumtitle...
    if(saveFilePath.isEmpty()){ return; }

    QStringList tableList;
    tableList.append(QString::number(ProjectDefines::projectVersionNum));
    tableList.append(this->ui->outputFolderPath->text());
    tableList.append(this->artworkPath);

    // ### Ver.1.0.1
    tableList.append(QVariant(this->ui->check_addTrackNo->isChecked()).toString());
    tableList.append(this->ui->track_no_delimiter->text());
    tableList.append(QString::number(this->ui->num_of_digit->value()));

    // ### Ver.1.0.2
    tableList.append(this->ui->filenameDelimiter->text());

    // ### Common
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

void MainWindow::SaveSettingFile(QString key, QVariant value)
{
    QSettings settingfile(ProjectDefines::settingFilePath, QSettings::IniFormat);
    settingfile.setValue(ProjectDefines::settingOutputFolder, this->ui->outputFolderPath->text());
}


void MainWindow::LoadSettingFile()
{
    QSettings settingfile(ProjectDefines::settingFilePath, QSettings::IniFormat);
    this->ui->outputFolderPath->setText(settingfile.value(ProjectDefines::settingOutputFolder, this->ui->outputFolderPath->text()).toString());
}

void MainWindow::loadProjectFile(QString projFilePath)
{
    if(projFilePath.isEmpty()){ return; }

    QFile file(projFilePath);
    if(file.open(QFile::ReadOnly) == false){ return; }

    currentWorkDirectory = QFileInfo(projFilePath).absoluteDir().absolutePath();

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
    QPixmap artwork = QPixmap(this->artworkPath);
    this->ui->artwork->setVisible(true);
    this->ui->artwork->setPixmap(artwork.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if(0x010001 <= projVersion){
        this->ui->check_addTrackNo->setChecked(QVariant(strList[index++]).toBool());
        this->ui->track_no_delimiter->setText(strList[index++]);
        this->ui->num_of_digit->setValue(strList[index++].toInt());
    }
    if(0x010002 <= projVersion){
        this->ui->filenameDelimiter->setText(strList[index++]);
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

    this->ui->batchInputButton->setEnabled(true);

    this->lastLoadProject = projFilePath;
}

void MainWindow::Encode()
{
    this->ui->statusBar->showMessage(tr("Start Encoding."));

    this->processedCount = 0;
    this->numEncodingMusic = this->ui->tableWidget->rowCount();
    this->numEncodingFile = [&]()
    {
        int result = 0;
        if(this->ui->outputMp3->isChecked()){
            result += this->numEncodingMusic;
        }
        if(this->ui->outputM4a->isChecked()){
            result += this->numEncodingMusic;
        }
        if(this->ui->outputFlac->isChecked()){
            result += this->numEncodingMusic;
        }
        return result;
    }();

    //エンコード中にエンコードさせないようにするためボタンを無効
    for(auto widget : widgetListDisableDuringEncode){ widget->setEnabled(false); }
    this->ui->tabWidget->setCurrentIndex(1);    //ログウィジェットを表示

    const QString outputFolder = this->ui->outputFolderPath->text();
    //出力先フォルダを作成 エンコーダーの有無・出力チェックの状態に応じてフォルダを作成
    QDir().mkdir(outputFolder);
    //他コーデックはコーデッククラス側で行っているため、wavのみ確認する。
    if(this->ui->outputWav->isChecked()){ QDir().mkdir(outputFolder+"/"+wavOutputPath); }
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

void MainWindow::WindowsEncodeProcess()
{
    const QString outputFolder = this->ui->outputFolderPath->text();
    const int size = this->ui->tableWidget->rowCount();


    for(const auto& component : encoderComponents){
        component.encoder->SetOutputFolderPath(outputFolder);
        component.encoder->SetIsAddTrackNo(this->ui->check_addTrackNo->isChecked());
        component.encoder->SetNumOfDigit(this->ui->num_of_digit->value());
        component.encoder->SetTrackNumberDelimiter(this->ui->track_no_delimiter->text());
        component.encoder->SetNumEncodingMusic(this->numEncodingMusic);
    }

    const auto wavOutputFullPath = outputFolder + "/" + this->wavOutputPath;

    for(int i=0; i<size; ++i)
    {
        AudioMetaData metaData;
        auto inputPath       = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::UserRole).toString();
        metaData.title       = this->ui->tableWidget->item(i, TableColumn::Title)->data(Qt::DisplayRole).toString();
        metaData.track_no    = this->ui->tableWidget->item(i, TableColumn::TrackNo)->data(Qt::DisplayRole).toString();
        metaData.artist      = this->ui->tableWidget->item(i, TableColumn::Artist)->data(Qt::DisplayRole).toString();
        metaData.albumTitle  = this->ui->tableWidget->item(i, TableColumn::AlbumTitle)->data(Qt::DisplayRole).toString();
        metaData.genre       = this->ui->tableWidget->item(i, TableColumn::Genre)->data(Qt::DisplayRole).toString();
        metaData.albumArtist = this->ui->tableWidget->item(i, TableColumn::AlbumArtist)->data(Qt::DisplayRole).toString();
        metaData.group       = this->ui->tableWidget->item(i, TableColumn::Group)->data(Qt::DisplayRole).toString();
        metaData.composer    = this->ui->tableWidget->item(i, TableColumn::Composer)->data(Qt::DisplayRole).toString();
        metaData.year        = this->ui->tableWidget->item(i, TableColumn::Year)->data(Qt::DisplayRole).toString();
        metaData.artworkPath = this->artworkPath;


        for(const auto& component : encoderComponents){
            if(component.enableCheck->isChecked())
            {
                if(component.encoder->Encode(inputPath, metaData, i)){
                    this->ui->logWidget->insertPlainText(tr("start %1 encoding : %2(%3/%4)\n").arg(component.encoder->GetCodecExtention()).arg(metaData.title).arg(i+1).arg(this->numEncodingMusic));
                }
                else{
                    this->processedCount++;
                }
            }
        }

        // ======== WAV ========
        if(this->ui->outputWav->isChecked())
        {
            QString outputFile = wavOutputFullPath+"/"+metaData.title+".wav";
            if(this->ui->check_addTrackNo->isChecked()){
                outputFile = wavOutputFullPath+"/"+QString("%1%2%3").arg(i+1, this->ui->num_of_digit->value()).arg(this->ui->track_no_delimiter->text()).arg(metaData.title)+".wav";
            }
            QFile::copy(inputPath, outputFile);
            this->ui->logWidget->insertPlainText(tr("copy wave file : %1(%2/%3)\n").arg(metaData.title).arg(i+1).arg(this->numEncodingMusic));
        }
    }
}
