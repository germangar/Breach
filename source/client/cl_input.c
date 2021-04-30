/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

extern cvar_t *cl_maxfps;

cvar_t *cl_ucmdMaxResend;
cvar_t *cl_ucmdFPS;
#ifdef UCMDTIMENUDGE
cvar_t *cl_ucmdTimeNudge;
#endif

cvar_t *sensitivity;
cvar_t *m_accel;
cvar_t *m_filter;

extern unsigned	sys_frame_time;
unsigned keys_frame_time;
unsigned old_keys_frame_time;

/*
===============================================================================

MOUSE

===============================================================================
*/
extern cvar_t *in_grabinconsole;

static unsigned int mouse_frame_time = 0;

/*
* CL_MouseMove
*/
void CL_MouseMove( usercmd_t *cmd, int mx, int my )
{
	static unsigned int mouse_time = 0, old_mouse_time = 0xFFFFFFFF;
	static float mouse_x = 0, mouse_y = 0;
	static float old_mouse_x = 0, old_mouse_y = 0;
	float accelSensitivity;
	float	rate;

	old_mouse_time = mouse_time;
	mouse_time = Sys_Milliseconds();
	if( old_mouse_time >= mouse_time )
		old_mouse_time = mouse_time - 1;

	mouse_frame_time = mouse_time - old_mouse_time;

	if( cls.key_dest == key_menu )
	{
		CL_UIModule_MouseMove( mx, my );
		return;
	}

	if( ( cls.key_dest == key_console ) && !in_grabinconsole->integer )
		return;

	if( cls.state < CA_ACTIVE )
		return;

	// allow mouse smoothing
	if ( m_filter->integer ) 
	{
		mouse_x = ( mx + old_mouse_x ) * 0.5;
		mouse_y = ( my + old_mouse_y ) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	if( mouse_frame_time )
		rate = sqrt( mouse_x * mouse_x + mouse_y * mouse_y ) / (float)mouse_frame_time;
	else
		rate = 0;

	accelSensitivity = sensitivity->value + ( rate * m_accel->value );
	accelSensitivity *= CL_GameModule_SetSensitivityScale( sensitivity->value );

	mouse_x *= accelSensitivity;
	mouse_y *= accelSensitivity;

	if( !mouse_x && !mouse_y )
		return;

	// add mouse X/Y movement to cmd
	cl.viewangles[YAW] -= ( m_yaw->value * mouse_x );
	cl.viewangles[PITCH] += ( m_pitch->value * mouse_y );
}

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int key, qboolean down, unsigned time);

===============================================================================
*/

kbutton_t in_klook;
kbutton_t in_left, in_right, in_forward, in_back;
kbutton_t in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t in_strafe, in_speed, in_activate, in_attack;
kbutton_t in_up, in_down;
kbutton_t in_special;
kbutton_t in_modebutton;
kbutton_t in_zoom;

/*
* KeyDown
*/
static void KeyDown( kbutton_t *b )
{
	int k;
	char *c;

	c = Cmd_Argv( 1 );
	if( c[0] )
		k = atoi( c );
	else
		k = -1; // typed manually at the console for continuous down

	if( k == b->down[0] || k == b->down[1] )
		return; // repeating key

	if( !b->down[0] )
		b->down[0] = k;
	else if( !b->down[1] )
		b->down[1] = k;
	else
	{
		Com_Printf( "Three keys down for a button!\n" );
		return;
	}

	if( b->state & 1 )
		return; // still down

	// save timestamp
	c = Cmd_Argv( 2 );
	b->downtime = atoi( c );
	if( !b->downtime )
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2; // down + impulse down
}

