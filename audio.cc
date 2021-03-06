#include "audio.hh"

#include "audio/Meta.hpp"
#include "err.hpp"

#include <cstdlib>
#include <opusfile.h>
#include <QFileInfo>
#include <QRegularExpression>

namespace quince::audio {

bool
GenresFromString(const QStringRef &genre_name, QVector<Genre> &vec)
{
//	auto ba1 = genre_name.toLocal8Bit();
//	mtl_info("Genre name: \"%s\"", ba1.data());
	static const QRegularExpression regex = QRegularExpression("[ \\-\\/\\+\\']");
	QString genre_string = genre_name.toString();
	const QString s = genre_string.toLower().replace(regex, "").replace('&', 'n');
	
	if (s.startsWith('('))
	{
		int end = s.indexOf(')');
		
		if (end == -1)
			return false;
		
		QStringRef ref = s.midRef(1, end - 1);
		auto list = ref.split(',');
		
		for (auto &next: list) {
			bool ok;
			i16 index = next.toShort(&ok);
			
			if (!ok || index > i16(Genre::Count))
				continue;
			
			vec.append(Genre(index));
		}
		
		return true;
	}
	
	GenresFromStringSubroutine(s, ',', vec);
//	mtl_info("subroutine #1: %d", vec.size());

	if (vec.isEmpty())
	{
		static const QRegularExpression regex2 = QRegularExpression("[ \\-\\+\\']");
		QString s2 = genre_string.toLower().replace(regex2, "").replace('&', 'n');
//		auto ba = s2.toLocal8Bit();
//		mtl_info("Genre --- \"%s\"", ba.data());
		GenresFromStringSubroutine(s2, '/', vec);
//		mtl_info("subroutine #2: %d", vec.size());
	}
	
	return true;
}

void
GenresFromStringSubroutine(const QString &s, const QChar delimiter, QVector<Genre> &vec)
{
	const int total_genres_count = int(Genre::Count);
	QVector<QStringRef> genres = s.splitRef(delimiter);
	
	for (const auto &str_genre: genres) {
//		auto ba2 = str_genre.toLocal8Bit();
//		mtl_info("Genre: %s", ba2.data());
		
		bool found = false;
		
		for (int i = 0; i <= total_genres_count; i++) {
			QString registered_genre = audio::GenresForInternalUsage[i];

			if (registered_genre == str_genre) {
				vec.append(Genre(i16(i)));
				found = true;
				break;
			}
		}
		
		 // workarounds:
		if (!found) {
			if (str_genre == "попрок") {
				vec.append(Genre::PopRock);
			} else if (str_genre == "синтипоп") {
				vec.append(Genre::SynthPop);
			}
		}
	}
}

const char*
GenreToString(const Genre g)
{
	if (g == Genre::None)
		return "";
	
	return GenreStringArray[int(g)];
}

QString
GenresToString(const QVector<Genre> &v)
{
	QString s;
	const auto count = v.size();
	
	for (int i = 0; i < count; i++) {
		s.append(GenreToString(v[i]));
		
		if (i < count - 1)
			s.append(',');
	}
	
	return s;
}

MpegVersion
GetMpegVersion(const int header)
{
	const i32 n = (header >> 19) & 0x3;
	
	switch (n)
	{
	case 0: return MpegVersion::_2_5;
	case 1: return MpegVersion::Reserved;
	case 2: return MpegVersion::_2;
	case 3: return MpegVersion::_1;
	default: return MpegVersion::None;
	}
}

MpegLayer
GetMpegLayer(const int header)
{
	const int n = (header >> 17) & 0x3;
	
	switch(n)
	{
	case 0: return MpegLayer::Reserved;
	case 1: return MpegLayer::_3;
	case 2: return MpegLayer::_2;
	case 3: return MpegLayer::_1;
	default: return MpegLayer::None;
	}
}

i32
GetMpegSampleRate(const int header, const MpegVersion v)
{
	const int n = (header >> 10) & 0x3;
	int div = 1;
	
	if (v == MpegVersion::_2)
		div = 2;
	else if (v == MpegVersion::_2_5)
		div = 4;
	else if (v == MpegVersion::Reserved)
		return -1;
	
	switch (n)
	{
	case 0: return 44100 / div;
	case 1: return 48000 / div;
	case 2: return 32000 / div;
	default: return -1;
	}
}

i8
GetMpegChannels(const int header)
{
	const int n = (header >> 6) & 0x3;
	
	switch (n)
	{
	case 0: return 2; // Stereo
	case 1: return 2; // Joint Stereo
	case 2: return 2; // Dual Channel Stereo
	case 3: return 1; // Mono
	}
	
	return -1;
}

void
PrintBitsUchar(const char *comment, const uchar c)
{
	printf("%s%d%d%d%d%d%d%d%d\n", comment, 
		c & 0x80 ? 1:0, c & 0x40 ? 1:0, c & 0x20 ? 1:0, c & 0x10 ? 1:0,
		c & 0x08 ? 1:0, c & 0x04 ? 1:0, c & 0x02 ? 1:0, c & 0x01 ? 1:0);
}

i32
ReadID3V1Size(std::ifstream& infile, Meta *meta)
{
	std::streampos saved_pos = infile.tellg(); 
	
	//get to 128 bytes from file end
	infile.seekg(0, std::ios::end);
	std::streampos length = infile.tellg() - std::streampos(128);
	infile.seekg(length);
	
	i32 size = 0;
	char buf[128] = {0};
	infile.read(buf, 128);
	
	if(buf[0] == 'T' && buf[1] == 'A' && buf[2] == 'G') {
		size = 128; //found tag data
	}
	
	infile.seekg(saved_pos);
	
	return size;
}

i32
ReadID3V2Size(std::ifstream& infile, Meta *meta, const char *full_path)
{
	std::streampos saved_pos = infile.tellg(); 
	infile.seekg(0, std::ios::beg);
	
	char buf[6] = {0};
	infile.read(buf, 6);
	
	if(buf[0] != 'I' || buf[1] != 'D' || buf[2] != '3')
	{
		// No tag data
		infile.seekg(saved_pos);
		return -1;
	}
	
	i32 size = 0;
	infile.read(reinterpret_cast<char*>(&size), 4);
	size = syncsafe(size);
	i32 so_far = 0;
	
//	mtl_info("Reading file: \"%s\", tag size: %u", full_path, size);
	
	if (meta != nullptr && size > 0) {
		char tagv2[size];
		memset(tagv2, 0, size);
		
		infile.read(tagv2, sizeof tagv2);
		
		while (so_far < size) {
			const i64 remaining = size - so_far;
			i32 sz = meta->InterpretTagV2Frame(tagv2 + so_far, full_path, remaining);
			
			if (sz == -1)
				break; // no more frames
			
			so_far += sz;
		}
	}
	
	infile.seekg(saved_pos);
	
	// + 10 bytes of ID3v2 header
	return size + 10;
}

bool
ReadFileMeta(const char *full_path, Meta &meta)
{
	if (meta.is_codec_mp3()) {
		ReadMp3FileMeta(full_path, meta);
	} else if (meta.is_codec_flac())
		ReadFlacFileMeta(full_path, meta);
	else if (meta.is_codec_ogg_opus())
		ReadOggOpusFileMeta(full_path, meta);
	else {
		mtl_trace();
		return false;
	}
	
	return true;
}

bool
ReadMp3FileMeta(const char *full_path, Meta &meta)
{
	std::ifstream infile(full_path, std::ios::binary);
	
	if(!infile.is_open())
	{
		mtl_warn("Error opening file");
		return false;
	}
	
	// Get start & end of primary frame data (don't confuse with ID3 tags)
	infile.seekg(0, std::ios::end);
	std::streampos data_end = infile.tellg();
	infile.seekg(0, std::ios::beg);
	
	const i32 v1_size = ReadID3V1Size(infile, &meta);
	const i32 v2_size = ReadID3V2Size(infile, &meta, full_path);
	
	if (v2_size == -1) {
		mtl_warn("ID3v 2 not present");
		infile.close();
		return false;
	}
	
	data_end -= v1_size;
	std::streampos data_begin = v2_size;
	infile.seekg(data_begin, std::ios::beg);
	
	i32 header = 0;
	infile.read(reinterpret_cast<char*>(&header), sizeof header);
	infile.close();
	header = reverse(header);
	const MpegVersion mpeg_version = GetMpegVersion(header);
	
	if (mpeg_version != MpegVersion::_1 ||
		GetMpegLayer(header) != MpegLayer::_3)
	{
		mtl_trace("Not MPEG_1_3: \"%s\"", full_path);
		return false;
	}
	
	i32 sample_rate = GetMpegSampleRate(header, mpeg_version);
	meta.sample_rate(sample_rate);
	meta.channels(GetMpegChannels(header));
	
	// determine bitrate based on header for first frame of audio data
	const i32 index = i32((header >> 12) & 0xF);
	
	if (index < 0 | index > Mp3BitrateArrayLen)
		return false;
	
	i32 bitrate = Mp3Bitrates[index];
	meta.bitrate(bitrate);
	
	const i64 to_ns = 1000000000L;
	i64 bitrate_in_bytes = bitrate / 8;
	i64 n = i64(data_end - data_begin) * to_ns / bitrate_in_bytes;
	meta.duration(n);
	
	return true;
}

bool
ReadFlacFileMeta(const char *full_path, Meta &meta)
{
	std::ifstream infile(full_path, std::ios::binary);
	
	if(!infile.is_open())
	{
		mtl_warn("Error opening file");
		return false;
	}
	
	const i32 ByteCount = 4;
	char buf[ByteCount] = {0};
	infile.read(buf, ByteCount);
	
	if (buf[0] != 'f' || buf[1] != 'L' || buf[2] != 'a' || buf[3] != 'C')
	{
		mtl_trace();
		infile.close();
		return false;
	}
	
	i32 meta_block_size = 0;
	infile.read(reinterpret_cast<char*>(&meta_block_size), sizeof meta_block_size);
	meta_block_size = quince::audio::syncsafe(meta_block_size);
	
	uchar meta_block[meta_block_size]; // 272 bits (34 bytes)
	infile.read(reinterpret_cast<char*>(meta_block), meta_block_size);
	infile.close();
	
	const i32 off_bits = 16 + 16 + 24 + 24;
	const i32 sample_rate_offset = off_bits / 8;
	
	u32 sample_rate = 0;
	
	for (int i = 0; i < 3; i++)
	{
		sample_rate <<= 8;
		const int index = sample_rate_offset + i;
		uchar c = meta_block[index];
		sample_rate |= u32(c);
	}
	
	sample_rate >>= 4;
	meta.sample_rate(sample_rate);
	
	const i32 num_channels_offset = sample_rate_offset + 2;
	const u8 bits_4_3_1 = meta_block[num_channels_offset];
	u8 num_channels = ((bits_4_3_1 & 0xE) >> 1) + 1;
	meta.channels(num_channels);
	
	u8 bits_per_sample = (bits_4_3_1 & 0x1) << 4;
	u8 bits_4_x = meta_block[num_channels_offset + 1];
	bits_per_sample |= (bits_4_x >> 4) + 1;
	meta.bits_per_sample(bits_per_sample);
	
	const i32 samples_off_bits = off_bits + 20 + 8;
	const int samples_off_bytes = samples_off_bits / 8;
	const i32 shift = samples_off_bits % 8;
	
	i64 total_samples = 0; // total samples in stream
	
	for (int i = 0; i < 5; i++)
	{
		total_samples <<= 8;
		const int index = samples_off_bytes + i;
		uchar c = meta_block[index];
		total_samples |= i64(c);
	}
	
	total_samples <<= shift;
	total_samples >>= shift;
	total_samples &= 0x0000000FFFFFFFFF;
	
	// Compute bitrate:
	// 44,100 samples per second × 16 bits per sample × 2 channels
	// = 1,411,200 bits per second (or 1,411.2 kbps)
	const i32 bitrate = sample_rate * bits_per_sample * num_channels;
	meta.bitrate(bitrate);
	
	const i64 to_ns = 1000000000L;
	i64 n = total_samples * to_ns / i64(sample_rate);
	meta.duration(n);
	
	return true;
}

bool
ReadOggOpusFileMeta(const char *full_path, Meta &meta)
{
	int error;
	OggOpusFile *opus_file = op_open_file(full_path, &error);
	
	if (opus_file == NULL) {
		mtl_trace();
		return false;
	}
	
	meta.InterpretOpusInfo(opus_file);
	
	i32 bitrate = op_bitrate(opus_file, 0);
	
	if (bitrate < 0) {
		op_free(opus_file);
		mtl_trace();
		return false;
	}
	
	meta.bitrate(bitrate);
	
	const OpusHead *opus_head = op_head(opus_file, -1);
	
	if (opus_head == NULL) {
		op_free(opus_file);
		mtl_trace();
		return false;
	}
	
	meta.sample_rate(opus_head->input_sample_rate);
	meta.channels(opus_head->channel_count);
//	i32 bits_per_sample = bitrate / (meta.sample_rate() * meta.channels());
//	mtl_info("bits_per_sample: %d", bits_per_sample);
	
	const i64 pcm = op_pcm_total(opus_file, -1);
	op_free(opus_file);
	
	i64 n = (pcm / 48000L) * 1000'000'000L;
	meta.duration(n);
	
	
	return true;
}

i32
reverse(i32 i)
{
	i32 ret = 0;
	ret |= ((i & 0x000000FF) << 24);
	ret |= ((i & 0x0000FF00) << 8);
	ret |= ((i & 0x00FF0000) >> 8);
	ret |= ((i & 0xFF000000) >> 24);
	return ret;
}

uchar
reverse_uchar(uchar b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

u32
syncsafe(u32 i)
{
	u32 ret = 0;
	ret |= ((i & 0x7F000000) >> 24);
	ret |= ((i & 0x007F0000) >>  9);
	ret |= ((i & 0x00007F00) <<  6);
	ret |= ((i & 0x0000007F) << 21);
	return ret;
}

u32
syncsafe_nobit_discard(u32 i)
{
	u32 ret = 0;
	ret |= ((i & 0xFF000000) >> 24);
	ret |= ((i & 0x00FF0000) >>  8);
	ret |= ((i & 0x0000FF00) <<  8);
	ret |= ((i & 0x000000FF) << 24);
	return ret;
}

const char*
StateToString(const GstState state)
{
	switch (state) {
	case GST_STATE_NULL: return "GST_STATE_NULL";
	case GST_STATE_PLAYING: return "GST_STATE_PLAYING";
	case GST_STATE_PAUSED: return "GST_STATE_PAUSED";
	case GST_STATE_READY: return "GST_STATE_READY";
	case GST_STATE_VOID_PENDING: return "GST_STATE_VOID_PENDING";
	default: mtl_trace(); return NULL;
	}
}

void
timespec_diff(struct timespec *start, struct timespec *stop,
	struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000L;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}
	
	return;
}

}
