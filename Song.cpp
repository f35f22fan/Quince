#include "Song.hpp"

#include "audio/TempSongInfo.hpp"
#include "ByteArray.hpp"
#include "io/File.hpp"

#include <QUrl>

namespace quince {

void
Song::Apply(const audio::Info &info)
{
	meta_.bitrate(info.bitrate);
	meta_.channels(info.channels);
	meta_.sample_rate(info.sample_rate);
	meta_.duration(info.duration);
}

void
Song::FillIn(audio::TempSongInfo &info)
{
	info.duration = meta_.duration();
	info.song = this;
	info.uri = uri_;
	info.position = position_;
	info.playlist_id = playlist_id_;
	info.state_ = state_;
}

Song*
Song::From(quince::ByteArray &ba, const i64 playlist_id)
{
	Song *song = new Song();
	song->display_name(ba.next_string());
	song->uri(ba.next_string());
	song->dir_path(ba.next_string());
	song->position(ba.next_i64());
	song->playlist_id(playlist_id);
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
Song::FromFile(const io::File &file, const i64 playlist_id)
{
	QStringRef ext = file.Extension();
	
	if (ext.isNull()) {
//		mtl_info("file extension is null");
		return nullptr;
	}
	
	QString lower = ext.toString().toLower();
	audio::Codec audio_codec = audio::Codec::Unknown;
	
	if (lower == QLatin1String("mp3"))
		audio_codec = audio::Codec::Mp3;
	else if (lower == QLatin1String("flac"))
		audio_codec = audio::Codec::Flac;
	else if (lower == QLatin1String("opus"))
		audio_codec = audio::Codec::OggOpus;
	else if (lower == QLatin1String("mka"))
		audio_codec = audio::Codec::Mka;
	else
		return nullptr;
	
	auto *song = new Song();
	song->display_name(file.name);
	song->playlist_id(playlist_id);
	song->dir_path(file.dir_path);
	
	audio::Meta &meta = song->meta();
	meta.audio_codec(audio_codec);
	
	QString uri_path = QLatin1String("file://") + file.build_full_path();
	song->uri(QUrl(uri_path).toEncoded());
	
	return song;
}

void
Song::SaveTo(quince::ByteArray &ba)
{
	ba.add_string(display_name_);
	ba.add_string(uri_);
	ba.add_string(dir_path_);
	ba.add_i64(position_);
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
