// g++ test.cpp $(pkg-config --cflags --libs x11) -o test

#include "global_hotkeys.hpp"
#include "../err.hpp"

#include <iostream>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XF86keysym.h>

#ifdef Q_WS_WIN
#define MY_EXPORT __declspec(dllexport)
#else
#define MY_EXPORT
#endif

using namespace std;
static bool SHOULD_CONTINUE = true;

/*
#define XF86XK_Standby	0x1008FF10 // System into standby mode
#define XF86XK_AudioLowerVolume 0x1008FF11 // Volume control down
#define XF86XK_AudioMute	0x1008FF12 // Mute sound from the system
#define XF86XK_AudioRaiseVolume 0x1008FF13 // Volume control up
#define XF86XK_AudioPlay	0x1008FF14 // Start playing of audio
#define XF86XK_AudioPause	0x1008FF31 // Pause audio playing
#define XF86XK_AudioStop	0x1008FF15 // Stop playing audio
#define XF86XK_AudioPrev	0x1008FF16 // Previous track
#define XF86XK_AudioNext	0x1008FF17 // Next track
#define XF86XK_AudioRecord	0x1008FF1C // Record audio application
#define XF86XK_AudioRewind	0x1008FF3E // "rewind" audio track
define XF86XK_AudioForward	0x1008FF97 // fast-forward audio track
#define XF86XK_AudioRepeat	0x1008FF98 // toggle repeat mode
#define XF86XK_AudioRandomPlay	0x1008FF99 // toggle shuffle mode
*/

static pthread_t my_thread;

void*
quince_global_hotkeys_Register_th(void *p)
{
mtl_trace();
	auto func = *((CallbackFunc*) p);
	Display* dpy = XOpenDisplay(0);
	Window root = DefaultRootWindow(dpy);
	XEvent ev;
	
	unsigned int modifiers = AnyModifier;//ControlMask | ShiftMask;
	auto keycode = AnyKey;//XKeysymToKeycode(dpy, AnyKey); //XK_Y);
	auto key_raise = XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume);
	Window grab_window =  root;
	Bool owner_events = False;
	int pointer_mode = GrabModeAsync;
	int keyboard_mode = GrabModeAsync;
	
	XGrabKey(dpy, keycode, modifiers, grab_window, owner_events, pointer_mode,
	keyboard_mode);
	
	XSelectInput(dpy, root, KeyPressMask);
	
	while(SHOULD_CONTINUE)
	{
		XNextEvent(dpy, &ev);
		bool is_press = false;
		
		switch (ev.type) {
		case KeyPress: {
			mtl_info("Key press");
			is_press = true;
			break;
		}
		default: {
			mtl_info("Other");
			continue;
		}
		}

		if (!is_press)
			continue;
		
mtl_trace();
		const auto code = ev.xkey.keycode;
		mtl_info("code: %u XF86: %d, translated: %d", code,XF86XK_AudioRaiseVolume, key_raise);
		if (code == key_raise) {
			printf("Raise volume\n");
			func(QuinceGlobalHotkeysAction::RaiseVolume);
		} else if (code == XF86XK_AudioLowerVolume) {
			printf("Lower volume\n");
			func(QuinceGlobalHotkeysAction::LowerVolume);
		}
	}
	
	XUngrabKey(dpy, keycode, modifiers, grab_window);
	XCloseDisplay(dpy);
	
	return NULL;
}

extern "C" MY_EXPORT void
quince_global_hotkeys_Register(CallbackFunc callback_func)
{
	int status = pthread_create(&my_thread, NULL,
		&quince_global_hotkeys_Register_th, &callback_func);
	
	if (status != 0) {
		mtl_info("Pthread creation failed: %s", strerror(status));
	} else {
		//pthread_join(my_thread, NULL);
	}
}

void quince_global_hotkeys_Shutdown()
{
	SHOULD_CONTINUE = false;
	pthread_detach(my_thread);
}
