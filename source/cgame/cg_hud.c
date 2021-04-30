/*
   Copyright (C) 2007 German Garcia
 */

#include "cg_local.h"

//#pragma warning( disable : 4306 )       // conversion from 'int' to 'void *' of greater size

static cvar_t *cg_viewlockHUD;

//=============================================================================

enum
{      // item model effects
	EFF3D_NONE = 0,
	EFF3D_WAVEPITCH,
	EFF3D_WAVEROLL,
	EFF3D_WAVEYAW,
	EFF3D_WAVEPITCH_STRONG,
	EFF3D_WAVEROLL_STRONG,
	EFF3D_WAVEYAW_STRONG,
	EFF3D_ROTATEPITCH,
	EFF3D_ROTATEYAW,
	EFF3D_ROTATEROLL
};

static int hud_window_x;
static int hud_window_y;
static int hud_window_width;
static int hud_window_height;
static int hud_cursor_x;
static int hud_cursor_y;
static int hud_cursor_width;
static int hud_cursor_height;
static int hud_cursor_align;
static vec4_t hud_cursor_color;
static vec3_t hud_cursor_angles;
static int hud_cursor_3Dobject_effect;
static struct mufont_s *hud_cursor_font;

//=============================================================================

#define HUD_VIRTUALSCREEN_WIDTH	    800
#define HUD_VIRTUALSCREEN_HEIGHT    600

#define HUDREALWIDTH( width ) ( hud_window_width * ( width / HUD_VIRTUALSCREEN_WIDTH ) )
#define HUDREALHEIGHT( height ) ( hud_window_height * ( height / HUD_VIRTUALSCREEN_HEIGHT ) )
#define HUDREALX( x ) ( hud_window_x + ( hud_window_width * ( x / HUD_VIRTUALSCREEN_WIDTH ) ) )
#define HUDREALY( y ) ( hud_window_y + ( hud_window_height * ( y / HUD_VIRTUALSCREEN_HEIGHT ) ) )

//=============================================================================
// COMMANDS
//=============================================================================

static int CG_LFuncCursor( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	hud_cursor_x = HUDREALX( GS_GetNumericArg( &argumentnode ) );
	hud_cursor_y = HUDREALY( GS_GetNumericArg( &argumentnode ) );
	return qtrue;
}

static int CG_LFuncMoveCursor( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	hud_cursor_x += HUDREALWIDTH( GS_GetNumericArg( &argumentnode ) );
	hud_cursor_y += HUDREALHEIGHT( GS_GetNumericArg( &argumentnode ) );
	return qtrue;
}

static int CG_LFuncSize( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	hud_cursor_width = HUDREALWIDTH( GS_GetNumericArg( &argumentnode ) );
	hud_cursor_height = HUDREALHEIGHT( GS_GetNumericArg( &argumentnode ) );
	return qtrue;
}

static int CG_LFuncColor( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		hud_cursor_color[i] = GS_GetNumericArg( &argumentnode );
		clamp( hud_cursor_color[i], 0, 1 );
	}
	return qtrue;
}

static int CG_LFuncAngles( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	int i;
	for( i = 0; i < 3; i++ )
	{
		hud_cursor_angles[i] = GS_GetNumericArg( &argumentnode );
	}
	return qtrue;
}

static int CG_LFuncSet3DObjectEffect( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	hud_cursor_3Dobject_effect = (int)GS_GetNumericArg( &argumentnode );
	return qtrue;
}

static int CG_LFuncAlign( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	int v, h;

	h = (int)GS_GetNumericArg( &argumentnode );
	v = (int)GS_GetNumericArg( &argumentnode );
	if( h < 1 ) h = 1;
	if( v < 1 ) v = 1;
	hud_cursor_align = ( h-1 )+( 3*( v-1 ) );
	return qtrue;
}

static int CG_LFuncFont( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	struct mufont_s *font;
	char *fontname = GS_GetStringArg( &argumentnode );

	if( !Q_stricmp( fontname, "con_fontsystemsmall" ) )
	{
		font = cgm.fontSystemSmall;
	}
	else if( !Q_stricmp( fontname, "con_fontsystemmedium" ) )
	{
		font = cgm.fontSystemMedium;
	}
	else if( !Q_stricmp( fontname, "con_fontsystembig" ) )
	{
		font = cgm.fontSystemBig;
	}
	else
	{
		font = trap_SCR_RegisterFont( fontname );
	}

	if( font )
	{
		hud_cursor_font = font;
		return qtrue;
	}

	return qfalse;
}

