/*
 */

#ifndef __GUI_WINDOWS_H__
#define __GUI_WINDOWS_H__

#define WINDOW_NAME_SIZE	32

class cWindow
{
public:
	char name[WINDOW_NAME_SIZE];
	int x, y, w, h;
	qboolean mergeablefocus;	// allows its items to be used when on focus even if not being the top window
	qboolean blockdraw; // skip drawing windows below this one (and blocks focus too)
	qboolean blockfocus; // disallow selecting anything below this one

	struct shader_s *custompic;
	float picS1, picS2, picT1, picT2;
	vec4_t color;
	vec4_t currentColor;

//	menuitem_private_t *items_headnode;
	class cMenuItem *menuItemsHeadnode;

	cWindow *nextRegistered;
	cWindow *nextActive;

	cWindow();
	~cWindow();

	void Activate( void );
	qboolean Deactivate( void );
	void Push( void );
	void ClickEvent( qboolean down );
	qboolean IsTopWindow( void );
	void ApplyChanges( void );
	void Draw( void );
	//float *CurrentColor( void );

//	menuitem_private_t *CreateItem( const char *itemname );
	class cMenuItem *cWindow::CreateMenuItem( const char *itemname );

private:
	float *CurrentColor( void );
};

class cWindowManager
{
public:
	class cWindow *rootWindow;
	class cWindow *focusedWindow;
	class cWindow *activeWindows;

	cWindowManager() {
		Init();
	};

	void Init( void )
	{
		ShutDown();
	}

	void ShutDown( void )
	{
		FreeWindows();
		rootWindow = NULL;
		focusedWindow = NULL;
		activeWindows = NULL;
	}

	void OpenWindow( const char *name );
	void CloseWindow( const char *name );
	void DrawActiveWindows( void );
	void UpdateFocusedWindow( void );
	class cWindow *FindWindowByName( const char *name );
	class cWindow *RegisterWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic );
	void FreeWindows( void );

	class cMenuItem *CreateMenuItem( const char *windowname, const char *itemname );
	
private:
	void RecurseDrawActiveWindows( class cWindow *window );
	class cWindow *RecurseFindWindowsFocus( class cWindow *window );
	void RecurseFreeWindow( class cWindow **win );
};

extern cWindowManager guiWindowManager;

extern void UI_InitWindow( const char *name, int x, int y, int w, int h, qboolean blockdraw, qboolean blockfocus, struct shader_s *custompic );
extern void UI_OpenWindow( const char *name );
extern void UI_CloseWindow( const char *name );
extern void UI_ApplyWindowChanges( const char *name );

#endif // __GUI_WINDOWS_H__
