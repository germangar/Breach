/*
 */

#include "ui_local.h"

#define ITEM_DEFAULT_WIDTH		64
#define ITEM_DEFAULT_HEIGHT		32

#define TEXT_MARGIN 4
#define ITEM_DIRECTION_CONTROL_SIZE 16

#define ITEM_DOUBLE_CLICK_MAX_TIME	400

static int rX;
static int rY;
static int rW;
static int rH;

static void UI_Item_ClampCoords( menuitem_private_t *item )
{
	int aX, aY;

	if( item )
	{
		// first clamp the item sizes inside the window ones
		assert( item->window );

		if( item->pub.w > item->window->w )
			item->pub.w = item->window->w;

		if( item->pub.h > item->window->h )
			item->pub.h = item->window->h;

		aX = item->pub.x + UI_HorizontalAlign( item->pub.align, item->pub.w );
		aY = item->pub.y + UI_VerticalAlign( item->pub.align, item->pub.h );

		if( aX < 0 ) 
		{
			item->pub.x += -( aX );
		}
		else if( aX > item->window->w )
		{
			item->pub.x -= aX - item->window->w;
		}

		aX = item->pub.x + UI_HorizontalAlign( item->pub.align, item->pub.w );
		if( aX + item->pub.w > item->window->w )
			item->pub.w = item->window->w - aX;

		if( aY < 0 ) 
		{
			item->pub.y += -( aY );
		}
		else if( aY > item->window->h )
		{
			item->pub.h -= aY - item->window->h;
		}

		aY = item->pub.y + UI_HorizontalAlign( item->pub.align, item->pub.h );
		if( aY + item->pub.h > item->window->h )
			item->pub.h = item->window->h - aY;


		// update the final coordinates used for drawing and finding focus
		rX = item->window->x + item->pub.x;
		rY = item->window->y + item->pub.y;
		rW = item->pub.w;
		rH = item->pub.h;
	}
}



/*
* ITEM_STATIC
* not interactive object.
*/

static void UI_GenericItem_Draw( menuitem_private_t *item )
{
	if( item == uis.itemFocus )
	{
		if( item->pub.custompicfocus )
		{
			UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
				UI_WindowColor( item->window, item->pub.color ), item->pub.custompicfocus );
		}
		else if( item->ClickEvent )
		{
			UI_DrawDefaultItemBox( rX, rY, rW, rH, item->pub.align, UI_WindowColor( item->window, item->pub.color ), qtrue );
		}
	}
	else
	{
		if( item->pub.custompic )
		{
			UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
				UI_WindowColor( item->window, item->pub.color ), item->pub.custompic );
		}
		else if( item->ClickEvent )
		{
			UI_DrawDefaultItemBox( rX, rY, rW, rH, item->pub.align, UI_WindowColor( item->window, item->pub.color ), qfalse );
		}
	}

	if( item->pub.custompicselected && item->pub.value )
		UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							UI_WindowColor( item->window, item->pub.color ), item->pub.custompicselected );

	if( item->pub.tittle ) 
	{
		int alignedX, alignedY, maxwidth, maxheight;

		// manually aligning 
		alignedX = rX + UI_HorizontalAlign( item->pub.align, rW );
		alignedY = rY + UI_VerticalAlign( item->pub.align, rH );

		// width and height without the margin
		maxwidth = rW - ( 2 * item->pub.marginW );
		maxheight = rH - ( 2 * item->pub.marginH );

		// x and y inside margin
		alignedX += item->pub.marginW;
		alignedY += item->pub.marginH;

		// Move to center, cause buttons and spinners look better having centered text
		alignedX += maxwidth * 0.5f;
		alignedY += maxheight * 0.5f;

		UI_DrawStringWidth( alignedX, alignedY, ALIGN_CENTER_MIDDLE, item->pub.tittle, maxwidth, item->pub.font, UI_WindowColor( item->window, item->pub.colorText ) );


		/*
		// MARGIN WITH ALIGNED ITEM KEEPING TEXT ALIGN
		// KEEP FOR TEXT BOXES (TODO)

		int xd, yd, maxwidth, maxheight;

		// convert the square to the one inside it removing the 
		// margin size, and do it so it respects align
		maxwidth = rW - ( 2 * item->pub.marginW );
		maxheight = rH - ( 2 * item->pub.marginH );
		xd = ( UI_HorizontalAlign( item->pub.align, 2 ) + 1 ) * item->pub.marginW;
		yd = ( UI_VerticalAlign( item->pub.align, 2 ) + 1 ) * item->pub.marginH;

		UI_DrawStringWidth( rX + xd, rY + yd, item->pub.align, item->pub.tittle, maxwidth, item->pub.font, UI_WindowColor( item->window, item->pub.colorText ) );
		*/
	}
}

