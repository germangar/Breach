/*
 */

#include "ui_local.h"

/*
* VIRTUAL WINDOW
*
* The virtual window is always 3x4 and of 800x600 size no matter
* the resolution.
*/

static int ui_window_x;
static int ui_window_y;
static int ui_window_width;
static int ui_window_height;

#define UI_SCALED_WIDTH( w ) ( (float)w * ( (float)ui_window_width / (float)UI_VIRTUALSCR_WIDTH ) )
#define UI_SCALED_HEIGHT( h ) ( (float)h * ( (float)ui_window_height / (float)UI_VIRTUALSCR_HEIGHT ) )
#define UI_SCALED_X( x ) ( ui_window_x + ( (float)x * ( (float)ui_window_width / (float)UI_VIRTUALSCR_WIDTH ) ) )
#define UI_SCALED_Y( y ) ( ui_window_y + ( (float)y * ( (float)ui_window_height / (float)UI_VIRTUALSCR_HEIGHT ) ) )

void UI_SetUpVirtualWindow( void )
{
	int width, height, x, y;

	width = uis.vidWidth;
	height = uis.vidHeight;
	x = 0;
	y = 0;

	// these are the rules: If the custom window is wider than
	// taller, we lock the virtual window inside it in a 3x4 proportion

	if( width * ( 3.0f/4.0f ) >= ( height - 1 ) )
	{
		ui_window_height = height;
		ui_window_width = height * ( 4.0f/3.0f );
		ui_window_x = ( x + ( width * 0.5 ) ) - ( ui_window_width * 0.5 );
		ui_window_y = y;
	}
	else // otherwise, let the HUD be stretched in vertical
	{
		ui_window_height = height;
		ui_window_width = width;
		ui_window_x = x;
		ui_window_y = y;
	}
}

int UI_VirtualWidth( int realwidth )
{
	UI_SetUpVirtualWindow();
	return 1 + Q_rint( (float)realwidth * ( (float)UI_VIRTUALSCR_WIDTH / (float)ui_window_width ) );
}

int UI_VirtualHeight( int realheight )
{
	UI_SetUpVirtualWindow();
	return 1 + Q_rint( (float)realheight * ( (float)UI_VIRTUALSCR_HEIGHT / (float)ui_window_height ) );
}

float *UI_WindowColor( uiwindow_t *window, vec4_t color )
{
	static vec4_t masked_color;
	static vec4_t backColorMask = { 0.9f, 0.9f, 0.9f, 1.0f };
	int i;

	if( !UI_IsTopWindow( window ) )
	{
		for( i = 0; i < 4; i++ ) 
			masked_color[i] = color[i] * backColorMask[i];
	}
	else
	{
		Vector4Copy( color, masked_color );
	}

	return masked_color;
}

/*
*  STRINGS DRAWING
*
*/

int UI_StringWidth( char *s, struct mufont_s *font )
{
	if( !font )
		font = uis.fontSystemSmall;
	return trap_SCR_strWidth( s, font, 0 );
}

int UI_StringHeight( struct mufont_s *font )
{
	if( !font )
		font = uis.fontSystemSmall;
	return trap_SCR_strHeight( font );
}

int UI_HorizontalAlign( int align, int width )
{
	int nx = 0;

	if( align % 3 == 0 )  // left
		nx = 0;
	if( align % 3 == 1 )  // center
		nx = -( width / 2 );
	if( align % 3 == 2 )  // right
		nx = -width;

	return nx;
}

int UI_VerticalAlign( int align, int height )
{
	int ny = 0;

	if( align / 3 == 0 )  // top
		ny = 0;
	else if( align / 3 == 1 )  // middle
		ny = -( height / 2 );
	else if( align / 3 == 2 )  // bottom
		ny = -height;

	return ny;
}

void UI_DrawClampString( int x, int y, int align, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	if( !str || !str[0] )
		return;
	if( !font )
		font = uis.fontSystemSmall;

	x = UI_SCALED_X( x );
	y = UI_SCALED_Y( y );

	x += UI_HorizontalAlign( align, trap_SCR_strWidth( str, font, 0) );
	y += UI_VerticalAlign( align, trap_SCR_strHeight( font ) );

	trap_SCR_DrawClampString( x, y, str, UI_SCALED_X( xmin ), UI_SCALED_Y( ymin ), UI_SCALED_X( xmax ), UI_SCALED_Y( ymax ), font, color );
}

void UI_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	if( !str || !str[0] )
		return;
	if( !font )
		font = uis.fontSystemSmall;

	if( maxwidth > 0 )
		trap_SCR_DrawStringWidth( UI_SCALED_X(x), UI_SCALED_Y(y), align, str, UI_SCALED_WIDTH(maxwidth), font, color );
	else
		trap_SCR_DrawString( UI_SCALED_X(x), UI_SCALED_Y(y), align, str, font, color );
}

