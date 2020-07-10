#include "File.hpp"

namespace quince::io {

QStringRef
File::Extension() const
{
	int index = name.lastIndexOf('.');
	
	if (index == -1 || index == (name.size() - 1))
		return QStringRef();
	
	return name.midRef(index + 1);
}

}
