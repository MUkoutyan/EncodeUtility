#ifndef PROJECTDEFINES_HPP
#define PROJECTDEFINES_HPP

#include <QStringList>

namespace ProjectDefines
{
    static constexpr char applicationVersion[] = "Version 1.0.4";
    static constexpr char projectExtention[] = ".encproj";
    static constexpr int  projectVersionNum  = 0x010002;
    static constexpr char projectVersion[]   = "1.0.2";

    static constexpr char settingOutputFolder[]     = "OutputFolder";
    static constexpr char settingCheckFFmpegFile[] = "CheckFFmpegFile";
    static const QStringList headerItems = {"No.", "Title", "Artist", "AlbumTitle", "AlbumArtist", "Composer", "Group", "Genre", "Year"};

    static QString settingFilePath;
};

#endif // PROJECTDEFINES_HPP
