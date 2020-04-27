#pragma once

#include "types.hxx"

#include "io/io.hh"

#include <QByteArray>
#include <QString>

namespace quince {

enum class AudioCodec : u8 {
	Unknown,
	Mp3,
	Opus,
	Flac,
};

class SongItem {
public:
	
	AudioCodec
	audio_codec() const { return audio_codec_; }
	
	void
	audio_codec(const AudioCodec codec) { audio_codec_= codec; }
	
	const QString& display_name() const { return display_name_; }
	void dispay_name(const QString &s) { display_name_ = s; }
	
	i64 duration_ns() { return duration_ns_; }
	void duration_ns(i64 n) { duration_ns_ = n; }
	void duration_seconds(i32 seconds) {
		duration_ns_ =  1000L * 1000L * 1000L * seconds; }
	
	const QString&
	uri() const { return uri_; }
	
	void
	uri(const QString &s) { uri_ = s; }
	
	static SongItem*
	FromFile(const io::File &file, const QString &dir_path);
	
	bool
	is_codec_unknown() const { return audio_codec_ == AudioCodec::Unknown; }
	
	bool
	is_flac() const { return audio_codec_ == AudioCodec::Flac; }
	
	bool
	is_mp3() const { return audio_codec_ == AudioCodec::Mp3; }
	
	bool
	is_playing() const { return playing_at_ != -1; }
	
	i64
	playing_at() const { return playing_at_; }
	
	void
	playing_at(const i64 t) { playing_at_ = t; }
	
private:
	i64 duration_ns_ = -1;
	i64 playing_at_ = -1;
	QString display_name_;
	QString uri_;
	bool playing_ = false;
	AudioCodec audio_codec_ = AudioCodec::Unknown;
};

}