/*
* KeyUp
*/
static void KeyUp( kbutton_t *b )
{
	int k;
	char *c;
	unsigned uptime;

	c = Cmd_Argv( 1 );
	if( c[0] )
		k = atoi( c );
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4; // impulse up
		return;
	}

	if( b->down[0] == k )
		b->down[0] = 0;
	else if( b->down[1] == k )
		b->down[1] = 0;
	else
		return; // key up without corresponding down (menu pass through)
	if( b->down[0] || b->down[1] )
		return; // some other key is still holding it down

	if( !( b->state & 1 ) )
		return; // still up (this should not happen)

	// save timestamp
	c = Cmd_Argv( 2 );
	uptime = atoi( c );
	if( uptime )
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1; // now up
	b->state |= 4;  // impulse up
}

static void IN_KLookDown( void ) { KeyDown( &in_klook ); }
static void IN_KLookUp( void ) { KeyUp( &in_klook ); }
static void IN_UpDown( void ) { KeyDown( &in_up ); }
static void IN_UpUp( void ) { KeyUp( &in_up ); }
static void IN_DownDown( void ) { KeyDown( &in_down ); }
static void IN_DownUp( void ) { KeyUp( &in_down ); }
static void IN_LeftDown( void ) { KeyDown( &in_left ); }
static void IN_LeftUp( void ) { KeyUp( &in_left ); }
static void IN_RightDown( void ) { KeyDown( &in_right ); }
static void IN_RightUp( void ) { KeyUp( &in_right ); }
static void IN_ForwardDown( void ) { KeyDown( &in_forward ); }
static void IN_ForwardUp( void ) { KeyUp( &in_forward ); }
static void IN_BackDown( void ) { KeyDown( &in_back ); }
static void IN_BackUp( void ) { KeyUp( &in_back ); }
static void IN_LookupDown( void ) { KeyDown( &in_lookup ); }
static void IN_LookupUp( void ) { KeyUp( &in_lookup ); }
static void IN_LookdownDown( void ) { KeyDown( &in_lookdown ); }
static void IN_LookdownUp( void ) { KeyUp( &in_lookdown ); }
static void IN_MoveleftDown( void ) { KeyDown( &in_moveleft ); }
static void IN_MoveleftUp( void ) { KeyUp( &in_moveleft ); }
static void IN_MoverightDown( void ) { KeyDown( &in_moveright ); }
static void IN_MoverightUp( void ) { KeyUp( &in_moveright ); }
static void IN_StrafeDown( void ) { KeyDown( &in_strafe ); }
static void IN_StrafeUp( void ) { KeyUp( &in_strafe ); }
static void IN_AttackDown( void ) { KeyDown( &in_attack ); }
static void IN_AttackUp( void ) { KeyUp( &in_attack ); }
static void IN_SpeedDown( void ) { KeyDown( &in_speed ); }
static void IN_SpeedUp( void ) { KeyUp( &in_speed ); }
static void IN_ActivateDown( void ) { KeyDown( &in_activate ); }
static void IN_ActivateUp( void ) { KeyUp( &in_activate ); }
static void IN_ZoomDown( void ) { KeyDown( &in_zoom ); }
static void IN_ZoomUp( void ) { KeyUp( &in_zoom ); }
static void IN_SpecialDown( void ) { KeyDown( &in_special ); }
static void IN_SpecialUp( void ) { KeyUp( &in_special ); }
static void IN_ModeButtonDown( void ) { KeyDown( &in_modebutton ); }
static void IN_ModeButtonUp( void ) { KeyUp( &in_modebutton ); }

/*
* CL_KeyState
*/
static float CL_KeyState( kbutton_t *key )
{
	float val;
	int msec;

	key->state &= 1; // clear impulses

	msec = key->msec;
	key->msec = 0;

	if( key->state )
	{
		// still down
		msec += sys_frame_time - key->downtime;
		key->downtime = sys_frame_time;
	}

	val = (float)msec / (float)keys_frame_time;

	return bound( 0, val, 1 );
}

//==========================================================================

cvar_t *cl_yawspeed;
cvar_t *cl_pitchspeed;

cvar_t *cl_anglespeedkey;

