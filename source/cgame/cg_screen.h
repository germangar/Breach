/*
   Copyright (C) 2007 German Garcia
 */

typedef struct
{
	char title[MAX_QPATH];
	char topic[MAX_QPATH];
	char file[MAX_QPATH];
} loadingscreen_t;

extern void CG_DrawLoadingScreen( void );
extern void CG_LoadingScreen_Topicname( char *str );
extern void CG_LoadingScreen_Filename( const char *string );
extern void CG_EscapeKey( void );
extern void CG_Init2D( void );
extern void CG_Draw2D( void );

// draw stuff
extern void CG_DrawItem( int x, int y, int align, int w, int h, int itemID, vec3_t angles );
extern void CG_DrawFPS( int x, int y, int align, struct mufont_s *font, vec4_t color );
extern void CG_DrawClock( int x, int y, int align, struct mufont_s *font, vec4_t color );
