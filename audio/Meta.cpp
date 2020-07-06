#include "Meta.hpp"

#include "../audio.hh"

#include <QRegularExpression>

namespace quince::audio {

void
Meta::InterpretID3V1(const char *buf)
{
	int offset = 3;
	song_name_ = QString::fromLocal8Bit(buf + offset, 30);
	
	if (!song_name_.isEmpty())
	{
//		auto ba = song_name_.toLocal8Bit();
//		mtl_info("\"%s\"", ba.data());
	}
	
}

bool
StartsWith4(const char *buf, const char *str)
{
	return buf[0] == str[0] && buf[1] == str[1] &&
		buf[2] == str[2] && buf[3] == str[3];
}

i32
Meta::InterpretTagV2Frame(const char *frame_start, const i32 max_size,
	const char *full_path)
{
	i32 size;
	memcpy(&size, frame_start + 4, 4);
	size = quince::audio::syncsafe(size);
	
	if (size <= 0)
		return -1; // no more frames
	
	const auto str_buf = frame_start + 11;
	auto str_len = size - 1;
	
	if (str_len > 50000) {
		return -1;
	}
	
	i32 skip = 0;
	
	if (str_len > 2)
	{
		const u8 n = str_buf[0];
		
		if (n == 0xFF || n == 0xFE)
			skip = 2;
	}
	
	QString s;
	
	if (skip == 2) {
		str_len /= 2;
		s = QString::fromUtf16((const char16_t*)str_buf, str_len);
	} else {
		s = QString::fromLatin1(str_buf, str_len);
	}
	
	auto ba = s.toLocal8Bit();
	const char *field_name = nullptr;
	
	if (StartsWith4(frame_start, "TIT2")) {
		field_name = "Title/TIT2";
		song_name_ = s;
	} else if (StartsWith4(frame_start, "TPE1")) {
		field_name = "Artist/TPE1";
		artist_ = s;
	} else if (StartsWith4(frame_start, "TCON")) {
		field_name = "Genre/TCON";
		static const QRegularExpression regex =
			QRegularExpression("[ \\-\\/\\+\\']");
		QString genre = s.toLower().replace(regex, "").replace('&', 'n');
		const QChar last_char = genre.at(genre.size() - 1);
		
		{ // workaround:
			if (last_char.unicode() == 0x00)
				genre = genre.left(genre.size() - 1);
		}
		
		audio::GenresFromString(genre, genres_);
		
		if (genres_.isEmpty()) {
			auto genre_ba = genre.toLocal8Bit();
			mtl_warn("At file: \"%s\"\nFailed to decode genre(s): \"%s\"",
				full_path, genre_ba.data());
		}
	} else if (StartsWith4(frame_start, "TYER")) {
		field_name = "Year/TYER";
	} else if (StartsWith4(frame_start, "TALB")) {
		field_name = "Album/TALB";
		album_ = s;
	} else {
		field_name = "(Unprocessed field)";
	}
	
	//mtl_info("%s: \"%s\"", field_name, ba.data());
	
	return size + 10;
}

}
