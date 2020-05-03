#pragma once

#include "types.hxx"

namespace quince::audio {

//Bitrates, assuming MPEG 1 Audio Layer 3
const i32 BitrateCount = 16;
const i32 bitrates[BitrateCount] = {
0, 32000, 40000, 48000, 56000, 64000, 80000, 96000,
112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 };

enum class Codec : u8 {
	Unknown,
	Mp3,
	OggOpus,
	Flac,
};

}
