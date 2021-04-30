/*
 */

#ifndef __UI_ITEMS_H__
#define __UI_ITEMS_H__

#define ITEM_MAX_NAME_SIZE		32

class cMenuItem
{
public:
	char name[ITEM_MAX_NAME_SIZE];
	float scroll_value; // listboxes only
	unsigned int timeStamp;
	vec4_t currentColor;

	menuitem_public_t pub;

	// not a method
	void ( *CustomClickEvent )( class cMenuItem *item, qboolean down );
	void ( *CustomDraw )( class cMenuItem *item );

	void ( *CustomAction )( struct menuitem_public_s *item );
	void ( *CustomRefresh )( struct menuitem_public_s *item );
	void ( *CustomUpdate )( struct menuitem_public_s*item );
	void ( *CustomApplyChanges )( struct menuitem_public_s *item );

	class cWindow *parentWindow;
	class cMenuItem *next;

	cMenuItem();
	~cMenuItem();

	void Draw( void );
	void ClickEvent( qboolean down );

	void Action( void );
	void Refresh( void );
	void Update( void );
	void ApplyChanges( void );

	
void ClampCoords( void );
	float *CurrentColor( void );
	float *CurrentTextColor( void );
private:
};

/*
typedef struct menuitem_private_s
{
	char name[ITEM_MAX_NAME_SIZE];

	menuitem_public_t pub;

	float scroll_value; // listboxes only

	void ( *Draw )( struct menuitem_private_s *item );
	void ( *ClickEvent )( struct menuitem_private_s *item, qboolean down );

	void ( *Action )( struct menuitem_public_s *item );
	void ( *Refresh )( struct menuitem_public_s *item );
	void ( *Update )( struct menuitem_public_s *item );
	void ( *ApplyChanges )( struct menuitem_public_s *item );

	unsigned int timeStamp;

	//char *listNames; // spinners, listboxes...

	class cWindow *parentWindow;

	struct menuitem_private_s *next;
} menuitem_private_t;
*/

class cMenuItemManager
{
public:
	class cMenuItem *focusedItem;
	class cMenuItem *dragItem;

	cMenuItemManager() {
		Init();
	};

	void Init( void )
	{
		ShutDown();
	}

	void ShutDown( void )
	{
		focusedItem = NULL;
		dragItem = NULL;
	}

	void ClickEvent( class cMenuItem *menuItem, qboolean down );
	void UpdateFocusedItem( void );
	void RecurseFreeItems( class cMenuItem *item );
	class cMenuItem *RegisterItem( const char *name, class cMenuItem **headnode );
	
	void RecurseDrawItems( class cMenuItem *menuItem );
private:
	class cMenuItem *RecurseFindItemFocus( class cMenuItem *item );
};

extern cMenuItemManager guiItemManager;

// exported to the client
extern struct menuitem_public_s *UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align );
extern struct menuitem_public_s *UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font );
extern struct menuitem_public_s *UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString );
extern struct menuitem_public_s *UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void *Update, void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void *Update, void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void *Update, void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void *Update, void *Apply, qboolean alwaysApply, void *Action, char *targetString );

#endif // __UI_ITEMS_H__


