
#ifndef __Q_CVAR_H__
#define __Q_CVAR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int cvar_flag_t;

// bit-masked cvar flags
#define CVAR_ARCHIVE		1   // set to cause it to be saved to vars.rc
#define CVAR_USERINFO		2   // added to userinfo  when changed
#define CVAR_SERVERINFO		4   // added to serverinfo when changed
#define CVAR_NOSET			8   // don't allow change from console at all, but can be set from the command line
#define CVAR_LATCH			16  // save changes until map restart
#define CVAR_LATCH_VIDEO    32  // save changes until video restart
#define CVAR_LATCH_SOUND    64  // save changes until video restart
#define CVAR_CHEAT			128 // will be reset to default unless cheats are enabled
#define CVAR_READONLY	    256 // don't allow changing by user, ever
#define	CVAR_DEVELOPER	    512 // edit in internal builds, lock & hide in release builds.

// nothing outside the Cvar_*() functions should access these fields!!!
typedef struct cvar_s
{
	char *name;
	char *string;
	char *dvalue;
	char *latched_string;       // for CVAR_LATCH vars
	cvar_flag_t flags;
	qboolean modified;          // set each time the cvar is changed
	float value;
	int integer;
} cvar_t;

#ifdef __cplusplus
};
#endif

#endif // __Q_CVAR_H__
