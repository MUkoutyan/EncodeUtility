#ifndef AUDIOMETADATA_HPP
#define AUDIOMETADATA_HPP

#include <QString>
#include <vector>

struct AudioMetaData
{
    QString title;
    QString track_no;
    QString artist;
    QString albumTitle;
    QString albumArtist;
    QString genre;
    QString group;
    QString composer;
    QString year;
    QString artworkPath;
};

struct ProjectMetaData
{
    std::string projectPath;
    std::string artworkPath;
    std::vector<AudioMetaData> audioMetaData;
};

#endif // AUDIOMETADATA_HPP
