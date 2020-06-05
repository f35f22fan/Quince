#pragma once

#include "io.hxx"

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <sys/sysmacros.h>

#include <QStringRef>
#include <QVector>

namespace io {

typedef bool (*FilterFunc)(const QString &dir_path, const QString &name);

bool
EnsureDir(const QString &dir_path, const QString &subdir);

bool
FileExists(const char *path, FileType *file_type = nullptr);

io::Err
FileFromPath(File &file, const QString &full_path);

void
FillIn(io::File &file, const struct stat &st, const QString &dir_path, const QString  &name);

QStringRef
GetFilenameExtension(const QString &name);

bool
IsSongExtension(const QString &dir_path, const QString &filename);

io::Err
ListFiles(const QString &full_dir_path, QVector<io::File> &vec,
	const u8 options, FilterFunc ff = nullptr);

io::Err
MapPosixError(int e);

inline FileType
MapPosixTypeToLocal(const mode_t mode) {
	switch (mode & S_IFMT) {
	case S_IFREG: return FileType::Regular;
	case S_IFDIR: return FileType::Dir;
	case S_IFLNK: return FileType::Symlink;
	case S_IFBLK: return FileType::BlockDevice;
	case S_IFCHR: return FileType::CharDevice;
	case S_IFIFO: return FileType::Pipe;
	case S_IFSOCK: return FileType::Socket;
	default: return FileType::Unknown;
	}
}

io::Err
WriteToFile(const QString &full_path, const char *data, const i64 size);

}
