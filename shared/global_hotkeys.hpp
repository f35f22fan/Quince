#pragma once

#include "../quince.hh"

enum class QuinceGlobalHotkeysAction : u8 {
	None = 0,
	PlayPause,
	LowerVolume,
	RaiseVolume,
	Stop
};

typedef void (*CallbackFunc) (QuinceGlobalHotkeysAction);
typedef void (*RegisterFunc)(CallbackFunc cf);

void quince_global_hotkeys_Shutdown();

