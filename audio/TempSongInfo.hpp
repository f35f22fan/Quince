#pragma once

#include "../decl.hxx"
#include "../types.hxx"

#include <gst/gst.h>

namespace quince::audio {

class TempSongInfo {
public:
	
	bool
	has_data() const { return duration != -1 || position != -1; }
	
	bool
	is_paused() const { return state_ == GST_STATE_PAUSED; }
	
	bool
	is_playing() const { return state_ == GST_STATE_PLAYING; }
	
	bool
	is_playing_or_paused() const { return is_playing() || is_paused(); }
	
	QString uri;
	quince::Song *song = nullptr; // might be null at any time
	i64 duration = -1;
	i64 position = -1;
	i64 playlist_id = -1;
	GstState state_ = GST_STATE_NULL;
};

}