static void UI_Items_InitStatic( menuitem_private_t *item, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	if( item )
	{
		item->Draw = UI_GenericItem_Draw;

		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;
		item->pub.marginW = UI_VirtualWidth( TEXT_MARGIN );
		item->pub.marginH = UI_VirtualHeight( TEXT_MARGIN );

		// statics are initialized as text
		item->pub.font = font ? font : uis.fontSystemMedium;

		if( tittle && strlen( tittle ) )
		{
			Q_strncpyz( item->pub.tittle, tittle, sizeof( item->pub.tittle ) );
			item->pub.w = ( item->pub.marginW * 2 ) + UI_VirtualWidth( trap_SCR_strWidth( item->pub.tittle, item->pub.font, 0 ) );
			item->pub.h = ( item->pub.marginH * 2 ) + UI_VirtualHeight( trap_SCR_strHeight( item->pub.font ) );
		}
		else
		{
			item->pub.tittle[0] = 0;
			item->pub.w = ITEM_DEFAULT_WIDTH;
			item->pub.h = ITEM_DEFAULT_HEIGHT;
		}

		// clamp button to window size
		UI_Item_ClampCoords( item );
	}
}

struct menuitem_public_s *UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitStatic( item, x, y, align, tittle, font );
	}

	return &item->pub;
}

/*
* ITEM_WINDOWDRAGGER
* Dragging this item drags the window it's in
*/

#define ITEM_WINDOWDRAGGER_DEFAULT_WIDTH 64
#define ITEM_WINDOWDRAGGER_DEFAULT_HEIGHT 16

static void UI_WindowDragger_ClickEvent( menuitem_private_t *item, qboolean down )
{
	if( down )
	{
		if( !item->pub.minvalue && !item->pub.maxvalue )
		{
			// set up for dragging the window
			item->timeStamp = uis.time;
			item->pub.minvalue = uis.cursorX;
			item->pub.maxvalue = uis.cursorY;
		}
		else if( uis.time > item->timeStamp + 100 )
		{
			item->window->x += uis.cursorX - item->pub.minvalue;
			item->window->y += uis.cursorY - item->pub.maxvalue;
			item->pub.minvalue = uis.cursorX;
			item->pub.maxvalue = uis.cursorY;
		}
	}
	else
	{
		item->pub.minvalue = 0;
		item->pub.maxvalue = 0;
		if( item->Refresh )
			item->Refresh( &item->pub );
	}
}

static void UI_Items_InitWindowDragger( menuitem_private_t *item, int x, int y, int align )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->Draw = UI_GenericItem_Draw;
		item->ClickEvent = UI_WindowDragger_ClickEvent;

		// WindowDraggers don't use any text (at least by now)
		item->pub.font = NULL;
		item->pub.tittle[0] = 0;

		item->pub.w = ITEM_WINDOWDRAGGER_DEFAULT_WIDTH;
		item->pub.h = ITEM_WINDOWDRAGGER_DEFAULT_HEIGHT;

		// clamp button to window size
		UI_Item_ClampCoords( item );
	}
}

struct menuitem_public_s *UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemWindowDragger : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitWindowDragger( item, x, y, align );
	}

	return &item->pub;
}

/*
* ITEM_BUTTON
* Clicking on this item executes an action
*/

static void UI_Button_ClickEvent( menuitem_private_t *item, qboolean down )
{
	if( !down )
	{
		if( item->Action )
			item->Action( &item->pub );
	}
}

static void UI_Items_InitButton( menuitem_private_t *item, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	if( item )
	{
		item->Draw = UI_GenericItem_Draw;
		item->ClickEvent = UI_Button_ClickEvent;
		item->Action = Action;

		if( targetString && targetString[0] )
			item->pub.targetString = UI_CopyString( targetString );

		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;
		item->pub.marginW = UI_VirtualWidth( TEXT_MARGIN );
		item->pub.marginH = UI_VirtualHeight( TEXT_MARGIN );

		item->pub.font = font ? font : uis.fontSystemMedium;

		if( tittle && strlen( tittle ) )
		{
			Q_strncpyz( item->pub.tittle, tittle, sizeof( item->pub.tittle ) );
			item->pub.w = ( item->pub.marginW * 2 ) + UI_VirtualWidth( trap_SCR_strWidth( item->pub.tittle, item->pub.font, 0 ) );
			item->pub.h = ( item->pub.marginH * 2 ) + UI_VirtualHeight( trap_SCR_strHeight( item->pub.font ) );
		}
		else
		{
			item->pub.tittle[0] = 0;
			item->pub.w = ITEM_DEFAULT_WIDTH;
			item->pub.h = ITEM_DEFAULT_HEIGHT;
		}

		// clamp button to window size
		UI_Item_ClampCoords( item );
	}
}

struct menuitem_public_s *UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemButton : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitButton( item, x, y, align, tittle, font, Action, targetString );
	}

	return &item->pub;
}

/*
* ITEM_CHECKBOX
* Clicking on this item toggles it's value on and off
*/

