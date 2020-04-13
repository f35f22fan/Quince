#include "io.hh"

namespace io {

bool
FileExists(const char *path)
{
	struct stat st;
	return lstat(path, &st) == 0;
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
	
	if (ext == "mp3" || ext == "opus" || ext == "webm" || ext == "flac"
		|| ext == "mka" || ext == "m4a")
		return true;
	
	return false;
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
	int count = 0;
	const bool list_hidden_files = options & u8(io::ListOptions::HiddenFiles);
	bool check = true;
	const QChar DotChar('.');
	
	while ((entry = readdir(dp)))
	{
		QString name(entry->d_name);
		
		if (name.startsWith(DotChar))
		{
			if (!list_hidden_files)
				continue;
			
			if (check && (name == "." || name == ".."))
			{
				count++;
				check = count < 2;
				continue;
			}
		}
		
		if (ff != nullptr)
		{
			if (!ff(dir_path, name))
				continue;
		}
			
		
		QString full_path = dir_path + name;
		auto ba = full_path.toLocal8Bit();
		
		if (lstat(ba.data(), &st) == -1)
		{
			perror("Failed reading file");
			closedir(dp);
			return MapPosixError(errno);
		}
		
		io::File file;
		using io::FileType;
		file.name = name;
		file.size = st.st_size;
		file.id = io::FileID {
			.device_id = st.st_dev,
			.inode_number = st.st_ino
		};
		
		switch (st.st_mode & S_IFMT)
		{
		case S_IFREG: file.type_ = FileType::Regular; break;
		case S_IFDIR: file.type_ = FileType::Dir; break;
		case S_IFLNK: file.type_ = FileType::Symlink; break;
		case S_IFBLK: file.type_ = FileType::Block; break;
		case S_IFCHR: file.type_ = FileType::Char; break;
		case S_IFIFO: file.type_ = FileType::Pipe; break;
		case S_IFSOCK: file.type_ = FileType::Socket; break;
		default: file.type_ = FileType::Unknown;
		}
		
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

} // io::
