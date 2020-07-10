#pragma once

#include "io.hxx"

namespace quince::io {

class File {
public:
	
	QStringRef Extension() const;
	
	bool is_dir() const { return type_ == FileType::Dir; }
	bool is_regular() const { return type_ == FileType::Regular; }
	bool is_symlink() const { return type_ == FileType::Symlink; }
	
	QString
	build_full_path() const { return dir_path + '/' + name; }
	
	QString name;
	QString dir_path;
	i64 size = -1;
	FileType type_ = FileType::Unknown;
	FileID id;
};

}