#define ITEM_CHECKBOX_DEFAULT_WIDTH 16
#define ITEM_CHECKBOX_DEFAULT_HEIGHT 16

static void UI_CheckBox_ClickEvent( menuitem_private_t *item, qboolean down )
{
	if( !down )
	{
		item->pub.value = !item->pub.value;
		if( item->Refresh )
			item->Refresh( &item->pub );
	}
}

static void UI_Items_InitCheckBox( menuitem_private_t *item, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, char *targetString )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->Draw = UI_GenericItem_Draw;
		item->ClickEvent = UI_CheckBox_ClickEvent;
		item->ApplyChanges = Apply;
		item->Update = Update;

		if( targetString && targetString[0] )
			item->pub.targetString = UI_CopyString( targetString );

		item->pub.custompic = uis.itemCheckBoxShader;
		item->pub.custompicfocus = uis.itemCheckBoxFocusShader;
		item->pub.custompicselected = uis.itemCheckBoxSelectedShader;

		// Checkboxes don't use any text
		item->pub.font = NULL;
		item->pub.tittle[0] = 0;

		item->pub.w = ITEM_CHECKBOX_DEFAULT_WIDTH;
		item->pub.h = ITEM_CHECKBOX_DEFAULT_HEIGHT;

		// clamp button to window size
		UI_Item_ClampCoords( item );

		if( item->Update )
			item->Update( &item->pub );
	}
}

struct menuitem_public_s *UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemCheckBox : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitCheckBox( item, x, y, align, Update, Apply, targetString );
		if( alwaysApply )
			item->Refresh = item->ApplyChanges;
	}

	return &item->pub;
}

/*
* ITEM_SPINNER
* Clicking on this item moves into the next option
*/

static void UI_Spinner_Draw( menuitem_private_t *item )
{
	char *s;
	int alignedX, alignedY, innerWidth, start, end;

	// manually aligning makes easier drawing the multiple parts
	alignedX = rX + UI_HorizontalAlign( item->pub.align, rW );
	alignedY = rY + UI_VerticalAlign( item->pub.align, rH );

	start = alignedX + ITEM_DIRECTION_CONTROL_SIZE;
	end = alignedX + rW - ITEM_DIRECTION_CONTROL_SIZE;
	innerWidth = rW - (2 * ITEM_DIRECTION_CONTROL_SIZE);

	if( item == uis.itemFocus )
	{
		if( item->pub.custompicfocus )
		{
			UI_DrawStretchPic( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							UI_WindowColor( item->window, item->pub.color ), item->pub.custompicfocus );
		}
		else
		{
			UI_DrawDefaultItemBox( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, UI_WindowColor( item->window, item->pub.color ), qtrue );
		}
	}
	else
	{
		if( item->pub.custompic )
		{
			UI_DrawStretchPic( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							UI_WindowColor( item->window, item->pub.color ), item->pub.custompic );
		}
		else
		{
			UI_DrawDefaultItemBox( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, UI_WindowColor( item->window, item->pub.color ), qfalse );
		}
	}

	// draw the controllers
	UI_DrawStretchPic( alignedX, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		UI_WindowColor( item->window, item->pub.color ), uis.itemSliderLeft );

	UI_DrawStretchPic( alignedX + rW - ITEM_DIRECTION_CONTROL_SIZE, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		UI_WindowColor( item->window, item->pub.color ), uis.itemSliderRight );

	s = UI_ListNameForPosition( item->pub.listNames, item->pub.value );

	if( s ) 
	{
		int innerX, innerY, maxwidth, maxheight;

		// width and height without the margin
		maxwidth = innerWidth - ( 2 * item->pub.marginW );
		maxheight = rH - ( 2 * item->pub.marginH );

		// x and y inside margin
		innerX = alignedX + ITEM_DIRECTION_CONTROL_SIZE + item->pub.marginW;
		innerY = alignedY + item->pub.marginH;

		// Move to center, cause buttons and spinners look better having centered text
		innerX += maxwidth * 0.5f;
		innerY += maxheight * 0.5f;

		UI_DrawStringWidth( innerX, innerY, ALIGN_CENTER_MIDDLE, s, maxwidth, item->pub.font, UI_WindowColor( item->window, item->pub.colorText ) );
	}
}

