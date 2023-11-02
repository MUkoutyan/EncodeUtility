#include "AACEncoder.h"

#include <QCheckBox>
#include <QUrl>
#include <QMessageBox>
#include <QDesktopServices>


AACEncoder::AACEncoder()
    : EncoderInterface()
{
    this->SetCodecFolderName("m4a");



}

AACEncoder::~AACEncoder(){
}

bool AACEncoder::Encode(QString inputPath, AudioMetaData metaData, int processNumber)
{
    QProcess* process = new QProcess(this);
    process->setProgram(qApp->applicationDirPath()+"/ffmpeg.exe");
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process](){
        QByteArray arr = process->readAllStandardOutput();
        emit this->readStdOut("[aac] " + QString(arr));
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process](){
        QByteArray arr = process->readAllStandardError();
        emit this->readStdOut("[aac] " + QString(arr));
    });
    connect(process, &QProcess::readChannelFinished, this, [this, inputPath, metaData](){
        emit this->encodeFinish(inputPath, metaData);
    });

    QString outputFile = GetOutputPath(metaData.title, ".m4a", processNumber);

    // AACエンコードオプション (AACのビットレートを指定)
    QStringList ffmpeg_option;
    ffmpeg_option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        ffmpeg_option << "-i" << artworkPath
                      << "-map" << "0" << "-map" << "1"
                      << "-c:v" << "mjpeg" << "-disposition:v:0" << "attached_pic";
    }
    ffmpeg_option << "-c:a" << "aac" << "-q:a" << "0";

    // メタデータオプションの追加
    if(!metaData.title.isEmpty()){ ffmpeg_option << "-metadata" << "title=" + EncloseDQ(metaData.title); }
    if(!metaData.artist.isEmpty()){ ffmpeg_option << "-metadata" << "artist=" + EncloseDQ(metaData.artist); }
    if(!metaData.albumTitle.isEmpty()){ ffmpeg_option << "-metadata" << "album=" + EncloseDQ(metaData.albumTitle); }
    if(!metaData.albumArtist.isEmpty()){ ffmpeg_option << "-metadata" << "band=" + EncloseDQ(metaData.albumArtist); }
    if(!metaData.composer.isEmpty()){ ffmpeg_option << "-metadata" << "composer=" + EncloseDQ(metaData.composer); }
    if(!metaData.genre.isEmpty()){ ffmpeg_option << "-metadata" << "genre=" + EncloseDQ(metaData.genre); }
    if(!metaData.year.isEmpty()){ ffmpeg_option << "-metadata" << "date=" + EncloseDQ(metaData.year); }
    ffmpeg_option << "-metadata" << "track=" + EncloseDQ(metaData.track_no + "/" + QString::number(numEncodingMusic));
    ffmpeg_option << "-metadata" << "disc=\"1/1\"";

    // 入力と出力ファイルオプションの追加
    ffmpeg_option << outputFile.replace("\\", "/");
    process->setArguments(ffmpeg_option);


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
