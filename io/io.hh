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
FileExists(const char *path);

QStringRef
GetFilenameExtension(const QString &name);

bool
IsSongExtension(const QString &dir_path, const QString &filename);

io::Err
ListFiles(const QString &full_dir_path, QVector<io::File> &vec,
	const u8 options, FilterFunc ff = nullptr);

io::Err
MapPosixError(int e);

}