static void UI_Spinner_ClickEvent( menuitem_private_t *item, qboolean down )
{
	int alignedX, alignedY, start, end;

	UI_Item_ClampCoords( item );

	// manually aligning the slider
	alignedX = rX + UI_HorizontalAlign( item->pub.align, item->pub.w );
	alignedY = rY + UI_VerticalAlign( item->pub.align, item->pub.h );

	start = ( alignedX + ITEM_DIRECTION_CONTROL_SIZE );
	end = ( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE );

	if( uis.cursorX >= alignedX && uis.cursorX <= alignedX + ITEM_DIRECTION_CONTROL_SIZE )
	{
		// clicked left control
		if( down )
		{
			if( uis.time > item->timeStamp + item->pub.timeRepeat )
			{
				item->timeStamp = uis.time;
				item->pub.value -= item->pub.stepvalue;
			}
		}	
	}
	else if( uis.cursorX >= end && uis.cursorX <= end + ITEM_DIRECTION_CONTROL_SIZE )
	{
		// clicked right control
		if( down )
		{
			if( uis.time > item->timeStamp +  + item->pub.timeRepeat )
			{
				item->timeStamp = uis.time;
				item->pub.value += item->pub.stepvalue;
			}
		}
	}

	item->pub.value = (int)item->pub.value;
	clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );

	if( !down )
	{
		if( item->Refresh )
			item->Refresh( &item->pub );
	}
}

static void UI_Items_InitSpinner( menuitem_private_t *item, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, char *targetString )
{
	int maxwidth = 0, i;
	char *s;

	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;
		item->pub.marginW = UI_VirtualWidth( TEXT_MARGIN );
		item->pub.marginH = UI_VirtualHeight( TEXT_MARGIN );

		item->Draw = UI_Spinner_Draw;
		item->ClickEvent = UI_Spinner_ClickEvent;
		item->Update = Update;
		item->ApplyChanges = Apply;

		item->pub.font = font ? font : uis.fontSystemMedium;
		item->pub.minvalue = 0;
		item->pub.maxvalue = 0;
		item->pub.value = item->pub.minvalue;

		if( targetString && targetString[0] )
			item->pub.targetString = UI_CopyString( targetString );

		// count the amount of names to spin

		if( namesList )
		{
			if( namesList[ strlen(namesList) - 1 ] != CHAR_SEPARATOR )
			{
				size_t size;

				// we have to add the last separator
				size = strlen( namesList ) + 1 + sizeof( CHAR_SEPARATOR );
				item->pub.listNames = UI_Malloc( size );
				Q_snprintfz( item->pub.listNames, size, "%s%c", namesList, CHAR_SEPARATOR );
			}
			else
			{
				item->pub.listNames = UI_CopyString( namesList );
			}

			// it's safe to count end-line separators now
			for( item->pub.maxvalue = 0; ( s = UI_ListNameForPosition( item->pub.listNames, item->pub.maxvalue ) ) != NULL; item->pub.maxvalue++ )
			{
				i = trap_SCR_strWidth( s, item->pub.font, 0 );
				if( i > maxwidth )
					maxwidth = i;

				// check for never having empty names
				s++;
				if( *s == CHAR_SEPARATOR )
					UI_Error( "UI_Items_InitSpinner: two consecutive end-line separators used\n" );
			}
		}

		item->pub.tittle[0] = 0; // tittle is not used
		if( !item->pub.maxvalue )
		{
			item->pub.w = ( item->pub.marginW * 2 ) + ITEM_DEFAULT_WIDTH;
			item->pub.h = ( item->pub.marginH * 2 ) + ITEM_DEFAULT_HEIGHT;
		}
		else
		{
			item->pub.w = ( item->pub.marginW * 2 ) + UI_VirtualWidth( maxwidth );
			item->pub.h = ( item->pub.marginH * 2 ) + UI_VirtualHeight( trap_SCR_strHeight( item->pub.font ) );
		}

		// add the size of the controller buttons at the sides
		item->pub.w += ITEM_DIRECTION_CONTROL_SIZE * 2;

		// clamp button to window size
		UI_Item_ClampCoords( item );

		if( item->Update )
			item->Update( &item->pub );
	}
}

struct menuitem_public_s *UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemSpinner : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitSpinner( item, x, y, align, font, spinnerNames, Update, Apply, targetString );
		if( alwaysApply )
			item->Refresh = item->ApplyChanges;
	}

	return &item->pub;
}

/*
* ITEM_SLIDER
* The slider can be dragged along the bar
*/

#define ITEM_SLIDER_DEFAULT_WIDTH 200
#define ITEM_SLIDER_DEFAULT_HEIGHT 16

static void UI_Slider_Draw( menuitem_private_t *item )
{
	int alignedX, alignedY, i, start, end;
	float frac;

	// manually aligning the slider is easier
	alignedX = rX + UI_HorizontalAlign( item->pub.align, item->pub.w );
	alignedY = rY + UI_VerticalAlign( item->pub.align, item->pub.h );

	UI_DrawStretchPic( alignedX, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		UI_WindowColor( item->window, item->pub.color ), uis.itemSliderLeft );

	UI_DrawStretchPic( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		UI_WindowColor( item->window, item->pub.color ), uis.itemSliderRight );

	start = ( alignedX + ITEM_DIRECTION_CONTROL_SIZE );
	end = ( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE );

	for( i = start; i < end; i += ( ITEM_DIRECTION_CONTROL_SIZE - 1 ) )
	{
		UI_DrawStretchPic( i, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
			UI_WindowColor( item->window, item->pub.color ), uis.itemSliderBar );
	}


	// draw the handler
	frac = (float)item->pub.value / (float)( item->pub.maxvalue - item->pub.minvalue );
	i = start + ( ( end - start ) * frac ) - ( ITEM_DIRECTION_CONTROL_SIZE * 0.5f );
	UI_DrawStretchPic( i, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		UI_WindowColor( item->window, item->pub.color ), uis.itemSliderHandle );
}

