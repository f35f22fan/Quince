#pragma once

#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"
#include "types.hxx"

#include <QString>
#include <gst/gst.h>

namespace quince {

class GstPlayer {
public:
	GstPlayer(quince::App *app, int argc, char *argv[]);
	virtual ~GstPlayer();
	
	void
	PlayPause(Song *song);
	
	GstElement*
	play_elem() const { return play_elem_; }
	
	void
	SeekTo(const i64 new_pos);
	
private:
	NO_ASSIGN_COPY_MOVE(GstPlayer);
	
	void
	InitGst(int argc, char *argv[]);
	
	GstElement *play_elem_ = nullptr;
	quince::App *app_ = nullptr;
	
};
}
