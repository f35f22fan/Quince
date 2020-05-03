#pragma once

#include "../types.hxx"

namespace quince::audio::flac {

bool
ReadFileDuration(const char *full_path, i64 &duration_ns);

}
