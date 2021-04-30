/*
   Copyright (C) 2002-2003 Victor Luchits

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

cgame_import_t CGAME_IMPORT;

/*
   =================
   GetCGameAPI

   Returns a pointer to the structure with all entry points
   =================
 */
cgame_export_t *GetCGameAPI( cgame_import_t *import )
{
	static cgame_export_t globals;

	CGAME_IMPORT = *import;

	globals.API = CG_API;

	globals.Init = CG_Init;
	globals.Shutdown = CG_Shutdown;

	globals.ServerCommand = CG_ServerCommand;
	globals.ConfigStringUpdate = CG_ConfigStringUpdate;

	globals.EscapeKey = CG_EscapeKey;

	globals.NewSnapshot = CG_NewSnapshot;
	globals.GetEntitySpatilization = CG_GetEntitySpatilization;

	globals.SetSensitivityScale = CG_SetSensitivityScale;

	globals.Trace = GS_Trace;
	globals.RenderView = CG_RenderView;

	return &globals;
}

#if defined ( HAVE_DLLMAIN ) && !defined ( CGAME_HARD_LINKED )
int _stdcall DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
