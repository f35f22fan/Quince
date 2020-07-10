#include "io.hh"

#include "File.hpp"
#include "../err.hpp"
#include "../ByteArray.hpp"

#include <QDir>
#include <QFileInfo>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace quince::io {

bool
EnsureDir(const QString &dir_path, const QString &subdir)
{
	auto d = dir_path;
	
	if (!d.endsWith('/'))
		d.append('/');
	
	d.append(subdir);
	
	auto ba = d.toLocal8Bit();
	FileType ft;
	
	if (FileExists(ba.data(), &ft))
	{
		if (ft != FileType::Dir)
		{
			if (remove(ba.data()) == 0)
				return mkdir(ba.data(), DirPermissions) == 0;
			return false;
		}
		
		return true;
	}
	
	return mkdir(ba.data(), DirPermissions) == 0;
}

bool
FileExists(const char *path, FileType *file_type)
{
	struct stat st;
	
	if (lstat(path, &st) != 0)
		return false;
	
	if (file_type != nullptr)
		*file_type = MapPosixTypeToLocal(st.st_mode);
	
	return true;
}

io::Err
FileFromPath(io::File &file, const QString &full_path)
{
	struct stat st;
	auto ba = full_path.toLocal8Bit();
	
	if (lstat(ba.data(), &st) == -1)
		return MapPosixError(errno);
	
	QFileInfo info(full_path);
	const QString parent_dir = info.dir().absolutePath();
	FillIn(file, st, parent_dir, info.fileName());
	
	return io::Err::Ok;
}

void
FillIn(io::File &file, const struct stat &st, const QString &dir_path,
	const QString &name)
{
	using io::FileType;
	file.name = name;
	file.dir_path = dir_path;
	file.size = st.st_size;
	file.type_ = MapPosixTypeToLocal(st.st_mode);
	file.id = io::FileID {
		.device_id = st.st_dev,
		.inode_number = st.st_ino
	};
}

QStringRef
GetFilenameExtension(const QString &name)
{
	int dot = name.lastIndexOf('.');
	
	if (dot == -1 || (dot == name.size() - 1))
		return QStringRef();
	
	return name.midRef(dot + 1);
}

bool
IsSongExtension(const QString &dir_path, const QString &filename)
{
	QStringRef ext_ref = io::GetFilenameExtension(filename);
	
	if (ext_ref.isEmpty())
		return false;
	
	QString ext = ext_ref.toString().toLower();
	
	if (ext == "mp3" || ext == "opus" || ext == "flac"
		|| ext == "mka" || ext == "m4a" || ext == "webm")
		return true;
	
	return false;
}

io::Err
ListFileNames(const QString &full_dir_path, QVector<QString> &vec)
{
	struct dirent *entry;
	auto dir_path_ba = full_dir_path.toLocal8Bit();
	DIR *dp = opendir(dir_path_ba.data());
	
	if (dp == NULL)
		return MapPosixError(errno);
	
	QString dir_path = full_dir_path;
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	struct stat st;
	const QChar DotChar('.');
	
	while ((entry = readdir(dp)))
	{
		QString name(entry->d_name);
		
		if (name.startsWith(DotChar))
		{
			if (name == QLatin1String(".") || name == QLatin1String(".."))
				continue;
		}
		
		vec.append(name);
	}
	
	closedir(dp);
	
	return Err::Ok;
}

io::Err
ListFiles(const QString &full_dir_path, QVector<io::File> &vec,
	const u8 options, FilterFunc ff)
{
	struct dirent *entry;
	auto dir_path_ba = full_dir_path.toLocal8Bit();
	DIR *dp = opendir(dir_path_ba.data());
	
	if (dp == NULL)
		return MapPosixError(errno);
	
	QString dir_path = full_dir_path;
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	struct stat st;
	const bool list_hidden_files = options & u8(io::ListOptions::HiddenFiles);
	const QChar DotChar('.');
	
	while ((entry = readdir(dp)))
	{
		QString name(entry->d_name);
		
		if (!list_hidden_files && name.startsWith(DotChar))
		{
			if (name == QLatin1String(".") || name == QLatin1String(".."))
				continue;
		}
		
		if (ff != nullptr && !ff(dir_path, name))
			continue;
		
		QString full_path = dir_path + name;
		auto ba = full_path.toLocal8Bit();
		
		if (lstat(ba.data(), &st) == -1)
		{
			perror("Failed reading file");
			closedir(dp);
			return MapPosixError(errno);
		}
		
		io::File file;
		FillIn(file, st, dir_path, name);
		vec.append(file);
	}
	
	closedir(dp);
	
	return Err::Ok;
}

Err
MapPosixError(int e)
{
	using io::Err;
	
	switch (e) {
	case EACCES: return Err::Access;
	case EIO: return Err::IO;
	default: return Err::Other;
	}
}

io::Err
ReadFile(const QString &full_path, quince::ByteArray &buffer)
{
	struct stat st;
	auto path = full_path.toLocal8Bit();
	
	if (lstat(path.data(), &st) != 0)
		return MapPosixError(errno);
	
	buffer.alloc(st.st_size);
	char *buf = buffer.data();
	const int fd = open(path.data(), O_RDONLY);
	
	if (fd == -1)
		return MapPosixError(errno);
	
	usize so_far = 0;
	
	while (so_far < st.st_size) {
		isize ret = read(fd, buf + so_far, st.st_size - so_far);
		
		if (ret == -1) {
			if (errno == EAGAIN)
				continue;
			io::Err e = MapPosixError(errno);
			close(fd);
			return e;
		}
		
		so_far += ret;
	}
	
	close(fd);
	
	return Err::Ok;
}

io::Err
WriteToFile(const QString &full_path, const char *data, const i64 size)
{
	auto path = full_path.toLocal8Bit();
	const int fd = open(path.data(), O_WRONLY | O_CREAT | O_TRUNC,
		io::FilePermissions);
	
	if (fd == -1)
		return MapPosixError(errno);
	
	isize written = 0;
	
	while (written < size) {
		// ssize_t write(int fd, const void *buf, size_t count);
		isize ret = write(fd, data + written, size - written);
		
		if (ret == -1) {
			if (errno == EAGAIN)
				continue;
			io::Err e = MapPosixError(errno);
			close(fd);
			return e;
		}
		
		written += ret;
	}
	
	close(fd);
	
	return Err::Ok;
}

} // io::
