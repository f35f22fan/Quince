#pragma once

#include "../audio.hxx"

namespace quince::audio {

class Meta {
	
public:
	
	void
	audio_codec(const Codec codec) { audio_codec_ = codec; }
	
	i32
	bitrate() const { return bitrate_; }
	
	void
	bitrate(const i32 n) { bitrate_ = n; }
	
	i64 duration() const { return duration_; }
	
	void
	duration(const i64 d) { duration_ = d; }
	
	bool
	is_codec_unknown() const { return audio_codec_ == Codec::Unknown; }
	
	bool
	is_codec_flac() const { return audio_codec_ == Codec::Flac; }
	
	bool
	is_codec_mp3() const { return audio_codec_ == Codec::Mp3; }
	
	bool
	is_codec_ogg_opus() const { return audio_codec_ == Codec::OggOpus; }
	
	bool
	is_duration_set() const { return duration_ != -1; }
private:
	
	i64 duration_ = -1;
	i32 bitrate_ = -1;
	Codec audio_codec_ = Codec::Unknown;
};

}