static void UI_Slider_ClickEvent( menuitem_private_t *item, qboolean down )
{
	int alignedX, alignedY, start, end;

	UI_Item_ClampCoords( item );

	// manually aligning the slider
	alignedX = rX + UI_HorizontalAlign( item->pub.align, item->pub.w );
	alignedY = rY + UI_VerticalAlign( item->pub.align, item->pub.h );

	start = ( alignedX + ITEM_DIRECTION_CONTROL_SIZE );
	end = ( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE );

	if( uis.cursorX >= alignedX && uis.cursorX <= alignedX + ITEM_DIRECTION_CONTROL_SIZE )
	{
		// clicked left control
		if( down )
		{
			if( uis.time > item->timeStamp + item->pub.timeRepeat )
			{
				item->timeStamp = uis.time;
				item->pub.value -= item->pub.stepvalue;
			}
		}	
	}
	else if( uis.cursorX >= end && uis.cursorX <= end + ITEM_DIRECTION_CONTROL_SIZE )
	{
		// clicked right control
		if( down )
		{
			if( uis.time > item->timeStamp +  + item->pub.timeRepeat )
			{
				item->timeStamp = uis.time;
				item->pub.value += item->pub.stepvalue;
			}
		}
	}
	else if( uis.cursorX > start && uis.cursorX < end )
	{
		// clicked at the bar
		if( down )
		{
			float frac;
			frac = (float)( uis.cursorX - start ) / (float)( end - start );
			item->pub.value = frac * ( item->pub.maxvalue - item->pub.minvalue );
			item->timeStamp = uis.time;
		}
	}


	clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );

	if( !down )
	{
		if( item->Refresh )
			item->Refresh( &item->pub );
	}
}

static void UI_Items_InitSlider( menuitem_private_t *item, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, char *targetString )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->Draw = UI_Slider_Draw;
		item->ClickEvent = UI_Slider_ClickEvent;
		item->ApplyChanges = Apply;
		item->Update = Update;

		item->pub.minvalue = min;
		item->pub.maxvalue = max;
		item->pub.stepvalue = stepsize;
		item->pub.value = min;

		if( targetString && targetString[0] )
			item->pub.targetString = UI_CopyString( targetString );

		// Sliders don't use any text (at least by now)
		item->pub.font = NULL;
		item->pub.tittle[0] = 0;

		item->pub.w = ITEM_SLIDER_DEFAULT_WIDTH;
		item->pub.h = ITEM_SLIDER_DEFAULT_HEIGHT;

		// clamp button to window size
		UI_Item_ClampCoords( item );

		if( item->Update )
			item->Update( &item->pub );
	}
}

struct menuitem_public_s *UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, char *targetString )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemSlider : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitSlider( item, x, y, align, min, max, stepsize, Update, Apply, targetString );
		if( alwaysApply )
			item->Refresh = item->ApplyChanges;
	}

	return &item->pub;
}

/*
* ITEM_LISTBOX
* Clicking on this item moves into the next option
*/

