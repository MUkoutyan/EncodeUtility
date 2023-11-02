#include "MP3Encoder.h"

#include <QCheckBox>
#include <QUrl>
#include <QMessageBox>
#include <QDesktopServices>

MP3Encoder::MP3Encoder()
    : EncoderInterface()
{
    this->SetCodecFolderName("mp3");
}

MP3Encoder::~MP3Encoder(){
}

bool MP3Encoder::Encode(QString inputPath, AudioMetaData metaData, int processNumber)
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

    QString outputFile = GetOutputPath(metaData.title, ".mp3", processNumber);

    // エンコードオプション
    QStringList ffmpeg_option;
    ffmpeg_option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        ffmpeg_option << "-i" << artworkPath
                      << "-map" << "0" << "-map" << "1"
                      << "-metadata:s:v" << "title=\"Album cover\""
                      << "-metadata:s:v" << "comment=\"Cover (front)\""
                      << "-c:v" << "copy"
                      << "-id3v2_version" << "3";
    }
    ffmpeg_option << "-c:a" << "libmp3lame" << "-q:a" << "0";

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

    process->setArguments(ffmpeg_option);

//    QStringList mp3_option;
//    mp3_option << "-b" << QString::number(320);
//    if(metaData.title!=""){ mp3_option << "--tt" <<  EncloseDQ(metaData.title); }
//    if(metaData.artist!=""){ mp3_option << "--ta" <<  EncloseDQ(metaData.artist); }
//    if(metaData.albumTitle!=""){ mp3_option << "--tl" <<  EncloseDQ(metaData.albumTitle); }
//    if(metaData.genre!=""){ mp3_option << "--tg" <<  EncloseDQ(metaData.genre); }
//    if(metaData.year!=""){ mp3_option << "--ty" <<  EncloseDQ(metaData.year); }
//    mp3_option << "--tn" <<  EncloseDQ(metaData.track_no + "/" + QString("%1").arg(numEncodingMusic));
//    auto artworkPath = metaData.artworkPath.replace("\\", "/");
//    if(QFile::exists(artworkPath)){ mp3_option << "--ti" << EncloseDQ(artworkPath); }
//    mp3_option << "-o" << EncloseDQ(outputFile);
//    mp3_option << EncloseDQ(inputPath);

//    process->setArguments(mp3_option);

#ifdef QT_DEBUG
    emit this->readStdOut(process->program() + " ");
    emit this->readStdOut(process->arguments().join(" ") + "\n");
#endif

    process->start();
    if (!process->waitForStarted(-1)) {
        qWarning() << process->errorString();
        return false;
    }

    return true;
}
