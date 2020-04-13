#pragma once

#include "err.hpp"
#include "types.hxx"

#include <QString>
#include <gst/gst.h>

namespace quince {

class GstPlayer {
public:
	GstPlayer(int argc, char *argv[]);
	virtual ~GstPlayer();
	
	void
	Play(const QString &full_path);
	
private:
	NO_ASSIGN_COPY_MOVE(GstPlayer);
	
	void
	InitGst(int argc, char *argv[]);
	
	GstElement *play_elem_ = nullptr;
	
};
}
