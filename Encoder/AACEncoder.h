#ifndef AACENCODER_H
#define AACENCODER_H

#include "EncoderInterface.h"

class AACEncoder : public EncoderInterface
{
    Q_OBJECT
public:
    AACEncoder();
    ~AACEncoder() override;

    bool Encode(QString inputPath, AudioMetaData metaData, int processNumber) override;

    QString GetEncoderFileName() const override { return "qaac"; }
    QString GetCodecExtention() const override { return "m4a"; }

private:

};

#endif // AACENCODER_H
