/*
 */

#ifndef __UI_ATOMS_H__
#define __UI_ATOMS_H__

/*
* scroll lists management
*
*/

typedef struct m_listitem_s
{
	char name[MAX_QPATH];
	struct m_listitem_s *pnext;
	int id;
	void *data;

} m_listitem_t;

typedef struct
{
	m_listitem_t *headNode;
	int numItems;
	char *item_names[32000];        //fixme
} m_itemslisthead_t;

void UI_FreeScrollItemList( m_itemslisthead_t *itemlist );
m_listitem_t *UI_FindItemInScrollListWithId( m_itemslisthead_t *itemlist, int itemid );
void UI_AddItemToScrollList( m_itemslisthead_t *itemlist, char *name, void *data );

/*
*
*
*/

extern char *UI_ListNameForPosition( const char *namesList, int position );
extern char *UI_RefreshMapListCvar( void );
extern char *UI_RefreshDemoListCvar( void );
/*
*
*
*/

extern void UI_SetUpVirtualWindow( void );
extern int UI_VirtualWidth( int realwidth );
extern int UI_VirtualHeight( int realheight );
extern float *UI_WindowColor( uiwindow_t *window, vec4_t color );
extern int UI_StringWidth( char *s, struct mufont_s *font );
extern int UI_StringHeight( struct mufont_s *font );
extern int UI_HorizontalAlign( int align, int width );
extern int UI_VerticalAlign( int align, int height );
extern void UI_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color );
extern void UI_DrawClampString( int x, int y, int align, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color );
extern void UI_DrawClampPic( int x, int y, int w, int h, int align, int xmin, int ymin, int xmax, int ymax, vec4_t color, struct shader_s *shader );
extern void UI_DrawStretchPic( int x, int y, int w, int h, int align, float s1, float t1, float s2, float t2, vec4_t color, struct shader_s *shader );
extern void UI_DrawDefaultItemBox( int x, int y, int w, int h, int align, vec4_t color, qboolean focus );
extern void UI_DrawDefaultListBox( int x, int y, int w, int h, int align, vec4_t color, qboolean focus );
extern void UI_DrawModelRealScreen( int x, int y, int w, int h, float fov, struct model_s *model, int frame, int oldframe, float lerpfrac, vec3_t angles );
extern void UI_DrawModel( int x, int y, int w, int h, int align, struct model_s *model, int frame, vec3_t angles );

#endif // __UI_ATOMS_H__
