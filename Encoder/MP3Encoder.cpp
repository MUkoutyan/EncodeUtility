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
    QStringList option;
    option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        option << "-i" << artworkPath
                      << "-map" << "0" << "-map" << "1"
                      << "-metadata:s:v" << "title=\"Album cover\""
                      << "-metadata:s:v" << "comment=\"Cover (front)\""
                      << "-c:v" << "copy"
                      << "-id3v2_version" << "3";
    }
    option << "-c:a" << "libmp3lame" << "-q:a" << "0";

    AppendCommonMetaDataOption(option, metaData);

    // 入力と出力ファイルオプションの追加
    option << outputFile.replace("\\", "/");

    process->setArguments(option);

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
