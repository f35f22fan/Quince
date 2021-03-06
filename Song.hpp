#pragma once

#include "io/io.hh"
#include "audio.hxx"
#include "audio/decl.hxx"
#include "audio/Meta.hpp"
#include "decl.hxx"
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
	
	void
	Apply(const audio::Info &info);
	
	u8&
	bits() { return bits_; }
	
	const QString& dir_path() const { return dir_path_; }
	void dir_path(const QString  &s) { dir_path_ = s; }
	
	const QString& display_name() const { return display_name_; }
	void display_name(const QString &s) { display_name_ = s; }
	
	audio::Meta&
	meta() { return meta_; }
	
	void
	FillIn(audio::TempSongInfo &info);
	
	static Song*
	From(quince::ByteArray &ba, const i64 playlist_id);
	
	static Song*
	FromFile(const io::File &file, const i64 playlist_id);
	
	bool
	is_paused() const { return state_ == GST_STATE_PAUSED; }
	
	bool
	is_playing() const { return state_ == GST_STATE_PLAYING; }
	
	bool
	is_playing_or_paused() const { return is_playing() || is_paused(); }
	
	void
	mark_for_deletion(const bool f = true) {
		if (f)
			bits_ |= u8(SongBits::MarkForDeletion);
		else
			bits_ &= ~u8(SongBits::MarkForDeletion);
	}
	
	bool
	marked_for_deletion() const { return bits_ & u8(SongBits::MarkForDeletion); }
	
	i64 playlist_id() const { return playlist_id_; }
	void playlist_id(i64 n) { playlist_id_ = n; }
	
	i64 position() const { return position_; }
	void position(const i64 t) { position_ = t; }
	
	void
	SaveTo(quince::ByteArray &ba);
	
	GstState state() const { return state_; }
	void state(GstState s) { state_ = s; }
	
	const QString& uri() const { return uri_; }
	void uri(const QString &s) { uri_ = s; }
	
private:
	GstState state_ = GST_STATE_NULL;
	i64 position_ = -1;
	i64 playlist_id_ = -1;
	QString display_name_;
	QString uri_;
	QString dir_path_;
	audio::Meta meta_ = {};
	u8 bits_ = 0;
};

}
