//
// these are the key numbers that should be passed to Key_Event
//
typedef enum
{
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE	= 32,

	// normal keys should be passed as lowercased ascii

	K_BACKSPACE = 127,

	K_CAPSLOCK,
	K_SCROLLLOCK,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_WIN,
	//	K_LWIN,
	//	K_RWIN,
	K_MENU,

#if defined ( __APPLE__ ) || defined ( MACOSX )
	K_F13,
	K_F14,
	K_F15,
	K_COMMAND,
#endif /* __APPLE__ || MACOSX */

	//
	// Keypad stuff..
	//

	K_NUMLOCK,
	KP_SLASH,
	KP_STAR,

	KP_HOME,
	KP_UPARROW,
	KP_PGUP,
	KP_MINUS,

	KP_LEFTARROW,
	KP_5,
	KP_RIGHTARROW,
	KP_PLUS,

	KP_END,
	KP_DOWNARROW,
	KP_PGDN,

	KP_INS,
	KP_DEL,
	KP_ENTER,

#if defined ( __APPLE__ ) || ( MACOSX )
	KP_MULT,
	KP_EQUAL,
#endif /* __APPLE__ || MACOSX */

	//
	// mouse buttons generate virtual keys
	//
#if !defined ( __APPLE__ ) && !defined ( MACOSX )
	K_MOUSE1 = 200,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
#endif /* !__APPLE__ && !MACOSX */

	//
	// joystick buttons
	//
	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,

	//
	// aux keys are for multi-buttoned joysticks to generate so they can use
	// the normal binding process
	//
	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32,

	K_MWHEELUP,
	K_MWHEELDOWN

#if defined ( __APPLE__ ) || defined ( MACOSX )
	,
	K_MOUSE1 = 241,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5
#endif /* __APPLE__ || MACOSX */
} keyNum_t;

