#pragma once

#include "audio.hxx"
#include "audio/TempSongInfo.hpp"
#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"
#include "types.hxx"

#include <QString>
#include <gst/gst.h>

namespace quince {

typedef void(GstPlayer::*PlayMethod)(Song *song);

class GstPlayer {
public:
	GstPlayer(quince::App *app, int argc, char *argv[]);
	virtual ~GstPlayer();
	
	
	void FinishUpPlayFunction(Song *song);
	GstElement* play_elem() const { return play_elem_; }
	void Pause(Song *song);
	void Play(Song *song);
	void SeekTo(const i64 new_pos);
	void SetSeekAndPause_Start(Song *song, PlayMethod play_method);
	void SetSeekAndPause_Finish();
	void StopPlaying(Song *song);
	audio::TempSongInfo& temp_song_info() { return temp_song_info_; }
	
	struct set_seek_and_pause {
		bool pending = false;
		bool pending2 = false;
		Song *song = nullptr;
		i64 new_pos = -1;
		PlayMethod play_method = nullptr;
	} set_seek_and_pause_ = {};
	
	
private:
	NO_ASSIGN_COPY_MOVE(GstPlayer);
	
	void InitGst(int argc, char *argv[]);
	
	GstElement *play_elem_ = nullptr;
	quince::App *app_ = nullptr;
	audio::TempSongInfo temp_song_info_ = {};
};
}
