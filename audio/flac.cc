#include "flac.hh"

#include "../audio.hh"
#include "../err.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>

namespace quince::audio::flac {

void
PrintBitsUchar(const char *comment, const uchar c)
{
	printf("%s%d%d%d%d%d%d%d%d\n", comment, 
		c & 0x80 ? 1:0, c & 0x40 ? 1:0, c & 0x20 ? 1:0, c & 0x10 ? 1:0,
		c & 0x08 ? 1:0, c & 0x04 ? 1:0, c & 0x02 ? 1:0, c & 0x01 ? 1:0);
}

bool
ReadFileDuration(const char *full_path, i32 &duration)
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
	
	u64 total_samples = 0;
	
	for (int i = 0; i < 5; i++)
	{
		total_samples <<= 8;
		const int index = samples_off_bytes + i;
		uchar c = meta_block[index];
		total_samples |= u64(c);
	}
	
	total_samples <<= shift;
	total_samples >>= shift;
	total_samples &= 0x0000000FFFFFFFFF;
	//mtl_info("total_samples: %lu", total_samples);

	duration = total_samples / sample_rate;
	
	return true;
}

}
