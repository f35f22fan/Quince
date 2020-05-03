#pragma once

#include "../types.hxx"

namespace quince::audio::ogg {

bool
ReadOpusFileDuration(const char *full_path, i64 &duration_ns);

}
