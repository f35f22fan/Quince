#pragma once

#include "io/io.hh"
#include "audio.hxx"
#include "audio/Meta.hpp"
#include "types.hxx"

#include <gst/gst.h>
#include <QByteArray>
#include <QString>

namespace quince {

enum class SongBits: u8 {
	None = 0,
	MarkForDeletion = 1,
};

class Song {
public:
	
	u8&
	bits() { return bits_; }
	
	const QString& dir_path() const { return dir_path_; }
	void dir_path(const QString  &s) { dir_path_ = s; }
	
	const QString& display_name() const { return display_name_; }
	void dispay_name(const QString &s) { display_name_ = s; }
	
	audio::Meta&
	meta() { return meta_; }
	
	const QString& uri() const { return uri_; }
	void uri(const QString &s) { uri_ = s; }
	
	static Song*
	FromFile(const io::File &file);
	
	bool
	is_paused() const { return state_ == GST_STATE_PAUSED; }
	
	bool
	is_playing() const { return state_ == GST_STATE_PLAYING; }
	
	bool
	is_playing_or_paused() const { return is_playing() || is_paused(); }
	
	GstState state() const { return state_; }
	void state(GstState s) { state_ = s; }
	
	i64 playing_at() const { return playing_at_; }
	void playing_at(const i64 t) { playing_at_ = t; }
	
private:
	GstState state_ = GST_STATE_NULL;
	i64 playing_at_ = -1;
	QString display_name_;
	QString uri_;
	QString dir_path_;
	audio::Meta meta_ = {};
	u8 bits_ = 0;
};

}