/*
* CL_AddButtonBits
*/
static void CL_AddButtonBits( qbyte *buttons )
{
	// figure button bits
	if( in_attack.state & 3 )
		*buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;

	if( in_speed.state & 1 )
		*buttons |= BUTTON_SPEED;
	in_speed.state &= ~2;

	if( in_activate.state & 3 )
		*buttons |= BUTTON_ACTIVATE;
	in_activate.state &= ~2;

	if( in_zoom.state & 3 )
		*buttons |= BUTTON_ZOOM;
	in_zoom.state &= ~2;

	if( in_special.state & 3 )
		*buttons |= BUTTON_SPECIAL;
	in_special.state &= ~2;

	if( in_modebutton.state & 3 )
		*buttons |= BUTTON_MODE;
	in_modebutton.state &= ~2;

	// add chat/console/ui icon as a button
	if( cls.key_dest != key_game )
		*buttons |= BUTTON_BUSYICON;

	if( anykeydown && cls.key_dest == key_game )
		*buttons |= BUTTON_ANY;
}

/*
* CL_AddAnglesFromKeys
*/
static void CL_AddAnglesFromKeys( int frametime )
{
	float speed;

	if( !frametime )
		return;

	if( in_speed.state & 1 )
		speed = ( (float)frametime * 0.001f ) * cl_anglespeedkey->value;
	else
		speed = (float)frametime * 0.001f;

	if( !( in_strafe.state & 1 ) )
	{
		cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState( &in_right );
		cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState( &in_left );
	}
	if( in_klook.state & 1 )
	{
		cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState( &in_forward );
		cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState( &in_back );
	}

	cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState( &in_lookup );
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState( &in_lookdown );
}

/*
* CL_AddMovementFromKeys
*/
static void CL_AddMovementFromKeys( short *forwardmove, short *sidemove, short *upmove, int frametime )
{
	if( !frametime )
		return;

	if( in_strafe.state & 1 )
	{
		*sidemove += frametime * CL_KeyState( &in_right );
		*sidemove -= frametime * CL_KeyState( &in_left );
	}

	*sidemove += frametime * CL_KeyState( &in_moveright );
	*sidemove -= frametime * CL_KeyState( &in_moveleft );

	*upmove += frametime * CL_KeyState( &in_up );
	*upmove -= frametime * CL_KeyState( &in_down );

	if( !( in_klook.state & 1 ) )
	{
		*forwardmove += frametime * CL_KeyState( &in_forward );
		*forwardmove -= frametime * CL_KeyState( &in_back );
	}
}

/*
* CL_UpdateCommandInput
*/
void CL_UpdateCommandInput( void )
{
	usercmd_t *cmd;

	if( cl.inputRefreshed )
		return;

	keys_frame_time = ( sys_frame_time - old_keys_frame_time ) & 0xFF;

	cmd = &cl.cmds[cls.ucmdHead & CMD_MASK];

	// always let the mouse refresh cl.viewangles
	IN_MouseMove( cmd );
	CL_AddButtonBits( &cmd->buttons );

	if( keys_frame_time )
	{
		cmd->msec += keys_frame_time;

		CL_AddAnglesFromKeys( keys_frame_time );
		CL_AddMovementFromKeys( &cmd->forwardmove, &cmd->sidemove, &cmd->upmove, keys_frame_time );
		IN_JoyMove( cmd );
		old_keys_frame_time = sys_frame_time;
	}

	if( cmd->msec )
	{
		cmd->forwardfrac = ( (float)cmd->forwardmove/(float)cmd->msec );
		cmd->sidefrac = ( (float)cmd->sidemove/(float)cmd->msec );
		cmd->upfrac = ( (float)cmd->upmove/(float)cmd->msec );
	}

	cmd->angles[0] = ANGLE2SHORT( cl.viewangles[0] );
	cmd->angles[1] = ANGLE2SHORT( cl.viewangles[1] );
	cmd->angles[2] = ANGLE2SHORT( cl.viewangles[2] );

	cl.inputRefreshed = qtrue;
}

/*
* IN_CenterView
*/
void IN_CenterView( void )
{
	/*	if( !cl.curFrame )
	return;

	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.curFrame->playerState.moveState.delta_angles[PITCH]);
	*/
}

