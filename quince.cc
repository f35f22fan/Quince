#include "quince.hh"

#include <QProcessEnvironment>

namespace quince {

LinuxDisplayType
GetLinuxDisplayType()
{
// Inspired from:
// stackoverflow.com/questions/45536141/how-i-can-find-out-if-a-linux-system-uses-wayland-or-x11
	
	auto env = QProcessEnvironment::systemEnvironment();
	
	QString value = env.value(QLatin1String("WAYLAND_DISPLAY"));
	if (!value.isEmpty())
		return LinuxDisplayType::Wayland;
	
	value = env.value(QLatin1String("DISPLAY"));
	if (!value.isEmpty())
		return LinuxDisplayType::X11;
	
	return LinuxDisplayType::None;
}

}
