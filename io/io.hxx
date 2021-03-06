#pragma once

#include "../types.hxx"
#include <QString>
#include <sys/stat.h>
#include <sys/types.h>

namespace quince::io {

class File;

static const mode_t DirPermissions = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
static const mode_t FilePermissions = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

enum class Err : u8 {
	Ok = 0,
	Access, // permission denied
	Perm, // operation not permitted
	IO, // I/O error
	Other
};

enum class FileType : u8 {
	Unknown = 0,
	Regular,
	Dir,
	Symlink,
	Socket,
	Pipe,
	BlockDevice,
	CharDevice
};

struct FileID {
	dev_t device_id; // ID of device containing file
	ino_t inode_number;
	
	bool
	operator == (const FileID &rhs) const {
		return Equals(rhs);
	}
	
	bool
	Equals(const FileID &rhs) const {
		return inode_number == rhs.inode_number &&
			device_id == rhs.device_id;
	}
	
	bool
	Initialized() const {
		return inode_number != 0 || device_id != 0;
	}
};

enum class ListOptions : u8 {
	HiddenFiles = 1u << 0,
};

}