/*
* CL_InitInput
*/
void CL_InitInput( void )
{
	Cmd_AddCommand( "in_restart", IN_Restart );

	// get input cvars
	cl_ucmdMaxResend =	Cvar_Get( "cl_ucmdMaxResend", "3", CVAR_ARCHIVE );
	cl_ucmdFPS =		Cvar_Get( "cl_ucmdFPS", "62", CVAR_DEVELOPER );

	sensitivity =		Cvar_Get( "sensitivity", "3", CVAR_ARCHIVE );
	m_accel =		Cvar_Get( "m_accel", "0", CVAR_ARCHIVE );
	m_filter =		Cvar_Get( "m_filter", "1", CVAR_ARCHIVE );

#ifdef UCMDTIMENUDGE
	cl_ucmdTimeNudge =	Cvar_Get( "cl_ucmdTimeNudge", "0", CVAR_USERINFO|CVAR_DEVELOPER );
	if( abs( cl_ucmdTimeNudge->integer ) > MAX_UCMD_TIMENUDGE )
	{
		if( cl_ucmdTimeNudge->integer < -MAX_UCMD_TIMENUDGE )
			Cvar_SetValue( "cl_ucmdTimeNudge", -MAX_UCMD_TIMENUDGE );
		else if( cl_ucmdTimeNudge->integer > MAX_UCMD_TIMENUDGE )
			Cvar_SetValue( "cl_ucmdTimeNudge", MAX_UCMD_TIMENUDGE );
	}
#endif

	IN_Init();

	// add input commands
	Cmd_AddCommand( "centerview", IN_CenterView );
	Cmd_AddCommand( "+moveup", IN_UpDown );
	Cmd_AddCommand( "-moveup", IN_UpUp );
	Cmd_AddCommand( "+movedown", IN_DownDown );
	Cmd_AddCommand( "-movedown", IN_DownUp );
	Cmd_AddCommand( "+left", IN_LeftDown );
	Cmd_AddCommand( "-left", IN_LeftUp );
	Cmd_AddCommand( "+right", IN_RightDown );
	Cmd_AddCommand( "-right", IN_RightUp );
	Cmd_AddCommand( "+forward", IN_ForwardDown );
	Cmd_AddCommand( "-forward", IN_ForwardUp );
	Cmd_AddCommand( "+back", IN_BackDown );
	Cmd_AddCommand( "-back", IN_BackUp );
	Cmd_AddCommand( "+lookup", IN_LookupDown );
	Cmd_AddCommand( "-lookup", IN_LookupUp );
	Cmd_AddCommand( "+lookdown", IN_LookdownDown );
	Cmd_AddCommand( "-lookdown", IN_LookdownUp );
	Cmd_AddCommand( "+strafe", IN_StrafeDown );
	Cmd_AddCommand( "-strafe", IN_StrafeUp );
	Cmd_AddCommand( "+moveleft", IN_MoveleftDown );
	Cmd_AddCommand( "-moveleft", IN_MoveleftUp );
	Cmd_AddCommand( "+moveright", IN_MoverightDown );
	Cmd_AddCommand( "-moveright", IN_MoverightUp );

	Cmd_AddCommand( "+klook", IN_KLookDown );
	Cmd_AddCommand( "-klook", IN_KLookUp );

	Cmd_AddCommand( "+attack", IN_AttackDown );
	Cmd_AddCommand( "-attack", IN_AttackUp );
	Cmd_AddCommand( "+speed", IN_SpeedDown );
	Cmd_AddCommand( "-speed", IN_SpeedUp );
	Cmd_AddCommand( "+activate", IN_ActivateDown );
	Cmd_AddCommand( "-activate", IN_ActivateUp );
	Cmd_AddCommand( "+zoom", IN_ZoomDown );
	Cmd_AddCommand( "-zoom", IN_ZoomUp );
	Cmd_AddCommand( "+special", IN_SpecialDown );
	Cmd_AddCommand( "-special", IN_SpecialUp );
	Cmd_AddCommand( "+mode", IN_ModeButtonDown );
	Cmd_AddCommand( "-mode", IN_ModeButtonUp );
}

