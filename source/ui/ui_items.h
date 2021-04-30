/*
 */

#ifndef __UI_ITEMS_H__
#define __UI_ITEMS_H__

#define ITEM_MAX_NAME_SIZE		32

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

	struct uiwindow_s *window;

	struct menuitem_private_s *next;
} menuitem_private_t;

// local
extern menuitem_private_t *UI_RegisterItem( const char *name, menuitem_private_t **headnode );
extern void UI_RecurseFreeItem( menuitem_private_t *item );
extern void UI_DrawItemsArray( menuitem_private_t *item );
extern menuitem_private_t *UI_RecurseFindItemFocus( menuitem_private_t *item );
extern void UI_Item_ClickEvent( menuitem_private_t *item, qboolean down );

// exported to the client
extern struct menuitem_public_s *UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align );
extern struct menuitem_public_s *UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font );
extern struct menuitem_public_s *UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString );
extern struct menuitem_public_s *UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString );
extern struct menuitem_public_s *UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, void *Action, char *targetString );

#endif // __UI_ITEMS_H__


