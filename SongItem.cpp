#include "SongItem.hpp"

#include <QUrl>

namespace quince {

SongItem*
SongItem::FromFile(const io::File &file, const QString &dir_path)
{
	auto *p = new SongItem();
	p->dispay_name(file.name);
	
	AudioCodec audio_codec = AudioCodec::Unknown;
	
	QString lower = file.name.toLower();
	
	if (lower.endsWith(".mp3"))
		audio_codec = AudioCodec::Mp3;
	else if (lower.endsWith(".flac"))
		audio_codec = AudioCodec::Flac;
	
	p->audio_codec(audio_codec);
	
	QString uri_path = QLatin1String("file://") + dir_path + file.name;
	p->uri(QUrl(uri_path).toEncoded());
	
	return p;
}

}
