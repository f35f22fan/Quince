#include "Song.hpp"

#include <QUrl>

namespace quince {

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
	p->dispay_name(file.name);
	
	audio::Meta &meta = p->meta();
	meta.audio_codec(audio_codec);
	
	QString uri_path = QLatin1String("file://") + file.dir_path + file.name;
	p->uri(QUrl(uri_path).toEncoded());
	
	return p;
}

}