#define BAR_SCROLLHEIGHT ( ( ( item->pub.maxvalue - item->pub.minvalue ) * item->pub.lineHeight ) - item->pub.h )
#define BAR_SLIDEMIN(y) ( 1 + y + ITEM_DIRECTION_CONTROL_SIZE + ( ITEM_DIRECTION_CONTROL_SIZE * 0.5 ) )
#define BAR_SLIDEMAX(y) ( y + ( item->pub.h - ITEM_DIRECTION_CONTROL_SIZE ) - ( 1 + ( ITEM_DIRECTION_CONTROL_SIZE * 0.5 ) ) )
static void UI_Listbox_Draw( menuitem_private_t *item )
{
	char *s;
	int alignedX, alignedY, innerWidth, i;
	vec4_t offcolor;
	int xmin, xmax, ymin, ymax;

	// manually aligning makes easier drawing the multiple parts
	alignedX = rX + UI_HorizontalAlign( item->pub.align, rW );
	alignedY = rY + UI_VerticalAlign( item->pub.align, rH );

	innerWidth = rW - ITEM_DIRECTION_CONTROL_SIZE;

	if( item->pub.custompic )
	{
		UI_DrawStretchPic( alignedX, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
			UI_WindowColor( item->window, item->pub.color ), item->pub.custompic );
	}
	else
	{
		UI_DrawDefaultListBox( alignedX, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, UI_WindowColor( item->window, item->pub.color ), ( uis.itemFocus == item ) );
	}

	// set up the strings context
	xmin = alignedX;
	xmax = alignedX + innerWidth;
	ymin = alignedY;
	ymax = alignedY + rH;

	xmin += item->pub.marginW;
	xmax -= item->pub.marginW;
	ymin += item->pub.marginH;
	ymax -= item->pub.marginH;

	if( item->pub.listNames )
	{
		int yd, xd;

		// the start position is scrolled up from the menuitem start
		yd = alignedY - ( (float)BAR_SCROLLHEIGHT * item->scroll_value );
		xd = alignedX;
		for( i = item->pub.minvalue; i < item->pub.maxvalue; i++, yd += item->pub.lineHeight )
		{
			// invisible, skip
			if( yd + item->pub.lineHeight < ymin )
				continue;
			if( yd > ymax )
				break;

			s = UI_ListNameForPosition( item->pub.listNames, i );
			if( !s ) // stop if no more lines with a content
				break;

			if( item == uis.itemFocus )
			{
				if( uis.cursorX >= xd && uis.cursorX <= xd + innerWidth
					&& uis.cursorY >= yd && uis.cursorY < yd + item->pub.lineHeight )
				{
					if( item->pub.custompicfocus )
					{
						UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
							UI_WindowColor( item->window, item->pub.color ), item->pub.custompicfocus );
					}
					else
					{
						UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
							UI_WindowColor( item->window, item->pub.color ), uis.itemListBoxFocusShader );
					}
				}
			}

			if( i == item->pub.value )
			{
				if( item->pub.custompicselected )
				{
					UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
						UI_WindowColor( item->window, item->pub.color ), item->pub.custompicselected );
				}
				else
				{
					UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
						UI_WindowColor( item->window, item->pub.color ), uis.itemListBoxSelectedShader );
				}
			}

			UI_DrawClampString( xd + item->pub.marginW, yd + item->pub.marginH, ALIGN_LEFT_TOP, s, xmin, ymin, xmax, ymax, item->pub.font, UI_WindowColor( item->window, item->pub.colorText ) );
		}
	}



	// draw the controllers

	Vector4Copy( UI_WindowColor( item->window, item->pub.color ), offcolor );
	if( item->scroll_value > item->pub.minvalue || item->scroll_value < item->pub.maxvalue - (item->pub.h / item->pub.lineHeight) )
	{	
	}
	else
	{
		for( i = 0; i < 3; i++ )
			offcolor[i] *= 0.75f;
	}

	UI_DrawStretchPic( alignedX + innerWidth, alignedY, ITEM_DIRECTION_CONTROL_SIZE, ITEM_DIRECTION_CONTROL_SIZE, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		offcolor, uis.scrollUpShader );

	UI_DrawStretchPic( alignedX + innerWidth, alignedY + item->pub.h - ITEM_DIRECTION_CONTROL_SIZE, ITEM_DIRECTION_CONTROL_SIZE, ITEM_DIRECTION_CONTROL_SIZE, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		offcolor, uis.scrollDownShader );

	// draw the handler
	ymin = BAR_SLIDEMIN( alignedY );
	ymax = BAR_SLIDEMAX( alignedY );
	i = ( ( ymax - ymin ) * item->scroll_value );
	UI_DrawStretchPic( alignedX + innerWidth, ymin + i - (ITEM_DIRECTION_CONTROL_SIZE*0.5), ITEM_DIRECTION_CONTROL_SIZE, ITEM_DIRECTION_CONTROL_SIZE, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		offcolor, uis.scrollHandleShader );
}

