#pragma once

#include <gst/gst.h>
#include <iostream>
#include <fstream>

#include "audio.hxx"
#include "audio/decl.hxx"
#include "types.hxx"

namespace quince::audio {

const char*
GenreToString(const Genre g);

void
PrintBitsUchar(const char *comment, const uchar c);

bool
ReadFileDurationFlac(const char *full_path, Meta &meta);

bool
ReadFileDurationMp3(const char *full_path, Meta &meta);

bool
ReadFileDurationOggOpus(const char *full_path, Meta &meta);

bool
ReadFileMeta(const char *full_path, Meta &meta);

//How much room does ID3 version 1 tag info
//take up at the end of this file (if any)?
i32
ReadID3V1Size(std::ifstream& infile, Meta *meta);

//how much room does ID3 version 2 tag info
//take up at the beginning of this file (if any)
i32
ReadID3V2Size(std::ifstream& infile);

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

void
timespec_diff(struct timespec *start, struct timespec *stop,
	struct timespec *result);

}