/*
* CL_ShutdownInput
*/
void CL_ShutdownInput( void )
{
	Cmd_RemoveCommand( "in_restart" );

	IN_Shutdown();

	Cmd_RemoveCommand( "centerview" );
	Cmd_RemoveCommand( "+moveup" );
	Cmd_RemoveCommand( "-moveup" );
	Cmd_RemoveCommand( "+movedown" );
	Cmd_RemoveCommand( "-movedown" );
	Cmd_RemoveCommand( "+left" );
	Cmd_RemoveCommand( "-left" );
	Cmd_RemoveCommand( "+right" );
	Cmd_RemoveCommand( "-right" );
	Cmd_RemoveCommand( "+forward" );
	Cmd_RemoveCommand( "-forward" );
	Cmd_RemoveCommand( "+back" );
	Cmd_RemoveCommand( "-back" );
	Cmd_RemoveCommand( "+lookup" );
	Cmd_RemoveCommand( "-lookup" );
	Cmd_RemoveCommand( "+lookdown" );
	Cmd_RemoveCommand( "-lookdown" );
	Cmd_RemoveCommand( "+strafe" );
	Cmd_RemoveCommand( "-strafe" );
	Cmd_RemoveCommand( "+moveleft" );
	Cmd_RemoveCommand( "-moveleft" );
	Cmd_RemoveCommand( "+moveright" );
	Cmd_RemoveCommand( "-moveright" );
	Cmd_RemoveCommand( "+speed" );
	Cmd_RemoveCommand( "-speed" );
	Cmd_RemoveCommand( "+attack" );
	Cmd_RemoveCommand( "-attack" );
	Cmd_RemoveCommand( "+use" );
	Cmd_RemoveCommand( "-use" );
	Cmd_RemoveCommand( "+klook" );
	Cmd_RemoveCommand( "-klook" );
	Cmd_RemoveCommand( "+special" );
	Cmd_RemoveCommand( "-special" );
	Cmd_RemoveCommand( "+zoom" );
	Cmd_RemoveCommand( "-zoom" );
}

//===============================================================================
//
//	UCMDS
//
//===============================================================================

/*
* CL_UserInputFrame
*/
void CL_UserInputFrame( void )
{
	// let the mouse activate or deactivate
	IN_Frame();

	// get new key events
	Sys_SendKeyEvents();

	// get new key events from mice or external controllers
	IN_Commands();

	// process console commands
	Cbuf_Execute();
}

/*
* MSG_WriteDeltaUsercmd
*/
static void MSG_WriteDeltaUsercmd( msg_t *buf, usercmd_t *from, usercmd_t *cmd )
{
	int bits;

	// send the movement message

	bits = 0;
	if( cmd->angles[0] != from->angles[0] )
		bits |= CM_ANGLE1;
	if( cmd->angles[1] != from->angles[1] )
		bits |= CM_ANGLE2;
	if( cmd->angles[2] != from->angles[2] )
		bits |= CM_ANGLE3;
	if( cmd->forwardfrac != from->forwardfrac )
		bits |= CM_FORWARD;
	if( cmd->sidefrac != from->sidefrac )
		bits |= CM_SIDE;
	if( cmd->upfrac != from->upfrac )
		bits |= CM_UP;
	if( cmd->buttons != from->buttons )
		bits |= CM_BUTTONS;

	MSG_WriteByte( buf, bits );

	if( bits & CM_ANGLE1 )
		MSG_WriteShort( buf, cmd->angles[0] );
	if( bits & CM_ANGLE2 )
		MSG_WriteShort( buf, cmd->angles[1] );
	if( bits & CM_ANGLE3 )
		MSG_WriteShort( buf, cmd->angles[2] );

	if( bits & CM_FORWARD )
		MSG_WriteChar( buf, (int)( cmd->forwardfrac * UCMD_PUSHFRAC_SNAPSIZE ) );
	if( bits & CM_SIDE )
		MSG_WriteChar( buf, (int)( cmd->sidefrac * UCMD_PUSHFRAC_SNAPSIZE ) );
	if( bits & CM_UP )
		MSG_WriteChar( buf, (int)( cmd->upfrac * UCMD_PUSHFRAC_SNAPSIZE ) );

	if( bits & CM_BUTTONS )
		MSG_WriteByte( buf, cmd->buttons );

	MSG_WriteLong( buf, cmd->serverTimeStamp );
}