static void UI_Listbox_ClickEvent( menuitem_private_t *item, qboolean down )
{
	int alignedX, alignedY, innerWidth, i;

	UI_Item_ClampCoords( item );

	// manually aligning the slider
	alignedX = rX + UI_HorizontalAlign( item->pub.align, item->pub.w );
	alignedY = rY + UI_VerticalAlign( item->pub.align, item->pub.h );

	innerWidth = rW - ITEM_DIRECTION_CONTROL_SIZE;

	// step size has to be updated on the fly (no custom allowed in listboxes)
	if( BAR_SCROLLHEIGHT > 0 )
		item->pub.stepvalue = ( 1.0f / BAR_SCROLLHEIGHT );
	
	// if it's in the list side, check for selecting lines
	if( uis.cursorX >= alignedX && uis.cursorX <= alignedX + innerWidth )
	{
		if( !down )
		{
			int xd, yd;
			yd = alignedY - ( (float)BAR_SCROLLHEIGHT * item->scroll_value );
			xd = alignedX;
			for( i = item->pub.minvalue; i < item->pub.maxvalue; i++, yd += item->pub.lineHeight )
			{
				// invisible, skip
				if( yd + item->pub.lineHeight < alignedY + item->pub.marginH )
					continue;
				if( yd > alignedY + rH - item->pub.marginH )
					break;

				if( !UI_ListNameForPosition( item->pub.listNames, i ) ) // stop if no more lines with a content
					break;

				if( ( uis.cursorY >= yd ) && ( uis.cursorY < yd + item->pub.lineHeight ) )
				{
					if( ( i == item->pub.value ) && 
						( item->timeStamp + ITEM_DOUBLE_CLICK_MAX_TIME > uis.time ) )
					{
						// double clicking on a selected item executes the action
						if( !down )
						{
							if( item->Action )
								item->Action( &item->pub );
						}
					}

					item->pub.value = i;
					clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );

					item->timeStamp = uis.time;
				}
			}
		}
	}
	else if( uis.cursorX > alignedX + innerWidth /*&& uis.cursorX <= alignedX + item->pub.w*/ )
	{
		// using the scroll buttons

		if( uis.cursorY >= alignedY && uis.cursorY <= alignedY + ITEM_DIRECTION_CONTROL_SIZE && uis.cursorX <= alignedX + item->pub.w )
		{
			// clicked up control
			if( down && ( BAR_SCROLLHEIGHT > 0 ) )
			{
				if( uis.time > item->timeStamp + item->pub.timeRepeat )
				{
					item->timeStamp = uis.time;
					item->scroll_value -= item->pub.stepvalue;
				}
			}
		}
		else if( uis.cursorY >= alignedY + item->pub.h - ITEM_DIRECTION_CONTROL_SIZE && uis.cursorY <= alignedY + item->pub.h && uis.cursorX <= alignedX + item->pub.w )
		{
			// clicked down control
			if( down && ( BAR_SCROLLHEIGHT > 0 ) )
			{
				if( uis.time > item->timeStamp + item->pub.timeRepeat )
				{
					item->timeStamp = uis.time;
					item->scroll_value += item->pub.stepvalue;
				}
			}
		}
		else // clicked on the bar
		{
			if( down && ( BAR_SCROLLHEIGHT > 0 ) )
			{
				int ymin = BAR_SLIDEMIN( alignedY );
				int ymax = BAR_SLIDEMAX( alignedY );

				item->timeStamp = uis.time;
				if( uis.cursorY >= ymin && uis.cursorY <= ymax )
					item->scroll_value = (float)( uis.cursorY - ymin ) / (float)( ymax - ymin );
				
			}
		}
	}

	item->pub.value = (int)item->pub.value;
	clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );
	clamp( item->scroll_value, 0.0f, 1.0f );

	if( !down )
	{
		if( item->Refresh )
			item->Refresh( &item->pub );
	}
}

void UI_Items_InitListbox( menuitem_private_t *item, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, void *Action, char *targetString )
{
	int maxwidth = 0, i;
	char *s;

	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;
		item->pub.marginW = UI_VirtualWidth( TEXT_MARGIN );
		item->pub.marginH = UI_VirtualHeight( TEXT_MARGIN );

		item->Draw = UI_Listbox_Draw;
		item->ClickEvent = UI_Listbox_ClickEvent;
		item->Update = Update;
		item->ApplyChanges = Apply;
		item->Action = Action;

		item->pub.font = font ? font : uis.fontSystemSmall;
		item->pub.minvalue = 0;
		item->pub.maxvalue = 0;
		item->pub.value = item->pub.minvalue;

		item->pub.timeRepeat = 5;

		if( targetString && targetString[0] )
			item->pub.targetString = UI_CopyString( targetString );

		// count the amount of names to list

		if( namesList )
		{
			if( namesList[ strlen(namesList) - 1 ] != CHAR_SEPARATOR )
			{
				size_t size;

				// we have to add the last separator
				size = strlen( namesList ) + 1 + sizeof( CHAR_SEPARATOR );
				item->pub.listNames = UI_Malloc( size );
				Q_snprintfz( item->pub.listNames, size, "%s%c", namesList, CHAR_SEPARATOR );
			}
			else
			{
				item->pub.listNames = UI_CopyString( namesList );
			}

			// it's safe to count end-line separators now
			for( item->pub.maxvalue = 0; ( s = UI_ListNameForPosition( item->pub.listNames, item->pub.maxvalue ) ) != NULL; item->pub.maxvalue++ )
			{
				i = trap_SCR_strWidth( s, item->pub.font, 0 );
				if( i > maxwidth )
					maxwidth = i;

				// check for never having empty names
				s++;
				if( *s == CHAR_SEPARATOR )
					UI_Error( "UI_Items_InitSpinner: two consecutive end-line separators used\n" );
			}
		}

		item->pub.tittle[0] = 0; // tittle is not used
		if( !item->pub.maxvalue )
		{
			item->pub.lineHeight = ITEM_DEFAULT_HEIGHT / 2;
			item->pub.w = ( item->pub.marginW * 2 ) + ITEM_DEFAULT_WIDTH;
			item->pub.h = ( item->pub.marginH * 2 ) + ITEM_DEFAULT_HEIGHT;
		}
		else
		{
			item->pub.lineHeight = ( item->pub.marginH * 2 ) + UI_VirtualHeight( trap_SCR_strHeight( item->pub.font ) );
			item->pub.w = ( item->pub.marginW * 2 ) + UI_VirtualWidth( maxwidth );
			item->pub.h = item->pub.lineHeight * item->pub.maxvalue;
		}

		// add the size of the scrollbar
		item->pub.w += ITEM_DIRECTION_CONTROL_SIZE;

		// clamp button to window size
		UI_Item_ClampCoords( item );

		if( item->Update )
			item->Update( &item->pub );

		item->pub.stepvalue = ( 1.0f / 256.0f );
		clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );
		item->scroll_value = 0;
	}
}

