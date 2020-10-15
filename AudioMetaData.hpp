#ifndef AUDIOMETADATA_HPP
#define AUDIOMETADATA_HPP

#include <string>
#include <vector>

struct AudioMetaData
{
    std::string track_no;
    std::string title;
    std::string albumTitle;
    std::string artist;
    std::string genre;
};

struct ProjectMetaData
{
    std::string projectPath;
    std::string artworkPath;
    std::vector<AudioMetaData> audioMetaData;
};

#endif // AUDIOMETADATA_HPP
