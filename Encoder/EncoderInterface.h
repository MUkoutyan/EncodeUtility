#ifndef ENCODERINTERFACE_H
#define ENCODERINTERFACE_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QProcess>
#include <QObject>

#include "ProjectDefines.hpp"
#include "AudioMetaData.hpp"

class EncoderInterface : public QObject
{
    Q_OBJECT
public:
    EncoderInterface() : isAddTrackNo(false),numOfDigit(0),numTotalFiles(0),numEncodingMusic(0){
    }
    ~EncoderInterface(){}

    virtual QString GetEncoderFileName() const = 0;
    virtual QString GetCodecExtention() const = 0;

    virtual bool Encode(QString inputPath, AudioMetaData metaData, int processNumber) = 0;

    void SetOutputFolderPath(QString path){
        outputBaseFolderPath = std::move(path);
    }

    void SetIsAddTrackNo(bool newIsAddTrackNo){
        isAddTrackNo = newIsAddTrackNo;
    }

    void SetNumOfDigit(int newNumOfDigit){
        numOfDigit = newNumOfDigit;
    }

    void SetTrackNumberDelimiter(const QString &newTrackNumberDelimiter){
        trackNumberDelimiter = newTrackNumberDelimiter;
    }

    void SetNumEncodingMusic(int newNumEncodingMusic){
        numEncodingMusic = newNumEncodingMusic;
    }

    QString GetCodecFolderName() const{
        return codecFolderName;
    }

    void SetCodecFolderName(const QString &newCodecFolderName){
        codecFolderName = newCodecFolderName;
    }

    bool GetIsEnableEncoder() const{
        return isEnableEncoder;
    }
    void SetIsEnableEncoder(bool newIsEnableEncoder){
        isEnableEncoder = newIsEnableEncoder;
    }


signals:
    void readStdOut(QString);
    void readStdError(QString);
    void encodeFinish(const QString inputPath, const AudioMetaData& AudioMetaData);

protected:
    QString GetOutputPath(QString title, QString extension, int i) const
    {
        auto outputFolder = outputBaseFolderPath+"/"+ GetCodecFolderName();
        if(QDir(outputFolder).exists() == false){
            QDir(outputBaseFolderPath).mkdir(GetCodecFolderName());
        }
        QString outputFile = outputFolder+"/"+title+extension;
        if(this->isAddTrackNo){
            outputFile = outputBaseFolderPath +"/"+ GetCodecFolderName() +"/"+QString("%1%2%3").arg(i+1, this->numOfDigit).arg(trackNumberDelimiter).arg(title)+extension;
        }
        return outputFile;
    };

    void SaveSettingFile(QString key, QVariant value) const
    {
        QSettings settingfile(ProjectDefines::settingFilePath, QSettings::IniFormat);
        settingfile.setValue(key, value);
    }

    QVariant LoadSettingValue(QString key, QVariant defval = QVariant()) const
    {
        QSettings settingfile(ProjectDefines::settingFilePath, QSettings::IniFormat);
        return settingfile.value(key, defval);
    }

    inline QString EncloseDQ(QString str) const{
        return str;
    }

    void AppendCommonMetaDataOption(QStringList& options, const AudioMetaData& metaData)
    {
        // メタデータオプションの追加
        if(!metaData.title.isEmpty()){ options << "-metadata" << "title=" + EncloseDQ(metaData.title); }
        if(!metaData.artist.isEmpty()){ options << "-metadata" << "artist=" + EncloseDQ(metaData.artist); }
        if(!metaData.albumTitle.isEmpty()){ options << "-metadata" << "album=" + EncloseDQ(metaData.albumTitle); }
        if(!metaData.albumArtist.isEmpty()){ options << "-metadata" << "band=" + EncloseDQ(metaData.albumArtist); }
        if(!metaData.composer.isEmpty()){ options << "-metadata" << "composer=" + EncloseDQ(metaData.composer); }
        if(!metaData.genre.isEmpty()){ options << "-metadata" << "genre=" + EncloseDQ(metaData.genre); }
        if(!metaData.year.isEmpty()){ options << "-metadata" << "date=" + EncloseDQ(metaData.year); }
        options << "-metadata" << "track=" + EncloseDQ(metaData.track_no + "/" + QString::number(numEncodingMusic));
        options << "-metadata" << "disc=1/1";
    }

    bool isEnableEncoder;
    bool isAddTrackNo;
    int numOfDigit;
    int numTotalFiles;      //処理するファイル数
    int numEncodingMusic;   //コーデックを抜きにした曲数
    QString trackNumberDelimiter;

    QString outputBaseFolderPath;   //出力先のルートフォルダパス
    QString codecFolderName;        //ルートの下に作る、コーデックごとのフォルダ名

};



#endif // ENCODERINTERFACE_H
