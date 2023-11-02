#ifndef FLACENCODER_H
#define FLACENCODER_H

#include "EncoderInterface.h"

class FlacEncoder : public EncoderInterface
{
    Q_OBJECT
public:
    FlacEncoder();
    ~FlacEncoder() override;

    bool Encode(QString inputPath, AudioMetaData metaData, int processNumber) override;

    QString GetEncoderFileName() const override { return "refalac"; }
    QString GetCodecExtention() const override { return "m4a"; }

private:
};

#endif // FLACENCODER_H
