#ifndef ENCODER_H
#define ENCODER_H

#include "AudioMetaData.hpp"

class Encoder
{
public:
    Encoder(const ProjectMetaData& metaData);

private:
    const ProjectMetaData& metaData;
};

#endif // ENCODER_H