/*
* CL_WriteUcmdsToMessage
*/
void CL_WriteUcmdsToMessage( msg_t *msg )
{
	usercmd_t *cmd;
	usercmd_t *oldcmd;
	usercmd_t nullcmd;
	unsigned int resendCount;
	unsigned int i;
	unsigned int ucmdFirst;
	unsigned int ucmdHead;
	qbyte bitflags;

	if( !msg || cls.state < CA_ACTIVE || cls.demo.playing )
		return;

	// find out what ucmds we have to send
	ucmdFirst = cls.ucmdAcknowledged + 1;
	ucmdHead = cl.cmdNum + 1;

	if( cl_ucmdMaxResend->integer > CMD_BACKUP * 0.5 )
		Cvar_SetValue( "cl_ucmdMaxResend", CMD_BACKUP * 0.5 );
	else if( cl_ucmdMaxResend->integer < 1 )
		Cvar_SetValue( "cl_ucmdMaxResend", 1 );

	// find what is our resend count (resend doesn't include the newly generated ucmds)
	// and move the start back to the resend start
	if( ucmdFirst <= cls.ucmdSent + 1 )
		resendCount = 0;
	else
		resendCount = ( cls.ucmdSent + 1 ) - ucmdFirst;
	if( resendCount > (unsigned int)cl_ucmdMaxResend->integer )
		resendCount = (unsigned int)cl_ucmdMaxResend->integer;

	if( ucmdFirst > ucmdHead )
		ucmdFirst = ucmdHead;

	// if this happens, the player is in a freezing lag. Send him the less possible data
	if( ( ucmdHead - ucmdFirst ) + resendCount > CMD_MASK * 0.5 )
		resendCount = 0;

	// move the start backwards to the resend point
	ucmdFirst = ( ucmdFirst > resendCount ) ? ucmdFirst - resendCount : ucmdFirst;

	if( ( ucmdHead - ucmdFirst ) > CMD_MASK ) // ran out of updates, reduce the send to try to recover activity
		ucmdFirst = ucmdHead - 3;

	// begin a client move command
	MSG_WriteByte( msg, clc_move );

	// write bitflags byte
	bitflags = 0;
	if( cl.receivedSnapNum == 0 || cls.demo.waiting )  // request no delta compression
		bitflags |= 1;
	MSG_WriteByte( msg, bitflags );

	// acknowledge server frame snap
	MSG_WriteLong( msg, cl.snapShots[cl.receivedSnapNum & SNAPS_BACKUP_MASK].snapNum );

	// Write the actual ucmds

	// write the id number of first ucmd to be sent, and the count
	MSG_WriteLong( msg, ucmdHead );
	MSG_WriteByte( msg, (qbyte)( ucmdHead - ucmdFirst ) );

	// write the ucmds
	for( i = ucmdFirst; i < ucmdHead; i++ )
	{
		if( i == ucmdFirst ) // first one isn't delta-compressed
		{
			cmd = &cl.cmds[i & CMD_MASK];
			memset( &nullcmd, 0, sizeof( nullcmd ) );
			MSG_WriteDeltaUsercmd( msg, &nullcmd, cmd );
		}
		else // delta compress to previous written
		{
			cmd = &cl.cmds[i & CMD_MASK];
			oldcmd = &cl.cmds[( i-1 ) & CMD_MASK];
			MSG_WriteDeltaUsercmd( msg, oldcmd, cmd );
		}
	}
	cls.ucmdSent = i;
}

