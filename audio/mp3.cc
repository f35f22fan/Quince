#include "mp3.hh"

#include "../audio.hh"
#include "../err.hpp"

namespace quince::audio::mp3 {

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
ReadFileDuration(const char *full_path, i64 &duration_ns)
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
	
	//mtl_info("bitrate %d/%d", bitrate, bitrate / 8);
	
	const i64 to_ns = 1000000000L;
	duration_ns = i64(data_end - data_begin) * to_ns / i64(bitrate / 8);
	
	return true;
}

}
