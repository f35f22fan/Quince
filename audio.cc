#include "audio.hh"

#include "audio/Meta.hpp"
#include "err.hpp"

#include <cstdlib>
#include <opusfile.h>

namespace quince::audio {

void
PrintBitsUchar(const char *comment, const uchar c)
{
	printf("%s%d%d%d%d%d%d%d%d\n", comment, 
		c & 0x80 ? 1:0, c & 0x40 ? 1:0, c & 0x20 ? 1:0, c & 0x10 ? 1:0,
		c & 0x08 ? 1:0, c & 0x04 ? 1:0, c & 0x02 ? 1:0, c & 0x01 ? 1:0);
}

i32
ReadID3V1Size(std::ifstream& infile)
{
	std::streampos saved_pos = infile.tellg(); 
	
	//get to 128 bytes from file end
	infile.seekg(0, std::ios::end);
	std::streampos length = infile.tellg() - (std::streampos)128;
	infile.seekg(length);
	
	i32 size = -1;
	char buffer[3] = {0};
	infile.read(buffer, 3);
	
	if( buffer[0] == 'T' && buffer[1] == 'A' && buffer[2] == 'G' )
		size = 128; //found tag data
	
	infile.seekg(saved_pos);
	
	return size;
}

i32
ReadID3V2Size(std::ifstream& infile)
{
	std::streampos saved_pos = infile.tellg(); 
	infile.seekg(0, std::ios::beg);
	
	char buffer[6] = {0};
	infile.read(buffer, 6);
	
	if(buffer[0] != 'I' || buffer[1] != 'D' || buffer[2] != '3')
	{
		// No tag data
		infile.seekg(saved_pos);
		return -1;
	}
	
	i32 size = 0;
	infile.read(reinterpret_cast<char*>(&size), sizeof(size));
	size = syncsafe(size);
	infile.seekg(saved_pos);
	
	// 10 => 10 bytes of ID3v2 header
	return size + 10;
}

bool
ReadFileMeta(const char *full_path, Meta &meta)
{
	if (meta.is_codec_mp3())
		ReadFileDurationMp3(full_path, meta);
	else if (meta.is_codec_flac())
		ReadFileDurationFlac(full_path, meta);
	else if (meta.is_codec_ogg_opus())
		ReadFileDurationOggOpus(full_path, meta);
	else {
		mtl_warn();
		return false;
	}
	
	return true;
}

bool
ReadFileDurationMp3(const char *full_path, Meta &meta)
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
	std::streampos data_begin = 0;
	
	const i32 v1 = ReadID3V1Size(infile);
	const i32 v2 = ReadID3V2Size(infile);
	
	if (v1 == -1 || v2 == -1) {
		mtl_warn("v1 or v2 equals -1");
		infile.close();
		return false;
	}
	
	data_end -= v1;
	data_begin += v2;
	
	infile.seekg(data_begin, std::ios::beg);
	
	//determine bitrate based on header for first frame of audio data
	i32 header_bytes = 0;
	infile.read(reinterpret_cast<char*>(&header_bytes), sizeof(header_bytes));
	infile.close();
	
	header_bytes = reverse(header_bytes);
	const i32 index = i32((header_bytes >> 12) & 0xF);
	
	if (index < 0 | index > BitrateCount)
		return false;
	
	i32 bitrate = bitrates[index];
	meta.bitrate(bitrate);
	
	const i64 to_ns = 1000000000L;
	i64 n = i64(data_end - data_begin) * to_ns / i64(bitrate / 8);
	meta.duration(n);
	
	return true;
}

bool
ReadFileDurationFlac(const char *full_path, Meta &meta)
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
	const i32 offset_bytes = off_bits / 8;
	
	u32 sample_rate = 0;
	
	for (int i = 0; i < 3; i++)
	{
		sample_rate <<= 8;
		const int index = offset_bytes + i;
		uchar c = meta_block[index];
		sample_rate |= u32(c);
	}
	
	sample_rate >>= 4;
	//mtl_info("Sample rate: %d", sample_rate);
	
	uchar num_channels = 0;
	const i32 num_channels_byte_offset = offset_bytes + 2;
	num_channels = meta_block[num_channels_byte_offset];
	num_channels &= 0x7;
	//mtl_info("Num channels: %u", num_channels);
	
	const i32 samples_off_bits = off_bits + 20 + 8;
	const int samples_off_bytes = samples_off_bits / 8;
	const i32 shift = samples_off_bits % 8;
	
	i64 total_samples = 0;
	
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
	//mtl_info("total_samples: %lu", total_samples);

	const i64 to_ns = 1000000000L;
	i64 n = total_samples * to_ns / i64(sample_rate);
	meta.duration(n);
	
	return true;
}

bool
ReadFileDurationOggOpus(const char *full_path, Meta &meta)
{
	
	int error;
	OggOpusFile *opus_file = op_open_file(full_path, &error);
	
	if (opus_file == NULL) {
		mtl_trace();
		return false;
	}
	
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
	
	i32 sample_rate = opus_head->input_sample_rate;
	//mtl_info("Sample rate: %d", sample_rate);
	
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

i32
syncsafe(i32 i)
{
	i32 ret = 0;
	ret |= ((i & 0x7F000000) >> 24);
	ret |= ((i & 0x007F0000) >>  9);
	ret |= ((i & 0x00007F00) <<  6);
	ret |= ((i & 0x0000007F) << 21);
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

}
