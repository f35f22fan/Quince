#include "Song.hpp"
#include "ByteArray.hpp"

#include <QUrl>

namespace quince {

Song*
Song::From(quince::ByteArray &ba)
{
	Song *song = new Song();
	song->display_name(ba.next_string());
	song->uri(ba.next_string());
	song->dir_path(ba.next_string());
	song->playing_at(ba.next_i64());
	song->state(GstState(ba.next_i32()));
	song->bits() = ba.next_u8();
	
	audio::Meta &meta = song->meta();
	meta.channels(ba.next_i8());
	meta.bits_per_sample(ba.next_i8());
	meta.sample_rate(ba.next_i32());
	meta.duration(ba.next_i64());
	meta.bitrate(ba.next_i32());
	meta.audio_codec(audio::Codec(ba.next_u8()));
	
	const u8 count = ba.next_u8();
	auto &vec = meta.genres();
	
	for (u8 i = 0; i < count; i++) {
		vec.append(audio::Genre(ba.next_i16()));
	}
	
	return song;
}

Song*
Song::FromFile(const io::File &file)
{
	audio::Codec audio_codec = audio::Codec::Unknown;
	QString lower = file.name.toLower();
	
	if (lower.endsWith(".mp3"))
		audio_codec = audio::Codec::Mp3;
	else if (lower.endsWith(".flac"))
		audio_codec = audio::Codec::Flac;
	else if (lower.endsWith(".opus"))
		audio_codec = audio::Codec::OggOpus;
	else
		return nullptr;
	
	auto *p = new Song();
	p->display_name(file.name);
	
	audio::Meta &meta = p->meta();
	meta.audio_codec(audio_codec);
	
	QString uri_path = QLatin1String("file://") + file.dir_path + file.name;
	p->uri(QUrl(uri_path).toEncoded());
	
	return p;
}

void
Song::SaveTo(quince::ByteArray &ba)
{
	ba.add_string(display_name_);
	ba.add_string(uri_);
	ba.add_string(dir_path_);
	ba.add_i64(playing_at_);
	GstState state = is_playing() ? GST_STATE_PAUSED : state_;
	ba.add_i32(i32(state));
	ba.add_u8(bits_);
	
	// now add audio::Meta
	ba.add_i8(meta_.channels());
	ba.add_i8(meta_.bits_per_sample());
	ba.add_i32(meta_.sample_rate());
	ba.add_i64(meta_.duration());
	ba.add_i32(meta_.bitrate());
	ba.add_u8(u8(meta_.audio_codec()));
	
	auto &vec = meta_.genres();
	const u8 count = vec.size();
	ba.add_u8(count);
	
	for (u8 i = 0; i < count; i++)
		ba.add_i16(i16(vec[i]));
}

}
