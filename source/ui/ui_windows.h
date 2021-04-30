/*
 */

#ifndef __UI_WINDOWS_H__
#define __UI_WINDOWS_H__

#define WINDOW_NAME_SIZE	32

typedef struct uiwindow_s
{
	char name[WINDOW_NAME_SIZE];
	int x, y, w, h;
	qboolean mergeablefocus;	// allows its items to be used when on focus even if not being the top window
	qboolean blockdraw; // skip drawing windows below this one (and blocks focus too)
	qboolean blockfocus; // disallow selecting anything below this one

	struct shader_s *custompic;
	float picS1, picS2, picT1, picT2;
	vec4_t color;

	menuitem_private_t *items_headnode;

	struct uiwindow_s *nextRegistered;
	struct uiwindow_s *nextActive;

} uiwindow_t;

extern void UI_InitWindows( void );
extern void UI_FreeWindows( void );
extern uiwindow_t *UI_FindWindow( const char *name );
extern uiwindow_t *UI_RegisterWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic );
extern void UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic );
extern void UI_DrawActiveWindows( uiwindow_t *focus );
extern uiwindow_t *UI_FindWindowsFocus( void );
extern qboolean UI_IsTopWindow( uiwindow_t *focus );

extern void UI_OpenWindow( const char *name );
extern void UI_CloseWindow( const char *name );
extern void UI_ApplyWindowChanges( const char *name );
extern menuitem_private_t *UI_Window_CreateItem( const char *windowname, const char *itemname );

extern void UI_Window_ClickEvent( uiwindow_t *window, qboolean down );

#endif // __UI_WINDOWS_H__
