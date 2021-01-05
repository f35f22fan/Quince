#pragma once

#include <QString>

namespace quince::actions {
const auto AddSongFilesToPlaylist = QLatin1String("AddSongFilesToPlaylist");
const auto MediaPlayPause = QLatin1String("MediaPlayPause");
const auto MediaPlayStop = QLatin1String("MediaPlayStop");
const auto MediaPlayNext = QLatin1String("MediaPlayNext");
const auto MediaPlayPrev = QLatin1String("MediaPlayPrev");
const auto PlaylistNew = QLatin1String("PlaylistNew");
const auto PlaylistDelete = QLatin1String("PlaylistDelete");
const auto PlaylistRemoveAllEntries = QLatin1String("PlaylistRemoveAllEntries");
const auto PlaylistRename = QLatin1String("PlaylistRename");
const auto QuitApp = QLatin1String("QuitApp");
const auto RemoveSongsFromPlaylist = QLatin1String("RemoveSongsFromPlaylist");
const auto RemoveSongsAndDeleteFiles = QLatin1String("Remove songs & delete files");
const auto ShowSongFolderPath = QLatin1String("Show song folder path");
const auto ShowHideWindow = QStringLiteral("Show/Hide");
}
