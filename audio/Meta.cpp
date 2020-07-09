#include "Meta.hpp"

#include "../audio.hh"

#include <QDate>
#include <QRegularExpression>

namespace quince::audio {

bool
StartsWith4(const char *buf, const char *str)
{
	return buf[0] == str[0] && buf[1] == str[1] &&
		buf[2] == str[2] && buf[3] == str[3];
}

i32
Meta::InterpretTagV2Frame(const char *frame_start, const char *full_path)
{
	u32 size;
	memcpy(&size, frame_start + 4, 4);
	size = quince::audio::syncsafe_nobit_discard(size);
	
	if (size <= 0) {
		//mtl_info("No more frames");
		return -1; // no more frames
	}
	
	const auto str_buf = frame_start + 11;
	auto str_len = size - 1;
	
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
	
	s = s.trimmed();
	
	const char *field_name = nullptr;
	
	if (StartsWith4(frame_start, "TIT2")) {
		field_name = "Title/TIT2";
		song_name_ = s;
	} else if (StartsWith4(frame_start, "TPE1")) {
		field_name = "Artist/TPE1";
		artist_ = s;
	} else if (StartsWith4(frame_start, "TCON")) {
		field_name = "Genre/TCON";
		const QChar last_char = s.at(s.size() - 1);
		
		{ // workaround:
			if (last_char.unicode() == 0x00) {
				s = s.left(s.size() - 1);
			}
		}
		audio::GenresFromString(s.midRef(0), genres_);
		if (genres_.isEmpty()) {
			auto genre_ba = s.toLocal8Bit();
			mtl_warn("At file: \"%s\"\nFailed to decode genre(s): \"%s\"",
				full_path, genre_ba.data());
		}
	} else if (StartsWith4(frame_start, "TYER")) {
		field_name = "Year/TYER";
	} else if (StartsWith4(frame_start, "TALB")) {
		field_name = "Album/TALB";
		album_ = s;
	} else if (StartsWith4(frame_start, "APIC")) {
		field_name = "Attached picture/APIC";
	} else {
		field_name = "(Unprocessed field)";
	}
	
	 //auto ba = s.toLocal8Bit();
	 //mtl_info("%s: \"%s\"", field_name, ba.data());
	
	return size + 10;
}


void
Meta::InterpretOpusInfo(OggOpusFile *opus_file)
{
	int current_link = op_current_link(opus_file);
	const OpusTags *opus_tags = op_tags(opus_file, current_link);
	CHECK_PTR_VOID(opus_tags);
	
	char **comments = opus_tags->user_comments;
	const u32 count = opus_tags->comments; // number of comment streams
	i32 *lengths = opus_tags->comment_lengths;
	
	const auto Genre = QLatin1String("genre");
	const auto Artist = QLatin1String("artist");
	const auto Album = QLatin1String("album");
	const auto Title = QLatin1String("title");
	const auto Date = QLatin1String("date");
	
	for (i32 i = 0; i < count; i++)
	{
		char *comment = comments[i];
		u32 len = lengths[i];
		QString s = QString::fromUtf8(comment, len);
		int index = s.indexOf('=');
		
		if (index < 1 || index >= s.size() - 2)
			continue;
		
		QString key = s.leftRef(index).toString().toLower();
		QStringRef value = s.midRef(index + 1);
		
		if (key == Genre) {
			audio::GenresFromString(value, genres_);
		} else if (key == Artist) {
			artist_ = value.toString();
		} else if (key == Album) {
			album_ = value.toString();
		} else if (key == Title) {
			song_name_ = value.toString();
		} else if (key == Date) {
			bool ok;
			int year = value.toInt(&ok);
			
			if (ok) {
				year_ = year;
			} else {
				auto ba = value.toLocal8Bit();
				mtl_trace("Invalid year: \"%s\"", ba.data());
			}
		}
	}
	
}

}
