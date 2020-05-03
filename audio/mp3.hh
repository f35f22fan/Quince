#pragma once

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "../types.hxx"

namespace quince::audio::mp3 {

//Bitrates, assuming MPEG 1 Audio Layer 3
const i32 BitrateCount = 16;
const i32 bitrates[BitrateCount] = {
0, 32000, 40000, 48000, 56000, 64000, 80000, 96000,
112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 };

//How much room does ID3 version 1 tag info
//take up at the end of this file (if any)?
i32
ReadID3V1Size(std::ifstream& infile);

//how much room does ID3 version 2 tag info
//take up at the beginning of this file (if any)
i32
ReadID3V2Size(std::ifstream& infile);

bool
ReadFileDuration(const char *full_path, i64 &duration_ns);

}
