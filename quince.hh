#pragma once

#include "types.hxx"

namespace quince {
enum class LinuxDisplayType : u8 {
	Wayland,
	X11,
	None
};

LinuxDisplayType GetLinuxDisplayType();
}
