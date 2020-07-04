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
Meta::InterpretTagV2Frame(const char *frame_start, const i32 max_size)
{
	i32 size;
	memcpy(&size, frame_start + 4, 4);
	size = quince::audio::syncsafe(size);
	
	if (size <= 0)
		return -1; // no more frames
	
	const auto str_buf = frame_start + 11;
	const auto str_len = size - 1;
	
	if (str_len > 50000) {
		//mtl_info("Too long (%d), skipping", str_len);
		return -1;
	}
	
	i32 skip = 0;
	
	if (str_len > 2)
	{
		const u8 n = str_buf[0];
		
		if (n == 0xFF || n == 0xFE)
			skip = 2;
	}
	
	const auto str_buf_start = str_buf;// + skip;
	auto str_buf_len = str_len;// - skip;
	QString s;
	
	if (skip == 2) {
		int end = -1;
		bool last_was_zero = false;
		
		for (int i = 0; i < str_buf_len; i++)
		{
			u8 n = str_buf_start[i];
			const bool is_zero = n == 0x00;
			
			if (is_zero) {
				if (last_was_zero) {
					end = i;
					//mtl_info("FOUND 2 ZEROES!");
					break;
				}
			}
			
			last_was_zero = is_zero;
		}
		
		if (end != -1)
			str_buf_len = end / 2;
		
		s = QString::fromUtf16((const ushort*)str_buf_start, str_buf_len);
	} else {
		s = QString::fromLocal8Bit(str_buf_start, str_buf_len);
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
		genre_ = audio::GenreFromString(s);
//		mtl_info("Decoded genre: %s, genre str len: %d",
//			audio::GenreToString(genre_), s.size());
		
//		for (int i = 0; i < ba.size(); i++)
//		{
//			char c = ba.at(i);
//			printf("[%c %X] ", c, u8(c));
//		}
//		printf("\n");
		
	} else if (StartsWith4(frame_start, "TYER")) {
		field_name = "Year/TYER";
	} else if (StartsWith4(frame_start, "TALB")) {
		field_name = "Album/TALB";
		album_ = s;
	} else {
		field_name = "(Unprocessed field)";
	}
	
	mtl_info("%s: \"%s\"", field_name, ba.data());
	
	return size + 10;
}

}
