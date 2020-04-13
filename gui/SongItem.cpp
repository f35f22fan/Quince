#include "SongItem.hpp"

namespace quince::gui {

SongItem*
SongItem::New(const QString &name)
{
	auto *p = new SongItem();
	p->dispay_name(name);
	//p->duration_ns(d);
	
	return p;
}


}
