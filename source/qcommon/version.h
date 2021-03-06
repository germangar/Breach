/*
Copyright (C) 2007 Victor Luchits

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

#ifndef APPLICATION
# define APPLICATION		"breach"
#endif

#ifndef APP_VERSION_MAJOR
# define APP_VERSION_MAJOR	0
#endif

#ifndef APP_VERSION_MINOR
# define APP_VERSION_MINOR	1
#endif

#ifndef APP_VERSION_UPDATE
#define APP_VERSION_UPDATE	0
#endif

#ifndef APP_VERSION
# define APP_VERSION		APP_VERSION_MAJOR + APP_VERSION_MINOR*0.1 + APP_VERSION_UPDATE*0.01
#endif

#ifdef PUBLIC_BUILD
#define APP_PROTOCOL_VERSION    1
#else
#define APP_PROTOCOL_VERSION    1342 // we're using revision number as protocol version for internal builds
#endif

#ifndef APP_URL
#define	APP_URL							""
#endif

#ifndef APP_UPDATE_URL
#define	APP_UPDATE_URL					""
#define	APP_SERVER_UPDATE_DIRECTORY		"autoupdate/"APP_VERSION_STR"/"
#define APP_SERVER_UPDATE_FILE			"filelist.txt"
#define APP_CLIENT_UPDATE_FILE			APPLICATION"_last_version.txt"
#endif

#ifndef APP_COPYRIGHT_OWNER
#define APP_COPYRIGHT_OWNER				"German Garcia and Victor Luchits"
#endif


#ifndef APP_SCREENSHOTS_PREFIX
#define APP_SCREENSHOTS_PREFIX			APPLICATION"_"
#endif

#undef STR_HELPER
#undef STR_TOSTR

#define STR_HELPER(s)		#s
#define STR_TOSTR(x)		STR_HELPER(x)

#define APP_PROTOCOL_VERRSION_STR		STR_TOSTR( APP_PROTOCOL_VERSION )
#define APP_DEMO_EXTENSION_STR			".d" APP_PROTOCOL_VERRSION_STR

#define APP_URI_SCHEME			APPLICATION "://"
#define APP_URI_PROTO_SCHEME	APPLICATION STR_TOSTR( APP_PROTOCOL_VERSION ) "://"

#ifndef APP_VERSION_STR
#define APP_VERSION_STR					STR_TOSTR( APP_VERSION_MAJOR ) "." STR_TOSTR( APP_VERSION_MINOR ) STR_TOSTR( APP_VERSION_UPDATE )
#endif

#ifndef APP_VERSION_STR_MAJORMINOR
#define APP_VERSION_STR_MAJORMINOR		STR_TOSTR( APP_VERSION_MAJOR ) STR_TOSTR( APP_VERSION_MINOR )
#endif

//
// the following macros are only used by the windows resource file
//
#ifndef APP_VERSION_RC
#define APP_VERSION_RC					APP_VERSION_MAJOR.APP_VERSION_MINOR
#endif

#ifndef APP_VERSION_RC_STR
#define APP_VERSION_RC_STR				STR_TOSTR( APP_VERSION_RC )
#endif

#ifndef APP_FILEVERSION_RC				
#define APP_FILEVERSION_RC				APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_UPDATE,0
#endif

#ifndef APP_FILEVERSION_RC_STR
#define APP_FILEVERSION_RC_STR			STR_TOSTR( APP_FILEVERSION_RC )
#endif
