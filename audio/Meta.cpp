#include "Meta.hpp"

#include "../audio.hh"

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
Meta::InterpretTagV2Frame(const char *buf, const i32 max_size)
{
	i32 size;
	memcpy(&size, buf + 4, sizeof size);
	size = quince::audio::syncsafe(size);
	
	if (size <= 0)
		return -1; // no more frames
	
	QString s = QString::fromLocal8Bit(buf + 11, size - 1);
	auto ba = s.toLocal8Bit();
	
	if (StartsWith4(buf, "TIT2")) {
		mtl_info("TIT2: \"%s\"", ba.data());
		song_name_ = s;
	} else if (StartsWith4(buf, "TPE1")) {
		mtl_info("Artist: \"%s\"", ba.data());
		artist_ = s;
	} else if (StartsWith4(buf, "TCON")) {
		mtl_info("Genre: \"%s\"", ba.data());
		genre_ = audio::GenreFromString(s);
	} else if (StartsWith4(buf, "TYER")) {
		mtl_info("Year: \"%s\"", ba.data());
	} else if (StartsWith4(buf, "TALB")) {
		mtl_info("Album: \"%s\"", ba.data());
		album_ = s;
	}
	
	return size + 10;
}

}
