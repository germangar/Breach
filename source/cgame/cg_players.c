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

#include "cg_local.h"

static const char *cg_defaultSexedSounds[] =
{
	"*death.wav", //"*death2.wav", "*death3.wav", "*death4.wav",
	"*fall_0.wav", "*fall_1.wav", "*fall_2.wav",
	"*falldeath.wav",
	"*gasp.wav", "*drown.wav",
	"*jump_1.wav",
	"*pain_soft_1.wav", "*pain_soft_2.wav", "*pain_medium.wav", "*pain_strong.wav",
	"*wj_1.wav", "*wj_2.wav",
	"*dash_1.wav", "*dash_2.wav",
	"*taunt.wav",
	NULL
};


//================
//CG_RegisterPmodelSexedSound
//================
static struct sfx_s *CG_RegisterPmodelSexedSound( playerobject_t *pmodelObject, const char *name )
{
	char *s, model[MAX_QPATH];
	cg_sexedSfx_t *sexedSfx;
	char oname[MAX_QPATH];
	char sexedFilename[MAX_QPATH];

	if( !pmodelObject )
		return NULL;

	Q_strncpyz( oname, name, sizeof( oname ) );
	COM_StripExtension( oname );
	for( sexedSfx = pmodelObject->sexedSfx; sexedSfx; sexedSfx = sexedSfx->next )
	{
		if( !Q_stricmp( sexedSfx->name, oname ) )
			return sexedSfx->sfx;
	}

	model[0] = 0;

	s = pmodelObject->name;
	if( s && s[0] )
		Q_strncpyz( model, pmodelObject->name, sizeof( model ) );

	// if we can't figure it out, use male or female
	if( !model[0] )
	{
		if( pmodelObject->sex == GENDER_FEMALE )
			Q_strncpyz( model, "female", sizeof( model ) );
		else if( pmodelObject->sex == GENDER_NEUTRAL )
			Q_strncpyz( model, "neutral", sizeof( model ) );
		else
			Q_strncpyz( model, "male", sizeof( model ) );
	}

	sexedSfx = ( cg_sexedSfx_t * )CG_Malloc( sizeof( cg_sexedSfx_t ) );
	sexedSfx->name = GS_CopyString( oname );
	sexedSfx->next = pmodelObject->sexedSfx;
	pmodelObject->sexedSfx = sexedSfx;

	// see if we already know of the model specific sound
	Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", model, oname+1 );

	if( ( !COM_FileExtension( sexedFilename ) &&
		trap_FS_FirstExtension( sexedFilename, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) ) ||
		trap_FS_FOpenFile( sexedFilename, NULL, FS_READ ) != -1 )
	{
		sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
	}
	else
	{
		// no, revert to default player sounds folders
		if( pmodelObject->sex == GENDER_FEMALE )
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/female/%s", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
		else
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/male/%s", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
	}

	return sexedSfx->sfx;
}

//================
//CG_UpdateSexedSoundsRegistration
//================
void CG_UpdateSexedSoundsRegistration( playerobject_t *pmodelObject )
{
	cg_sexedSfx_t *sexedSfx, *next;
	const char *name;
	int i;

	if( !pmodelObject )
		return;

	// free loaded sounds
	for( sexedSfx = pmodelObject->sexedSfx; sexedSfx; sexedSfx = next )
	{
		next = sexedSfx->next;
		CG_Free( sexedSfx );
	}
	pmodelObject->sexedSfx = NULL;

	// load default sounds
	for( i = 0;; i++ )
	{
		name = cg_defaultSexedSounds[i];
		if( !name )
			break;
		CG_RegisterPmodelSexedSound( pmodelObject, name );
	}

	// load sounds server told us
	for( i = 1; i < MAX_SOUNDS; i++ )
	{
		name = trap_GetConfigString( CS_SOUNDS+i );
		if( !name || !name[0] )
			break;
		if( name[0] == '*' )
			CG_RegisterPmodelSexedSound( pmodelObject, name );
	}
}

//================
//CG_RegisterSexedSound
//================
struct sfx_s *CG_RegisterSexedSound( int entnum, char *name )
{
	if( entnum < 0 || entnum >= MAX_EDICTS )
		return NULL;
	return CG_RegisterPmodelSexedSound( cg_entities[entnum].pmodelObject, name );
}

//================
//CG_SexedSound
//================
void CG_SexedSound( int entnum, int entchannel, char *name, float fvol )
{
	if( ISVIEWERENTITY( entnum ) )
	{
		trap_S_StartGlobalSound( CG_RegisterSexedSound( entnum, name ), entchannel, fvol );
	}
	else
	{
		trap_S_StartRelativeSound( CG_RegisterSexedSound( entnum, name ), entnum, entchannel, fvol, ATTN_NORMAL );
	}
}


//================
//CG_LoadClientInfo
//================
void CG_LoadClientInfo( cg_clientInfo_t *ci, const char *cstring, int client )
{
	char *s;
	int rgbcolor;

	s = Info_ValueForKey( cstring, "name" );
	if( s && s[0] )
	{
		Q_strncpyz( ci->name, s, sizeof( ci->name ) );
	}
	else
	{
		Q_strncpyz( ci->name, "badname", sizeof( ci->name ) );
	}

	s = Info_ValueForKey( cstring, "hand" );
	if( s && s[0] )
	{
		ci->hand = atoi( s );
	}
	else
	{
		ci->hand = 0;
	}

	s = Info_ValueForKey( cstring, "fov" );
	if( s && s[0] )
		ci->fov = (float)atoi( s );
	else
		ci->fov = 90;

	s = Info_ValueForKey( cstring, "zfov" );
	if( s && s[0] )
		ci->zoomfov = (float)atoi( s );
	else
		ci->zoomfov = 40;

	Vector4Set( ci->color, 255, 255, 255, 255 );
	rgbcolor = COM_ReadColorRGBString( Info_ValueForKey( cstring, "color" ) );
	if( rgbcolor != -1 )
	{
		Vector4Set( ci->color, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );
	}
}
