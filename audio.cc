#include "audio.hh"

#include "err.hpp"

namespace quince::audio {

void
PrintBitsUchar(const char *comment, const uchar c)
{
	printf("%s%d%d%d%d%d%d%d%d\n", comment, 
		c & 0x80 ? 1:0, c & 0x40 ? 1:0, c & 0x20 ? 1:0, c & 0x10 ? 1:0,
		c & 0x08 ? 1:0, c & 0x04 ? 1:0, c & 0x02 ? 1:0, c & 0x01 ? 1:0);
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