static int CG_LFuncIf( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	return (int)GS_GetNumericArg( &argumentnode );
}

static int CG_LFuncDrawString( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	char *string = GS_GetStringArg( &argumentnode );

	if( !string || !string[0] )
		return qfalse;
	trap_SCR_DrawString( hud_cursor_x, hud_cursor_y, hud_cursor_align, string, hud_cursor_font, hud_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawStringNumeric( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	int value = (int)GS_GetNumericArg( &argumentnode );

	trap_SCR_DrawString( hud_cursor_x, hud_cursor_y, hud_cursor_align, va( "%i", value ), hud_cursor_font, hud_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawItem( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	int value = (int)GS_GetNumericArg( &argumentnode );
	vec3_t angles;
	float delta, range = 5, range_strong = 10, range_mini = 3;

	VectorCopy( hud_cursor_angles, angles );

	delta = ( (float)( cg.time & 0x0FFF ) / (float)0x0FFF ); // 4095 milliseconds loop

	switch( hud_cursor_3Dobject_effect )
	{
	case EFF3D_WAVEPITCH:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[PITCH] += ( delta * range );
		break;
	case EFF3D_WAVEROLL:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[ROLL] += ( delta * range );
		break;
	case EFF3D_WAVEYAW:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[YAW] += ( delta * range );
		break;
	case EFF3D_WAVEPITCH_STRONG:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[PITCH] += ( delta * range_strong );
		angles[ROLL] += ( delta * range_mini );
		break;
	case EFF3D_WAVEROLL_STRONG:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[ROLL] += ( delta * range_strong );
		angles[PITCH] += ( delta * range_mini );
		break;
	case EFF3D_WAVEYAW_STRONG:
		delta = sin( DEG2RAD( delta*360 ) );
		angles[YAW] += ( delta * range_strong );
		angles[ROLL] += ( delta * range_mini );
		break;
	case EFF3D_ROTATEPITCH:
		delta *= 360;
		angles[PITCH] += delta;
		break;
	case EFF3D_ROTATEYAW:
		delta *= 360;
		angles[YAW] += delta;
		break;
	case EFF3D_ROTATEROLL:
		delta *= 360;
		angles[ROLL] += delta;
		break;
	default:
		break;
	}

	CG_DrawItem( hud_cursor_x, hud_cursor_y, hud_cursor_align, hud_cursor_width, hud_cursor_height, value, angles );
	return qtrue;
}

// miscelanea functions
static int CG_LFuncDrawFPS( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	CG_DrawFPS( hud_cursor_x, hud_cursor_y, hud_cursor_align, hud_cursor_font, hud_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawClock( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments )
{
	CG_DrawClock( hud_cursor_x, hud_cursor_y, hud_cursor_align, hud_cursor_font, hud_cursor_color );
	return qtrue;
}

static gs_scriptcommand_t HUDCommands[] =
{
	{
		"setCursor",
		CG_LFuncCursor,
		2,
		"Positions the cursor to x and y coordinates."
	},

	{
		"moveCursor",
		CG_LFuncMoveCursor,
		2,
		"Moves the cursor by x and y pixels."
	},

	{
		"setAlign",
		CG_LFuncAlign,
		2,
		"Changes align setting. Parameters: horizontal alignment, vertical alignment"
	},

	{
		"setSize",
		CG_LFuncSize,
		2,
		"Sets width and height. Used for pictures and models."
	},

	{
		"setFont",
		CG_LFuncFont,
		1,
		"Sets font by font name. Accepts 'con_fontsystemsmall', 'con_fontsystemmedium' and 'con_fontsystembig' as shortcut to default game fonts."
	},

	{
		"setColor",
		CG_LFuncColor,
		4,
		"Sets color setting in RGBA mode. Used for text and pictures"
	},

	{
		"setAngles",
		CG_LFuncAngles,
		3,
		"Sets angles for objects subject of using them. Most likely item models"
	},

	{
		"set3DObjectEffect",
		CG_LFuncSet3DObjectEffect,
		1,
		"Sets effect type to be added to a 3d object. Most likely item models."
	},

	{
		"if",
		CG_LFuncIf,
		1,
		"Conditional expression. Argument accepts operations >, <, ==, >=, <=, etc"
	},

	{
		"endif",
		NULL,
		0,
		"End of conditional expression block"
	},

	{
		"drawString",
		CG_LFuncDrawString,
		1,
		"Draws the string in the argument"
	},

	{
		"drawStringNum",
		CG_LFuncDrawStringNumeric,
		1,
		"Draws numberic values as text"
	},

	{
		"drawItem",
		CG_LFuncDrawItem,
		1,
		"Draws Item model"
	},

	// miscellanea
	{
		"drawFPS",
		CG_LFuncDrawFPS,
		0,
		"Draws fps counter as string"
	},

	{
		"drawClock",
		CG_LFuncDrawClock,
		0,
		"Draws clock counter as string"
	},

	{
		NULL,
		NULL,
		0,
		NULL
	}
};

//=============================================================================
//	DYNAMIC INTERGERS
//=============================================================================

static int CG_GetStat( void *parameter )
{
	assert( (qintptr)parameter >= 0 && (qintptr)parameter < MAX_STATS );

	return cg.predictedPlayerState.stats[(qintptr)parameter];
}

static int CG_GetGameStat( void *parameter )
{
	assert( (qintptr)parameter >= 0 && (qintptr)parameter < MAX_GAME_STATS );

	return gs.gameState.stats[(qintptr)parameter];
}
/*
static unsigned int CG_GetGameLongStat( void *parameter )
{
	assert( (qintptr)parameter >= 0 && (qintptr)parameter < MAX_GAME_LONGSTATS );

	return gs.gameState.longstats[(qintptr)parameter];
}
*/
static int CG_GetCvar( void *parameter )
{
	return trap_Cvar_Value( (char *)parameter );
}

static int CG_GetWeapon( void *parameter )
{
	return cg.predictedEntityState.weapon;
}

static const gs_script_dynamic_t HUDdynamics[] = {
	// stats
	{ "STAT_HEALTH", CG_GetStat, (void *)STAT_HEALTH },
	{ "STAT_LAYOUTS", CG_GetStat, (void *)STAT_FLAGS },
	{ "STAT_PENDING_WEAPON", CG_GetStat, (void *)STAT_PENDING_WEAPON },
	{ "STAT_NEXT_RESPAWN", CG_GetStat, (void *)STAT_NEXT_RESPAWN },

	// game stats
	{ "STAT_MATCH_STATE", CG_GetGameStat, (void *)GAMESTAT_MATCHSTATE },

	// cvars
	{ "SHOW_GUN", CG_GetCvar, "cg_gun" },

	// misc
	{ "WEAPON_ITEM", CG_GetWeapon, NULL },

	{ NULL, NULL, NULL }
};

//=============================================================================
//	CONSTANT INTEGERS
//=============================================================================

static const gs_script_constant_t HUDconstants[] = {
	// teams
	{ "TEAM_NONE", TEAM_NOTEAM },
	{ "TEAM_SPECTATOR", TEAM_SPECTATOR },

	// align
	{ "LEFT", 1 },
	{ "CENTER", 2 },
	{ "RIGHT", 3 },
	{ "TOP", 1 },
	{ "MIDDLE", 2 },
	{ "BOTTOM", 3 },

	{ "WIDTH", 800 },
	{ "HEIGHT", 600 },

	// effects of use on item models
	{ "EFF3D_NONE", EFF3D_NONE },
	{ "EFF3D_WAVEPITCH", EFF3D_WAVEPITCH },
	{ "EFF3D_WAVEROLL", EFF3D_WAVEROLL },
	{ "EFF3D_WAVEYAW", EFF3D_WAVEYAW },
	{ "EFF3D_WAVEPITCH_STRONG", EFF3D_WAVEPITCH_STRONG },
	{ "EFF3D_WAVEROLL_STRONG", EFF3D_WAVEROLL_STRONG },
	{ "EFF3D_WAVEYAW_STRONG", EFF3D_WAVEYAW_STRONG },
	{ "EFF3D_ROTATEPITCH", EFF3D_ROTATEPITCH },
	{ "EFF3D_ROTATEYAW", EFF3D_ROTATEYAW },
	{ "EFF3D_ROTATEROLL", EFF3D_ROTATEROLL },


	{ NULL, 0 }
};

//=============================================================================
//	HELP
//=============================================================================

void Cmd_CG_PrintHudHelp_f( void )
{
	gs_scriptcommand_t *cmd;
	int i;

	GS_Printf( "- %sHUD scripts commands\n---------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( cmd = HUDCommands; cmd->name; cmd++ )
	{
		GS_Printf( "- cmd: %s%s%s expected arguments: %s%i%s\n- desc: %s%s%s\n",
		           S_COLOR_YELLOW, cmd->name, S_COLOR_WHITE,
		           S_COLOR_YELLOW, cmd->numparms, S_COLOR_WHITE,
		           S_COLOR_BLUE, cmd->help, S_COLOR_WHITE );
	}
	GS_Printf( "\n" );

	GS_Printf( "- %sHUD scripts DYNAMIC numeric names\n---------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( i = 0; HUDdynamics[i].name != NULL; i++ )
	{
		GS_Printf( "- %s%s%s\n", S_COLOR_YELLOW, HUDdynamics[i].name, S_COLOR_WHITE );
	}
	GS_Printf( "\n" );

	GS_Printf( "- %sHUD scripts CONSTANT numeric names\n---------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( i = 0; HUDconstants[i].name != NULL; i++ )
	{
		GS_Printf( "%s%s%s, ", S_COLOR_YELLOW, HUDconstants[i].name, S_COLOR_WHITE );
	}
	GS_Printf( "\n", S_COLOR_WHITE );
}

//=============================================================================
//	LOADING AND EXECUTION
//=============================================================================

//================
//CG_HUD_SetUpWindow
//================
static void CG_HUD_SetUpWindow( void )
{
	int width, height, x, y;

	if( cg_viewlockHUD->integer )
	{
		width = cg.view.refdef.width;
		height = cg.view.refdef.height;
		x = cg.view.refdef.x;
		y = cg.view.refdef.y;
	}
	else
	{
		width = cgs.vidWidth;
		height = cgs.vidHeight;
		x = 0;
		y = 0;
	}

	// these are the rules: If the custom window is wider than
	// taller, we lock the hud window inside it in a 3x4 proportion

	if( width * ( 3.0f/4.0f ) >= ( height - 1 ) )
	{
		hud_window_height = height;
		hud_window_width = height * ( 4.0f/3.0f );
		hud_window_x = ( x + ( width * 0.5 ) ) - ( hud_window_width * 0.5 );
		hud_window_y = y;
	}
	// otherwise, let the HUD be stretched in vertical
	else
	{
		hud_window_height = height;
		hud_window_width = width;
		hud_window_x = x;
		hud_window_y = y;
	}
}

//================
//CG_ExecuteHUDScript
//================
void CG_ExecuteHUDScript( void )
{

	CG_HUD_SetUpWindow();

	// setup the cursor defaults
	hud_cursor_x = 0;
	hud_cursor_y = 0;
	hud_cursor_width = 128;
	hud_cursor_height = 128;
	hud_cursor_align = ALIGN_LEFT_TOP;
	Vector4Set( hud_cursor_color, 1, 1, 1, 1 );
	VectorSet( hud_cursor_angles, 0, 0, 0 );
	hud_cursor_3Dobject_effect = EFF3D_NONE;
	hud_cursor_font = cgm.fontSystemSmall;

	GS_ExecuteScriptThread( cg.HUDprogram, HUDCommands, HUDconstants, HUDdynamics );
}

//================
//CG_LoadHUDScript
//================
qboolean CG_LoadHUDScript( char *s )
{
	char name[MAX_QPATH];
	const char *extension = ".hud";
	const char *directory = "huds";

	if( !s || !s[0] )
		return qfalse;

	if( !cg_viewlockHUD )
		cg_viewlockHUD = trap_Cvar_Get( "cg_viewlockHUD", "0", CVAR_ARCHIVE );

	// strip extension and add local path
	Q_strncpyz( name, s, sizeof( name ) );
	COM_StripExtension( name );

	// free old if any
	GS_FreeScriptThread( cg.HUDprogram );

	// load the new status bar program
	cg.HUDprogram = GS_LoadScript( name, directory, extension, HUDCommands, HUDconstants, HUDdynamics );

	return qtrue;
}
