#pragma once

#include <gst/gst.h>

#include "types.hxx"

namespace quince::audio {

void
PrintBitsUchar(const char *comment, const uchar c);

//Intel processors are little-endian;
//search Google or see: http://en.wikipedia.org/wiki/Endian
i32
reverse(i32 i);

uchar
reverse_uchar(uchar b);

//In short, data in ID3v2 tags are stored as
//"syncsafe integers". This is so the tag info
//isn't mistaken for audio data, and attempted to
//be "played". For more info, have fun Googling it.
i32
syncsafe(i32 i);

const char*
StateToString(const GstState state);

}
