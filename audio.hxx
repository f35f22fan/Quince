#pragma once

#include "types.hxx"

namespace quince::audio {

enum class Pick : i8 {
	Prev,
	Next
};

enum class PlayMode : i8 {
	NotSet = -1,
	RepeatTrack,
	RepeatPlaylist,
	StopAtTrackEnd,
	StopAtPlaylistEnd,
};

//Bitrates, assuming MPEG 1 Audio Layer 3
const i32 Mp3BitrateArrayLen = 16;
const i32 Mp3Bitrates[Mp3BitrateArrayLen] = {
0, 32000, 40000, 48000, 56000, 64000, 80000, 96000,
112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 };

const i32 Mp3SampleRateArrayLen = 4;
const int Mp3SampleRates[Mp3SampleRateArrayLen] = {
	44100, 48000, 32000, 0
};

enum class MpegVersion : u8
{
	None,
	_2_5,
	Reserved,
	_2,
	_1,
};

enum class MpegLayer: u8 {
	None,
	Reserved,
	_1,
	_2,
	_3,
};

// https://www.mp3-tech.org/programmer/frame_header.html
enum class Mp3ChannelMode {
	Stereo = 0,
	JointStereo = 1,
	DualChannel = 2, // 2 mono channels
	SingleChannel = 3, // Mono
};

enum class Codec : u8 {
	Unknown,
	Mp3,
	OggOpus,
	Flac,
};

enum class Genre : i16 {
	None = -1,
// WARNING: Don't change the genres order!

// The following genres are defined in ID3v1:
	Blues, ClassicRock, Country, Dance, Disco, Funk, Grunge, HipHop,
	Jazz, Metal, NewAge, Oldies, Other, Pop, RNB, Rap, Reggae, Rock,
	Techno, Industrial, Alternative, Ska, DeathMetal, Pranks,
	Soundtrack, EuroTechno, Ambient, TripHop, Vocal, JazzFunk,
	Fusion, Trance, Classical, Instrumental, Acid, House, Game,
	SoundClip, Gospel, Noise, AlternRock, Bass, Soul, Punk, Space,
	Meditative, InstrumentalPop, InstrumentalRock, Ethnic, Gothic,
	Darkwave, TechnoIndustrial, Electronic, PopFolk, Eurodance, Dream,
	SouthernRock, Comedy, Cult, Gangsta, Top40, ChristianRap, PopFunk,
	Jungle, NativeAmerican, Cabaret, NewWave, Psychedelic, Rave,
	Showtunes, Trailer, LoFi, Tribal, AcidPunk, Polka, Retro, Musical,
	RockNRoll, HardRock,
	
// The following genres are Winamp extensions
	Folk, FolkRock, NationalFolk, Swing, FastFusion, Bebob, Latin,
	Revival, Celtic, Bluegrass, Avantgarde, GothicRock, ProgressiveRock,
	PsychedelicRock, SymphonicRock, SlowRock, BigBand, Chorus,
	EasyListening, Acoustic, Humour, Speech, Chanson, Opera,
	ChamberMusic, Sonata, Symphony, BootyBrass, Primus, PornGroove,
	Satire, SlowJam, Club, Tango, Samba, Folklore, Ballad, PowerBallad,
	RhytmicSoul, Freestyle, Duet, PunkRock, DrumSolo, ACapela,
	EuroHouse, DanceHall,

// And even more genres, I forgot where I took these from:
	GoaMusic, DrumNBass, ClubHouse, HardcoreTechno, Terror, Indie,
	BritPop, Negerpunk, PolskPunk, Beat, ChristianGangstaRap,
	HeavyMetal, BlackMetal, Crossover, ContemporaryChristian,
	ChristianRock, Merengue, Salsa, ThrashMetal, Anime, Jpop, Synthpop,
	Abstract, ArtRock, Baroque, Bhangra, BigBeat, Breakbeat, Chillout,
	Downtempo, Dub, EBM, Eclectic, Electro, Electroclash, Emo,
	Experimental, Garage, Global, IDM, Illbient, IndustroGoth,
	JamBand, Krautrock, Leftfield, Lounge, MathRock, NewRomantic,
	NuBreakz, PostPunk, PostRock, Psytrance, Shoegaze, SpaceRock,
	TropRock, WorldMusic, Neoclassical, Audiobook, AudioTheatre,
	NeueDeutscheWelle, Podcast, IndieRock, GFunk, Dubstep, GarageRock,
	Psybient,
	
	Count
};

// http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
// This string array matches exactly the audio::Genre enum.
static const char *GenreStringArray[] =
{
 "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
 "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other",
 "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial",
 "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
 "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
 "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
 "Game", "Sound Clip", "Gospel", "Noise", "Alternative Rock", "Bass",
 "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
 "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
 "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance",
 "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40",
 "Christian Rap", "Pop/Funk", "Jungle", "Native American",
 "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes",
 "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka",
 "Retro", "Musical", "Rock & Roll", "Hard Rock",

 "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob",
 "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
 "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
 "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening",
 "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music",
 "Sonata", "Symphony", "Booty Brass", "Primus", "Porn Groove",
 "Satire", "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad",
 "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock",
 "Drum Solo", "A Capela", "Euro-House", "Dance Hall",

 "Goa Music", "Drum & Bass", "Club-House", "Hardcore Techno",
 "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat",
 "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
 "Contemporary Christian", "Christian Rock", "Merengue", "Salsa",
 "Thrash Metal", "Anime", "Jpop", "Synthpop", "Abstract", "ArtRock",
 "Baroque", "Bhangra", "Big beat", "Breakbeat", "Chillout",
 "Downtempo", "Dub", "EBM", "Eclectic", "Electro", "Electroclash",
 "Emo", "Experimental", "Garage", "Global", "IDM", "Illbient",
 "Industro-Goth", "JamBand", "Krautrock", "Leftfield", "Lounge",
 "Math Rock", "New Romantic", "Nu-Breakz", "Post-Punk", "Post-Rock",
 "Psytrance", "Shoegaze", "Space Rock", "Trop Rock", "World Music",
 "Neoclassical", "Audiobook", "Audio Theatre", "Neue Deutsche Welle",
 "Podcast", "Indie-Rock", "G-Funk", "Dubstep", "Garage Rock",
 "Psybient"
};

}