/*
*  PICTURES DRAWING
*
*/

void UI_DrawClampPicRealScreen( int x, int y, int w, int h, int xmin, int ymin, int xmax, int ymax, vec4_t color, struct shader_s *shader )
{
	int offset, sx, sy, sw, sh;
	float pixelwidth, pixelheight;
	float s1, s2, t1, t2;

	if( x + w < xmin || x > xmax )
		return;
	if( y + h < ymin || y > ymax )
		return;

	s1 = 0.0f;
	s2 = 1.0f;
	t1 = 0.0f;
	t2 = 1.0f;
	sx = x;
	sy = y;
	sw = w;
	sh = h;

	pixelwidth = 1.0f / w;
	pixelheight = 1.0f / h;

	// clamp xmin
	if( x <= xmin && x + w >= xmin )
	{
		offset = xmin - x;
		if( offset )
		{
			sx += offset;
			sw -= offset;
			s1 += ( pixelwidth * offset );
		}
	}

	// clamp ymin
	if( y <= ymin && y + h >= ymin )
	{
		offset = ymin - y;
		if( offset )
		{
			sy += offset;
			sh -= offset;
			t1 += ( pixelheight * offset );
		}
	}

	// clamp xmax
	if( x <= xmax && x + w >= xmax )
	{
		offset = ( x + w ) - xmax;
		if( offset != 0 )
		{
			sw -= offset;
			s2 -= ( pixelwidth * offset );
		}
	}

	// clamp ymax
	if( y <= ymax && y + h >= ymax )
	{
		offset = ( y + h ) - ymax;
		if( offset != 0 )
		{
			sh -= offset;
			t2 -= ( pixelheight * offset );
		}
	}

	if( sw || sh )
		trap_R_DrawStretchPic( sx, sy, sw, sh, s1, t1, s2, t2, color, shader );
}

void UI_DrawClampPic( int x, int y, int w, int h, int align, int xmin, int ymin, int xmax, int ymax, vec4_t color, struct shader_s *shader )
{
	x = UI_SCALED_X(x);
	y = UI_SCALED_Y(y);
	w = UI_SCALED_WIDTH(w);
	h = UI_SCALED_HEIGHT(h);
	x += UI_HorizontalAlign( align, w );
	y += UI_VerticalAlign( align, h );
	xmin = UI_SCALED_X( xmin );
	xmax = UI_SCALED_X( xmax );
	ymin = UI_SCALED_Y( ymin );
	ymax = UI_SCALED_Y( ymax );
	UI_DrawClampPicRealScreen( x, y, w, h, xmin, ymin, xmax, ymax, color, shader );
}

void UI_DrawStretchPic( int x, int y, int w, int h, int align, float s1, float t1, float s2, float t2, vec4_t color, struct shader_s *shader )
{
	x += UI_HorizontalAlign( align, w );
	y += UI_VerticalAlign( align, h );

	trap_R_DrawStretchPic( UI_SCALED_X(x), UI_SCALED_Y(y), UI_SCALED_WIDTH(w), UI_SCALED_HEIGHT(h), s1, t1, s2, t2, color, shader );
}

void UI_FillRect( int x, int y, int w, int h, vec4_t color )
{
	UI_DrawStretchPic( x, y, w, h, ALIGN_LEFT_TOP, 0, 0, 1, 1, color, uis.whiteShader );
}

void UI_DrawDefaultItemBox( int x, int y, int w, int h, int align, vec4_t color, qboolean focus )
{
	struct shader_s *shader;

	shader = uis.itemShader;
	if( focus )
		shader = uis.itemFocusShader;

	UI_DrawStretchPic( x, y, w, h, align, 0, 0, 1, 1, color, shader );
}

