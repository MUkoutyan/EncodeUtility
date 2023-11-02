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
    QStringList option;
    option << "-y" << "-i" << inputPath;
    // アートワークオプションの追加
    auto artworkPath = metaData.artworkPath.replace("\\", "/");
    if(QFile::exists(artworkPath)){
        option << "-i" << artworkPath
                      << "-map" << "0" << "-map" << "1"
                      << "-c:v" << "mjpeg" << "-disposition:v:0" << "attached_pic";
    }
    option << "-c:a" << "aac" << "-q:a" << "0";

    // メタデータオプションの追加
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
