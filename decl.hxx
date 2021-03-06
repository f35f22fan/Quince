#pragma once

#include "types.hxx"

namespace quince {
class App;
class ByteArray;
class Duration;
class GstPlayer;
class Song;

enum class PlaylistActivationOption: u8 {
	None,
	RestoreStreamPosition,
};

enum class Desktop : u8 {
	None,
	KDE,
	Gnome,
	Other
};

}