/*
* CL_NextUserCommandTimeReached
*/
static qboolean CL_NextUserCommandTimeReached( int realmsec )
{
	static int minMsec = 1, allMsec = 0, extraMsec = 0;
	static float roundingMsec = 0.0f;
	float maxucmds;

	if( cls.state < CA_ACTIVE )
		maxucmds = 10; // reduce ratio while connecting
	else
		maxucmds = cl_ucmdFPS->value;

	// the cvar is developer only
	//clamp( maxucmds, 10, 90 ); // don't let people abuse cl_ucmdFPS

	if( !cl_timedemo->integer && !cls.demo.playing )
	{
		minMsec = ( 1000.0f / maxucmds );
		roundingMsec += ( 1000.0f / maxucmds ) - minMsec;
	}
	else
		minMsec = 1;

	if( roundingMsec >= 1.0f )
	{
		minMsec += (int)roundingMsec;
		roundingMsec -= (int)roundingMsec;
	}

	if( minMsec > extraMsec )  // remove, from min frametime, the extra time we spent in last frame
		minMsec -= extraMsec;

	allMsec += realmsec;
	if( allMsec < minMsec )
	{
		//if( !cls.netchan.unsentFragments ) {
		//	NET_Sleep( minMsec - allMsec );
		return qfalse;
	}

	extraMsec = allMsec - minMsec;
	if( extraMsec > minMsec )
		extraMsec = minMsec - 1;

	allMsec = 0;

	// send a new user command message to the server
	return qtrue;
}

/*
* CL_NewUserCommand
*/
void CL_NewUserCommand( int realmsec )
{
	usercmd_t *ucmd;

	if( !CL_NextUserCommandTimeReached( realmsec ) )
		return;

	if( cls.state < CA_ACTIVE )
		return;

	cl.cmdNum = cls.ucmdHead;
	ucmd = &cl.cmds[cl.cmdNum & CMD_MASK];
	ucmd->serverTimeStamp = cl.serverTime; // return the time stamp to the server
	cl.cmd_time[cl.cmdNum & CMD_MASK] = cls.realtime;

	// control cinematics by buttons
	if( ucmd->buttons && SCR_GetCinematicTime() > 0 && cls.realtime > 1000 + SCR_GetCinematicTime() )
	{
		SCR_StopCinematic();
		SCR_FinishCinematic();
		SCR_UpdateScreen();
	}

	// snap push fracs so client and server version match
	ucmd->forwardfrac = ( (int)( UCMD_PUSHFRAC_SNAPSIZE * ucmd->forwardfrac ) ) / UCMD_PUSHFRAC_SNAPSIZE;
	ucmd->sidefrac = ( (int)( UCMD_PUSHFRAC_SNAPSIZE * ucmd->sidefrac ) ) / UCMD_PUSHFRAC_SNAPSIZE;
	ucmd->upfrac = ( (int)( UCMD_PUSHFRAC_SNAPSIZE * ucmd->upfrac ) ) / UCMD_PUSHFRAC_SNAPSIZE;

	if( cl.cmdNum > 0 )
		ucmd->msec = ucmd->serverTimeStamp - cl.cmds[( cl.cmdNum-1 ) & CMD_MASK].serverTimeStamp;
	else
		ucmd->msec = 20;

	if( ucmd->msec < 1 )
		ucmd->msec = 1;

	// advance head and init the new command
	cls.ucmdHead++;
	ucmd = &cl.cmds[cls.ucmdHead & CMD_MASK];
	memset( ucmd, 0, sizeof( usercmd_t ) );

	// start up with the most recent viewangles
	ucmd->angles[0] = ANGLE2SHORT( cl.viewangles[0] );
	ucmd->angles[1] = ANGLE2SHORT( cl.viewangles[1] );
	ucmd->angles[2] = ANGLE2SHORT( cl.viewangles[2] );
}