struct menuitem_public_s *UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void ( *Update )( struct menuitem_public_s *item ), void *Apply, qboolean alwaysApply, void *Action, char *targetString )
{
	struct menuitem_private_s *item = UI_Window_CreateItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemListbox : failed to create the item at window %s\n", windowname );
	}
	else
	{
		UI_Items_InitListbox( item, x, y, align, font, namesList, Update, Apply, Action, targetString );
		if( alwaysApply )
			item->Refresh = item->ApplyChanges;
	}

	return &item->pub;
}

/*
* ITEMS MANAGEMENT
*
*/

void UI_RecurseDrawItems( menuitem_private_t *item )
{
	if( !item )
		return;

	if( item->next )
		UI_RecurseDrawItems( item->next );

	// lock size and position inside window size and position

	UI_Item_ClampCoords( item );

	// draw it

	if( item->Draw )
		item->Draw( item );
}

void UI_DrawItemsArray( menuitem_private_t *item )
{
	if( item )
		UI_RecurseDrawItems( item );
}

void UI_Item_ClickEvent( menuitem_private_t *item, qboolean down )
{
	if( item && item->ClickEvent )
	{
		if( down )
		{
			uis.itemDrag = item;
		}
		else
		{
			uis.itemDrag = NULL;
			item->ClickEvent( item, down );
		}
	}
}

menuitem_private_t *UI_RecurseFindItemFocus( menuitem_private_t *item )
{
	menuitem_private_t *focus;

	focus = NULL;
	if( item )
	{
		UI_Item_ClampCoords( item );

		// if this window is in focus it overrides
		// its children being in focus
		if( uis.cursorX >= rX + UI_HorizontalAlign( item->pub.align, item->pub.w ) &&
			uis.cursorX <= (rX + UI_HorizontalAlign( item->pub.align, item->pub.w ) + rW) &&
			uis.cursorY >= rY + UI_VerticalAlign( item->pub.align, item->pub.h ) &&
			uis.cursorY <= (rY + UI_VerticalAlign( item->pub.align, item->pub.h ) + rH) )
		{
			return item;
		}


		if( item->next )
			focus = UI_RecurseFindItemFocus( item->next );
	}

	return focus;
}

void UI_RecurseFreeItem( menuitem_private_t *item )
{
	if( item )
	{
		if( item->next )
			UI_RecurseFreeItem( item->next );

		if( item->pub.listNames )
			UI_Free( item->pub.listNames );

		if( item->pub.targetString )
			UI_Free( item->pub.targetString );

		UI_Free( item );
	}
}

menuitem_private_t *UI_RegisterItem( const char *name, menuitem_private_t **headnode )
{
	menuitem_private_t *item;

	if( !name )
		return NULL;

	if( strlen( name ) >= ITEM_MAX_NAME_SIZE )
		UI_Error( "UI_RegisterMenuItem: item name is too long\n" );

	for( item = *headnode; item; item = item->next )
	{
		if( !Q_stricmp( item->name, name ) ) {
			return item;
		}
	}

	item = UI_Malloc( sizeof( menuitem_private_t ) );
	memset( item, 0, sizeof( menuitem_private_t ) );
	Q_strncpyz( item->name, name, sizeof( item->name ) );
	item->next = *headnode;
	*headnode = item;

	// set up some default values
	item->pub.w = ITEM_DEFAULT_WIDTH;
	item->pub.h = ITEM_DEFAULT_HEIGHT;
	item->pub.align = ALIGN_LEFT_TOP;
	item->pub.font = uis.fontSystemSmall;
	item->pub.picS1 = item->pub.picT1 = 0;
	item->pub.picS2 = item->pub.picT2 = 1;
	Vector4Copy( colorWhite, item->pub.color );
	Vector4Copy( colorWhite, item->pub.colorText );

	item->pub.stepvalue = 1;
	item->pub.timeRepeat = 250;

	item->Draw = NULL;
	item->Action = NULL;
	item->Refresh = NULL;
	item->Update = NULL;
	item->ApplyChanges = NULL;
	item->ClickEvent = NULL;

	return item;
}

