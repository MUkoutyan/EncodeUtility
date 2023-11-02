#include "FlacEncoder.h"

#include <QCheckBox>
#include <QUrl>
#include <QMessageBox>
#include <QDesktopServices>

FlacEncoder::FlacEncoder()
    : EncoderInterface()
{
    this->SetCodecFolderName("flac");
}

FlacEncoder::~FlacEncoder(){
}

bool FlacEncoder::Encode(QString inputPath, AudioMetaData metaData, int processNumber)
{
    QProcess* process = new QProcess(this);
    process->setProgram(qApp->applicationDirPath()+"/ffmpeg.exe");
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process](){
        QByteArray arr = process->readAllStandardOutput();
        emit this->readStdOut("[mp3] " + QString(arr));
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process](){
        QByteArray arr = process->readAllStandardError();
        emit this->readStdOut("[mp3] " + QString(arr));
    });
    connect(process, &QProcess::readChannelFinished, this, [this, inputPath, metaData](){
        emit this->encodeFinish(inputPath, metaData);
    });

    QString outputFile = GetOutputPath(metaData.title, ".flac", processNumber);

    // エンコードオプション
    QStringList ffmpeg_option;
    ffmpeg_option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        ffmpeg_option << "-i" << artworkPath
                      << "-map" << "0:0" << "-map" << "1:0"
                      << "-c:v" << "mjpeg" << "-disposition:v:0" << "attached_pic";
    }
    ffmpeg_option << "-c:a" << "flac";

    // メタデータオプションの追加
    if(!metaData.title.isEmpty()){ ffmpeg_option << "-metadata" << "title=" + EncloseDQ(metaData.title); }
    if(!metaData.artist.isEmpty()){ ffmpeg_option << "-metadata" << "artist=" + EncloseDQ(metaData.artist); }
    if(!metaData.albumTitle.isEmpty()){ ffmpeg_option << "-metadata" << "album=" + EncloseDQ(metaData.albumTitle); }
    if(!metaData.albumArtist.isEmpty()){ ffmpeg_option << "-metadata" << "band=" + EncloseDQ(metaData.albumArtist); }
    if(!metaData.composer.isEmpty()){ ffmpeg_option << "-metadata" << "composer=" + EncloseDQ(metaData.composer); }
    if(!metaData.genre.isEmpty()){ ffmpeg_option << "-metadata" << "genre=" + EncloseDQ(metaData.genre); }
    if(!metaData.year.isEmpty()){ ffmpeg_option << "-metadata" << "date=" + EncloseDQ(metaData.year); }
    ffmpeg_option << "-metadata" << "track=" + EncloseDQ(metaData.track_no + "/" + QString::number(numEncodingMusic));
    ffmpeg_option << "-metadata" << "disc=1/1";

    // 入力と出力ファイルオプションの追加
    ffmpeg_option << outputFile.replace("\\", "/");

    /*
    QStringList flac_option;
    outputFile = outputFile.replace("\\", "/");

    flac_option << "-o" << EncloseDQ(outputFile);
    flac_option << "--compression-level-5";
    if(metaData.title != ""){ flac_option << QString("--tag=TITLE=\"%1\"").arg(metaData.title); }
    if(metaData.artist != ""){ flac_option << QString("--tag=ARTIST=\"%1\"").arg(metaData.artist); }
    if(metaData.albumTitle != ""){ flac_option << QString("--tag=ALBUM=\"%1\"").arg(metaData.albumTitle); }
    if(metaData.albumArtist != ""){ flac_option << QString("--tag=ALBUMARTIST=\"%1\"").arg(metaData.albumArtist); }
    if(metaData.composer != ""){ flac_option << QString("--tag=COMPOSER=\"%1\"").arg(metaData.composer); }
    if(metaData.genre != ""){ flac_option << QString("--tag=GENRE=\"%1\"").arg(metaData.genre); }
    if(metaData.year != ""){ flac_option << QString("--tag=DATE=\"%1\"").arg(metaData.year); }
    flac_option << QString("--tag=TRACKNUMBER=\"%1\"").arg(metaData.track_no);
    flac_option << "--tag=DISCNUMBER=1";
    flac_option << EncloseDQ(inputPath.replace("\\", "/"));

    auto artworkPath = metaData.artworkPath.toLocal8Bit().replace("\\", "/");

    connect(process, &QProcess::readChannelFinished, this, [this, artworkPath, outputFile, inputPath, metaData]()
    {
        if(QFile::exists(artworkPath) == false){
            emit this->encodeFinish(inputPath, metaData);
            return;
        }

#ifdef QT_DEBUG
        emit this->readStdOut(metaprocess->program() + " ");
        emit this->readStdOut(metaprocess->arguments().join(" ") + "\n");
#endif
        metaprocess->setArguments(QStringList{QString("--import-picture-from=\"%1\"").arg(artworkPath), EncloseDQ(outputFile)});
        metaprocess->start();
        if (!metaprocess->waitForStarted(-1)) {
            qWarning() << metaprocess->errorString();
        }
    });
    */

#ifdef QT_DEBUG
    emit this->readStdOut(process->program() + " ");
    emit this->readStdOut(process->arguments().join(" ") + "\n");
#endif

    process->setArguments(ffmpeg_option);
    process->start();
    if (!process->waitForStarted(-1)) {
        qWarning() << process->errorString();
        return false;
    }

    return true;
}
