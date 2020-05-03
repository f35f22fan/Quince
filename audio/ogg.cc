#include "ogg.hh"

#include "../audio.hh"
#include "../err.hpp"

#include <iostream>
#include <opusfile.h>

namespace quince::audio::ogg {

bool
ReadOpusFileDuration(const char *full_path, i64 &duration_ns)
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
	
	mtl_info("bitrate is: %d", bitrate);
	
	const OpusHead *opus_head = op_head(opus_file, -1);
	
	if (opus_head == NULL) {
		op_free(opus_file);
		mtl_trace();
		return false;
	}
	
	i32 sample_rate = opus_head->input_sample_rate;
	mtl_info("Sample rate: %d", sample_rate);
	
	const i64 pcm = op_pcm_total(opus_file, -1);
	duration_ns = (pcm / 48000L) * 1000'000'000L;
	
	op_free(opus_file);
	
	return true;
}

}
