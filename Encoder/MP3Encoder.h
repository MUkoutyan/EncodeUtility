#ifndef MP3ENCODER_H
#define MP3ENCODER_H

#include "EncoderInterface.h"

class MP3Encoder : public EncoderInterface
{
public:
    MP3Encoder();
    ~MP3Encoder() override;

    bool Encode(QString inputPath, AudioMetaData metaData, int processNumber) override;

    QString GetEncoderFileName() const override { return "lame"; }
    QString GetCodecExtention() const override { return "mp3"; }

private:
};

#endif // MP3ENCODER_H