void UI_DrawDefaultListBox( int x, int y, int w, int h, int align, vec4_t color, qboolean focus )
{
#define BLOCKSIZE 8
	int realX, realY, realW, realH, start;

	x += UI_HorizontalAlign( align, w );
	y += UI_VerticalAlign( align, h );

	// convert to real window coordinates
	realX = UI_SCALED_X( x );
	realY = UI_SCALED_Y( y );
	realW = UI_SCALED_WIDTH( w );
	realH = UI_SCALED_HEIGHT( h );

	trap_R_DrawStretchPic( realX, realY, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_up_left );
	trap_R_DrawStretchPic( realX + realW - BLOCKSIZE, realY, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_up_right );
	
	trap_R_DrawStretchPic( realX, realY + realH - BLOCKSIZE, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_down_left );
	trap_R_DrawStretchPic( realX + realW - BLOCKSIZE, realY + realH - BLOCKSIZE, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_down_right );

	trap_R_DrawStretchPic( realX + BLOCKSIZE, realY + BLOCKSIZE, realW - ( 2 * BLOCKSIZE ), realH - ( 2 * BLOCKSIZE ), 0,0,1,1, color, uis.listbox_back );

	for( start = realX + BLOCKSIZE; start < realX + realW - BLOCKSIZE; start += BLOCKSIZE )
	{
		trap_R_DrawStretchPic( start, realY, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_up );
		trap_R_DrawStretchPic( start, realY + realH - BLOCKSIZE, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_down );
	}

	for( start = realY + BLOCKSIZE; start < realY + realH - BLOCKSIZE; start += BLOCKSIZE )
	{
		trap_R_DrawStretchPic( realX, start, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_left );
		trap_R_DrawStretchPic( realX + realW - BLOCKSIZE, start, BLOCKSIZE, BLOCKSIZE, 0,0,1,1, color, uis.listbox_right );
	}

#undef BLOCKSIZE
}

/*
*  MODELS DRAWING
*
*/

void UI_DrawModelRealScreen( int x, int y, int w, int h, float fov, struct model_s *model, int frame, int oldframe, float lerpfrac, vec3_t angles )
{
	refdef_t refdef;
	int i;
	float radius = 0.0f;
	vec3_t origin, offset, axis[3];
	entity_t ent;
	vec3_t mins, maxs;

	if( !model )
		return;

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;
	refdef.fov_x = fov;
	refdef.fov_y = CalcFov( refdef.fov_x, w, h );
	refdef.time = uis.time;
	refdef.rdflags = RDF_NOWORLDMODEL;
	Matrix_Copy( axis_identity, refdef.viewaxis );

	trap_R_ClearScene();

	trap_R_ModelBounds( model, mins, maxs );
	radius = 0.5f + ( 0.5 * RadiusFromBounds( mins, maxs ) );

	VectorClear( origin );
	AnglesToAxis( angles, axis );
	for( i = 0; i < 3; i++ )
	{
		offset[i] = 0.5 * ( maxs[i] + mins[i] );
		VectorMA( origin, -offset[i], axis[i], origin );
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.rtype = RT_MODEL;
	Vector4Set( ent.shaderRGBA, 255, 255, 255, 255 );
	ent.scale = 1.0f;
	ent.renderfx = RF_FULLBRIGHT | RF_NOSHADOW | RF_FORCENOLOD;
	ent.frame = frame;
	ent.oldframe = oldframe;
	clamp( lerpfrac, 0.0f, 1.0f );
	ent.backlerp = 1.0f - lerpfrac;
	ent.model = model;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.origin2 );
	VectorCopy( origin, ent.lightingOrigin );
	Matrix_Copy( axis, ent.axis );

	trap_R_AddEntityToScene( &ent );

	VectorMA( refdef.vieworg, -radius * ( 1.0 / 0.179 ), axis_identity[0], refdef.vieworg );

	trap_R_RenderScene( &refdef );
}

void UI_DrawModel( int x, int y, int w, int h, int align, struct model_s *model, int frame, vec3_t angles )
{
	if( !model )
		return;

	UI_SetUpVirtualWindow();

	x += UI_HorizontalAlign( align, w );
	y += UI_VerticalAlign( align, h );

	// convert to real screen
	x = UI_SCALED_X( x );
	y = UI_SCALED_Y( y );
	w = UI_SCALED_WIDTH( w );
	h = UI_SCALED_HEIGHT( h );

	UI_DrawModelRealScreen( x, y, w, h, 30, model, frame, frame, 0, angles );
}

/*
* tools
*
*/
char *UI_ListNameForPosition( const char *namesList, int position )
{
	static char buf[MAX_STRING_CHARS];
	char separator = CHAR_SEPARATOR;
	const char *s, *t;
	char *b;
	int count, len;

	if( !namesList )
		return NULL;

	// set up the tittle from the spinner names
	s = namesList;
	t = s;
	count = 0;
	buf[0] = 0;
	b = buf;
	while( *s && ( s = strchr( s, separator ) ) )
	{
		if( count == position )
		{
			len = s - t;
			if( len <= 0 )
				UI_Error( "G_NameInStringList: empty name in list\n" );
			if( len > MAX_STRING_CHARS - 1 )
				UI_Printf( "WARNING: G_NameInStringList: name is too long\n" );
			while( t <= s )
			{
				if( *t == separator || t == s )
				{
					*b = 0;
					break;
				}

				*b = *t;
				t++;
				b++;
			}

			break;
		}

		count++;
		s++;
		t = s;
	}

	if( buf[0] == 0 )
		return NULL;

	return buf;
}

