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
    QStringList option;
    option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        option << "-i" << artworkPath
                      << "-map" << "0:0" << "-map" << "1:0"
                      << "-c:v" << "mjpeg" << "-disposition:v:0" << "attached_pic";
    }
    option << "-c:a" << "flac";

    // メタデータオプションの追加
    AppendCommonMetaDataOption(option, metaData);

    // 入力と出力ファイルオプションの追加
    option << outputFile.replace("\\", "/");

#ifdef QT_DEBUG
    emit this->readStdOut(process->program() + " ");
    emit this->readStdOut(process->arguments().join(" ") + "\n");
#endif

    process->setArguments(option);
    process->start();
    if (!process->waitForStarted(-1)) {
        qWarning() << process->errorString();
        return false;
    }

    return true;
}