char *UI_AllocCreateNamesList( const char *path, const char *extension )
{
	char separator[2];
	char name[MAX_CONFIGSTRING_CHARS];
	char buffer[MAX_STRING_CHARS], *s, *list;
	int nummaps, i, j, found, length, fulllength;

	if( !extension || !path )
		return NULL;

	if( extension[0] != '.' || strlen( extension ) < 2 )
		return NULL;

	if( ( nummaps = trap_FS_GetFileList( path, extension, NULL, 0, 0, 0 ) ) == 0 ) 
	{
		return NULL;
	}

	separator[0] = CHAR_SEPARATOR;
	separator[1] = 0;

	/*
	* do a first pass just for finding the full len of the list
	*/

	i = 0;
	found = 0;
	length = 0;
	fulllength = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, nummaps ) ) == 0 ) 
		{
            // can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
			{
				Com_Printf( "Warning: UI_GenerateNamesList :file name too long: %s\n", s );
				continue;
			}

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			fulllength += strlen( name ) + 1;
            found++;
		}
	} while( i < nummaps );

	if( !found )
		return NULL;

	/*
	* Allocate a string for the full list and do a second pass to copy them in there
	*/

	fulllength += 1;
	list = UI_Malloc( fulllength );

	i = 0;
	length = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, nummaps ) ) == 0 ) 
		{
            // can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
			{
				continue;
			}

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			Q_strncatz( list, name, fulllength );
			Q_strncatz( list, separator, fulllength );
		}
	} while( i < nummaps );

	return list;
}

/*
* Creates a list with the available map names and keeps it in a cvar
*
*/
char *UI_RefreshMapListCvar( void )
{
	cvar_t *ui_maplist;
	char *list;

	ui_maplist = trap_Cvar_Get( "ui_maplist", "", CVAR_NOSET );

	list = UI_AllocCreateNamesList( "maps", ".bsp" );
	if( !list )
	{
		trap_Cvar_ForceSet( "ui_maplist", "" );
		return NULL;
	}

	if( Q_stricmp( ui_maplist->string, list ) )
		trap_Cvar_ForceSet( "ui_maplist", list );
	UI_Free( list );

	return ui_maplist->string;
}

/*
* Creates a list with the available map names and keeps it in a cvar
*
*/
char *UI_RefreshDemoListCvar( void )
{
	cvar_t *ui_demolist;
	char *list;

	ui_demolist = trap_Cvar_Get( "ui_demolist", "", CVAR_NOSET );

	list = UI_AllocCreateNamesList( "demos", APP_DEMO_EXTENSION_STR );
	if( !list )
	{
		trap_Cvar_ForceSet( "ui_demolist", "" );
		return NULL;
	}

	if( Q_stricmp( ui_demolist->string, list ) )
		trap_Cvar_ForceSet( "ui_demolist", list );
	UI_Free( list );

	return ui_demolist->string;
}



/*
* scroll lists management
*
*/

void UI_FreeScrollItemList( m_itemslisthead_t *itemlist )
{
	m_listitem_t *ptr;

	while( itemlist->headNode )
	{
		ptr = itemlist->headNode;
		itemlist->headNode = ptr->pnext;
		UI_Free( ptr );
	}

	itemlist->headNode = NULL;
	itemlist->numItems = 0;
}

m_listitem_t *UI_FindItemInScrollListWithId( m_itemslisthead_t *itemlist, int itemid )
{
	m_listitem_t *item;

	if( !itemlist->headNode )
		return NULL;

	item = itemlist->headNode;
	while( item )
	{
		if( item->id == itemid )
			return item;
		item = item->pnext;
	}

	return NULL;
}

void UI_AddItemToScrollList( m_itemslisthead_t *itemlist, char *name, void *data )
{
	m_listitem_t *newitem, *checkitem;

	//check for the address being already in the list
	checkitem = itemlist->headNode;
	while( checkitem )
	{
		if( !Q_stricmp( name, checkitem->name ) )
			return;
		checkitem = checkitem->pnext;
	}

	newitem = (m_listitem_t *)UI_Malloc( sizeof( m_listitem_t ) );
	Q_strncpyz( newitem->name, name, sizeof( newitem->name ) );
	newitem->pnext = itemlist->headNode;
	itemlist->headNode = newitem;
	newitem->id = itemlist->numItems;
	newitem->data = data;

	// update item names array
	itemlist->item_names[itemlist->numItems] = UI_CopyString( newitem->name );
	itemlist->item_names[itemlist->numItems+1] = NULL;

	itemlist->numItems++;
}
