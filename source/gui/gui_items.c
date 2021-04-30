/*
 */

#include "gui_local.h"

#define ITEM_DEFAULT_WIDTH		64
#define ITEM_DEFAULT_HEIGHT		32

#define TEXT_MARGIN 4
#define ITEM_DIRECTION_CONTROL_SIZE 16

#define ITEM_DOUBLE_CLICK_MAX_TIME	400

static int rX;
static int rY;
static int rW;
static int rH;

void cMenuItem::ClampCoords( void )
{
	int aX, aY;

	// first clamp the item sizes inside the window ones
	assert( this->parentWindow );

	if( this->pub.w > this->parentWindow->w )
		this->pub.w = this->parentWindow->w;

	if( this->pub.h > this->parentWindow->h )
		this->pub.h = this->parentWindow->h;

	aX = this->pub.x + UI_HorizontalAlign( this->pub.align, this->pub.w );
	aY = this->pub.y + UI_VerticalAlign( this->pub.align, this->pub.h );

	if( aX < 0 ) 
	{
		this->pub.x += -( aX );
	}
	else if( aX > this->parentWindow->w )
	{
		this->pub.x -= aX - this->parentWindow->w;
	}

	aX = this->pub.x + UI_HorizontalAlign( this->pub.align, this->pub.w );
	if( aX + this->pub.w > this->parentWindow->w )
		this->pub.w = this->parentWindow->w - aX;

	if( aY < 0 ) 
	{
		this->pub.y += -( aY );
	}
	else if( aY > this->parentWindow->h )
	{
		this->pub.h -= aY - this->parentWindow->h;
	}

	aY = this->pub.y + UI_HorizontalAlign( this->pub.align, this->pub.h );
	if( aY + this->pub.h > this->parentWindow->h )
		this->pub.h = this->parentWindow->h - aY;


	// update the final coordinates used for drawing and finding focus
	rX = this->parentWindow->x + this->pub.x;
	rY = this->parentWindow->y + this->pub.y;
	rW = this->pub.w;
	rH = this->pub.h;
}

float *cMenuItem::CurrentColor( void )
{
	static vec4_t backColorMask = { 0.9f, 0.9f, 0.9f, 1.0f };
	int i;

	assert( this->parentWindow );

	if( !this->parentWindow->IsTopWindow() )
	{
		for( i = 0; i < 4; i++ ) 
			this->currentColor[i] = this->pub.color[i] * backColorMask[i];
	}
	else
	{
		Vector4Copy( this->pub.color, this->currentColor );
	}

	return this->currentColor;
}

float *cMenuItem::CurrentTextColor( void )
{
	static vec4_t backColorMask = { 0.9f, 0.9f, 0.9f, 1.0f };
	int i;

	assert( this->parentWindow );

	if( !this->parentWindow->IsTopWindow() )
	{
		for( i = 0; i < 4; i++ ) 
			this->currentColor[i] = this->pub.colorText[i] * backColorMask[i];
	}
	else
	{
		Vector4Copy( this->pub.colorText, this->currentColor );
	}

	return this->currentColor;
}


/*
* ITEM_STATIC
* not interactive object.
*/

static void UI_GenericItem_CustomDraw( class cMenuItem *item )
{
	if( item == guiItemManager.focusedItem )
	{
		if( item->pub.custompicfocus )
		{
			UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
				item->CurrentColor(), item->pub.custompicfocus );
		}
		else if( item->CustomClickEvent )
		{
			UI_DrawDefaultItemBox( rX, rY, rW, rH, item->pub.align, item->CurrentColor(), qtrue );
		}
	}
	else
	{
		if( item->pub.custompic )
		{
			UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
				item->CurrentColor(), item->pub.custompic );
		}
		else if( item->CustomClickEvent )
		{
			UI_DrawDefaultItemBox( rX, rY, rW, rH, item->pub.align, item->CurrentColor(), qfalse );
		}
	}

	if( item->pub.custompicselected && item->pub.value )
		UI_DrawStretchPic( rX, rY, rW, rH, item->pub.align, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							item->CurrentColor(), item->pub.custompicselected );

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

		UI_DrawStringWidth( alignedX, alignedY, ALIGN_CENTER_MIDDLE, item->pub.tittle, maxwidth, item->pub.font, item->CurrentTextColor() );
	}
}

static void UI_MenuItems_InitStatic( class cMenuItem *item, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	if( item )
	{
		item->CustomDraw = UI_GenericItem_CustomDraw;

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
		item->ClampCoords();
	}
}

struct menuitem_public_s *UI_InitItemStatic( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

	UI_MenuItems_InitStatic( item, x, y, align, tittle, font );

	return &item->pub;
}

/*
* ITEM_WINDOWDRAGGER
* Dragging this item drags the window it's in
*/

#define ITEM_WINDOWDRAGGER_DEFAULT_WIDTH 64
#define ITEM_WINDOWDRAGGER_DEFAULT_HEIGHT 16

static void UI_WindowDragger_CustomClickEvent( class cMenuItem *item, qboolean down )
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
			item->parentWindow->x += uis.cursorX - item->pub.minvalue;
			item->parentWindow->y += uis.cursorY - item->pub.maxvalue;
			item->pub.minvalue = uis.cursorX;
			item->pub.maxvalue = uis.cursorY;
		}
	}
	else
	{
		item->pub.minvalue = 0;
		item->pub.maxvalue = 0;
		item->Refresh();
	}
}

static void UI_MenuItems_InitWindowDragger( class cMenuItem *item, int x, int y, int align )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->CustomDraw = UI_GenericItem_CustomDraw;
		item->CustomClickEvent = UI_WindowDragger_CustomClickEvent;

		// WindowDraggers don't use any text (at least by now)
		item->pub.font = NULL;
		item->pub.tittle[0] = 0;

		item->pub.w = ITEM_WINDOWDRAGGER_DEFAULT_WIDTH;
		item->pub.h = ITEM_WINDOWDRAGGER_DEFAULT_HEIGHT;

		// clamp button to window size
		item->ClampCoords();
	}
}

struct menuitem_public_s *UI_InitItemWindowDragger( const char *windowname, const char *itemname, int x, int y, int align )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

	UI_MenuItems_InitWindowDragger( item, x, y, align );

	return &item->pub;
}

/*
* ITEM_BUTTON
* Clicking on this item executes an action
*/

static void UI_Button_CustomClickEvent( class cMenuItem *item, qboolean down )
{
	if( !down )
		item->Action();
}

static void UI_MenuItems_InitButton( class cMenuItem *item, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	if( item )
	{
		item->CustomDraw = UI_GenericItem_CustomDraw;
		item->CustomClickEvent = UI_Button_CustomClickEvent;
		item->CustomAction = ( void (__cdecl *)(struct menuitem_public_s *) )Action;

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
		item->ClampCoords();
	}
}

struct menuitem_public_s *UI_InitItemButton( const char *windowname, const char *itemname, int x, int y, int align, const char *tittle, struct mufont_s *font, void *Action, char *targetString )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

	UI_MenuItems_InitButton( item, x, y, align, tittle, font, Action, targetString );

	return &item->pub;
}

/*
* ITEM_CHECKBOX
* Clicking on this item toggles it's value on and off
*/

#define ITEM_CHECKBOX_DEFAULT_WIDTH 16
#define ITEM_CHECKBOX_DEFAULT_HEIGHT 16

static void UI_CheckBox_CustomClickEvent( class cMenuItem *item, qboolean down )
{
	if( !down )
	{
		item->pub.value = !item->pub.value;
		item->Refresh();
	}
}

static void UI_MenuItems_InitCheckBox( class cMenuItem *item, int x, int y, int align, void *Update, void *Apply, char *targetString )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->CustomDraw = UI_GenericItem_CustomDraw;
		item->CustomClickEvent = UI_CheckBox_CustomClickEvent;
		item->CustomApplyChanges = ( void (__cdecl *)(struct menuitem_public_s *) )Apply;
		item->CustomUpdate = ( void (__cdecl *)(struct menuitem_public_s *) )Update;

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
		item->ClampCoords();
		item->Update();
	}
}

struct menuitem_public_s *UI_InitItemCheckBox( const char *windowname, const char *itemname, int x, int y, int align, void *Update, void *Apply, qboolean alwaysApply, char *targetString )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

		UI_MenuItems_InitCheckBox( item, x, y, align, Update, Apply, targetString );
		if( alwaysApply )
			item->CustomRefresh = item->CustomApplyChanges;

	return &item->pub;
}

/*
* ITEM_SPINNER
* Clicking on this item moves into the next option
*/

static void UI_Spinner_CustomDraw( class cMenuItem *item )
{
	char *s;
	int alignedX, alignedY, innerWidth, start, end;

	// manually aligning makes easier drawing the multiple parts
	alignedX = rX + UI_HorizontalAlign( item->pub.align, rW );
	alignedY = rY + UI_VerticalAlign( item->pub.align, rH );

	start = alignedX + ITEM_DIRECTION_CONTROL_SIZE;
	end = alignedX + rW - ITEM_DIRECTION_CONTROL_SIZE;
	innerWidth = rW - (2 * ITEM_DIRECTION_CONTROL_SIZE);

	if( item == guiItemManager.focusedItem )
	{
		if( item->pub.custompicfocus )
		{
			UI_DrawStretchPic( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							item->CurrentColor(), item->pub.custompicfocus );
		}
		else
		{
			UI_DrawDefaultItemBox( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->CurrentColor(), qtrue );
		}
	}
	else
	{
		if( item->pub.custompic )
		{
			UI_DrawStretchPic( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->pub.picS1, item->pub.picT1, item->pub.picS2, item->pub.picT2,
							item->CurrentColor(), item->pub.custompic );
		}
		else
		{
			UI_DrawDefaultItemBox( start, alignedY, innerWidth, rH, ALIGN_LEFT_TOP, item->CurrentColor(), qfalse );
		}
	}

	// draw the controllers
	UI_DrawStretchPic( alignedX, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		item->CurrentColor(), uis.itemSliderLeft );

	UI_DrawStretchPic( alignedX + rW - ITEM_DIRECTION_CONTROL_SIZE, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		item->CurrentColor(), uis.itemSliderRight );

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

		UI_DrawStringWidth( innerX, innerY, ALIGN_CENTER_MIDDLE, s, maxwidth, item->pub.font, item->CurrentTextColor() );
	}
}

static void UI_Spinner_CustomClickEvent( class cMenuItem *item, qboolean down )
{
	int alignedX, alignedY, start, end;

	item->ClampCoords();

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
		item->Refresh();
	}
}

static void UI_MenuItems_InitSpinner( class cMenuItem *item, int x, int y, int align, struct mufont_s *font, char *namesList, void *Update, void *Apply, char *targetString )
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

		item->CustomDraw = UI_Spinner_CustomDraw;
		item->CustomClickEvent = UI_Spinner_CustomClickEvent;
		item->CustomUpdate = ( void (__cdecl *)(struct menuitem_public_s *) )Update;
		item->CustomApplyChanges = ( void (__cdecl *)(struct menuitem_public_s *) )Apply;

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
				item->pub.listNames = ( char * )UI_Malloc( size );
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
		item->ClampCoords();
		item->Update();
	}
}

struct menuitem_public_s *UI_InitItemSpinner( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *spinnerNames, void *Update, void *Apply, qboolean alwaysApply, char *targetString )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

		UI_MenuItems_InitSpinner( item, x, y, align, font, spinnerNames, Update, Apply, targetString );
		if( alwaysApply )
			item->CustomRefresh = item->CustomApplyChanges;


	return &item->pub;
}

/*
* ITEM_SLIDER
* The slider can be dragged along the bar
*/

#define ITEM_SLIDER_DEFAULT_WIDTH 200
#define ITEM_SLIDER_DEFAULT_HEIGHT 16

static void UI_Slider_CustomDraw( class cMenuItem *item )
{
	int alignedX, alignedY, i, start, end;
	float frac;

	// manually aligning the slider is easier
	alignedX = rX + UI_HorizontalAlign( item->pub.align, item->pub.w );
	alignedY = rY + UI_VerticalAlign( item->pub.align, item->pub.h );

	UI_DrawStretchPic( alignedX, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		item->CurrentColor(), uis.itemSliderLeft );

	UI_DrawStretchPic( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		item->CurrentColor(), uis.itemSliderRight );

	start = ( alignedX + ITEM_DIRECTION_CONTROL_SIZE );
	end = ( alignedX + item->pub.w - ITEM_DIRECTION_CONTROL_SIZE );

	for( i = start; i < end; i += ( ITEM_DIRECTION_CONTROL_SIZE - 1 ) )
	{
		UI_DrawStretchPic( i, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
			item->CurrentColor(), uis.itemSliderBar );
	}


	// draw the handler
	frac = (float)item->pub.value / (float)( item->pub.maxvalue - item->pub.minvalue );
	i = start + ( ( end - start ) * frac ) - ( ITEM_DIRECTION_CONTROL_SIZE * 0.5f );
	UI_DrawStretchPic( i, alignedY, ITEM_DIRECTION_CONTROL_SIZE, rH, ALIGN_LEFT_TOP, 0, 0, 1, 1,
		item->CurrentColor(), uis.itemSliderHandle );
}

static void UI_Slider_CustomClickEvent( class cMenuItem *item, qboolean down )
{
	int alignedX, alignedY, start, end;

	item->ClampCoords();

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
		item->Refresh();
	}
}

static void UI_MenuItems_InitSlider( class cMenuItem *item, int x, int y, int align, int min, int max, float stepsize, void *Update, void *Apply, char *targetString )
{
	if( item )
	{
		item->pub.x = x;
		item->pub.y = y;
		item->pub.align = align;

		item->CustomDraw = UI_Slider_CustomDraw;
		item->CustomClickEvent = UI_Slider_CustomClickEvent;
		item->CustomApplyChanges = ( void (__cdecl *)(struct menuitem_public_s *) )Apply;
		item->CustomUpdate = ( void (__cdecl *)(struct menuitem_public_s *) )Update;

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
		item->ClampCoords();
		item->Update();
	}
}

struct menuitem_public_s *UI_InitItemSlider( const char *windowname, const char *itemname, int x, int y, int align, int min, int max, float stepsize, void *Update, void *Apply, qboolean alwaysApply, char *targetString )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

		UI_MenuItems_InitSlider( item, x, y, align, min, max, stepsize, Update, Apply, targetString );
		if( alwaysApply )
			item->CustomRefresh = item->CustomApplyChanges;

	return &item->pub;
}

/*
* ITEM_LISTBOX
* Clicking on this item moves into the next option
*/

#define BAR_SCROLLHEIGHT ( ( ( item->pub.maxvalue - item->pub.minvalue ) * item->pub.lineHeight ) - item->pub.h )
#define BAR_SLIDEMIN(y) ( 1 + y + ITEM_DIRECTION_CONTROL_SIZE + ( ITEM_DIRECTION_CONTROL_SIZE * 0.5 ) )
#define BAR_SLIDEMAX(y) ( y + ( item->pub.h - ITEM_DIRECTION_CONTROL_SIZE ) - ( 1 + ( ITEM_DIRECTION_CONTROL_SIZE * 0.5 ) ) )

static void UI_Listbox_CustomDraw( class cMenuItem *item )
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
			item->CurrentColor(), item->pub.custompic );
	}
	else
	{
		UI_DrawDefaultListBox( alignedX, alignedY, innerWidth, rH, ALIGN_LEFT_TOP,
			item->CurrentColor(), ( guiItemManager.focusedItem == item ) ? qtrue : qfalse );
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

			if( item == guiItemManager.focusedItem )
			{
				if( uis.cursorX >= xd && uis.cursorX <= xd + innerWidth
					&& uis.cursorY >= yd && uis.cursorY < yd + item->pub.lineHeight )
				{
					if( item->pub.custompicfocus )
					{
						UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
							item->CurrentColor(), item->pub.custompicfocus );
					}
					else
					{
						UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
							item->CurrentColor(), uis.itemListBoxFocusShader );
					}
				}
			}

			if( i == item->pub.value )
			{
				if( item->pub.custompicselected )
				{
					UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
						item->CurrentColor(), item->pub.custompicselected );
				}
				else
				{
					UI_DrawClampPic( xd, yd, innerWidth, item->pub.lineHeight, ALIGN_LEFT_TOP, xmin, ymin, xmax, ymax,
						item->CurrentColor(), uis.itemListBoxSelectedShader );
				}
			}

			UI_DrawClampString( xd + item->pub.marginW, yd + item->pub.marginH, ALIGN_LEFT_TOP, s, xmin, ymin, xmax, ymax, item->pub.font, item->CurrentTextColor() );
		}
	}



	// draw the controllers

	Vector4Copy( item->CurrentColor(), offcolor );
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

static void UI_Listbox_CustomClickEvent( class cMenuItem *item, qboolean down )
{
	int alignedX, alignedY, innerWidth, i;

	item->ClampCoords();

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
							item->Action();
						}
					}

					item->pub.value = i;
					clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );

					item->timeStamp = uis.time;
				}
			}
		}
	}
	else if( uis.cursorX > alignedX + innerWidth )
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
		item->Refresh();
	}
}

void UI_MenuItems_InitListbox( class cMenuItem *item, int x, int y, int align, struct mufont_s *font, char *namesList, void *Update, void *Apply, void *Action, char *targetString )
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

		item->CustomDraw = UI_Listbox_CustomDraw;
		item->CustomClickEvent = UI_Listbox_CustomClickEvent;
		item->CustomUpdate = ( void (__cdecl *)(struct menuitem_public_s *) )Update;
		item->CustomApplyChanges = ( void (__cdecl *)(struct menuitem_public_s *) )Apply;
		item->CustomAction = ( void (__cdecl *)(struct menuitem_public_s *) )Action;

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
				item->pub.listNames = ( char * )UI_Malloc( size );
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
		item->ClampCoords();
		item->Update();

		item->pub.stepvalue = ( 1.0f / 256.0f );
		clamp( item->pub.value, item->pub.minvalue, item->pub.maxvalue - 1 );
		item->scroll_value = 0;
	}
}

struct menuitem_public_s *UI_InitItemListbox( const char *windowname, const char *itemname, int x, int y, int align, struct mufont_s *font, char *namesList, void *Update, void *Apply, qboolean alwaysApply, void *Action, char *targetString )
{
	class cMenuItem *item = guiWindowManager.CreateMenuItem( windowname, itemname );
	if( !item )
	{
		UI_Error( "UI_InitItemStatic : failed to create the item at window %s\n", windowname );
		return NULL;
	}

		UI_MenuItems_InitListbox( item, x, y, align, font, namesList, Update, Apply, Action, targetString );
		if( alwaysApply )
			item->CustomRefresh = item->CustomApplyChanges;

	return &item->pub;
}

void cMenuItem::Draw( void )
{
	// lock size and position inside window size and position

	this->ClampCoords();

	// draw it
	if( this->CustomDraw )
		this->CustomDraw( this );
}

void cMenuItem::ClickEvent( qboolean down )
{
	if( this->CustomClickEvent )
		this->CustomClickEvent( this, down );
}

void cMenuItem::Action( void )
{
	if( this->CustomAction )
		this->CustomAction( &this->pub );
}

void cMenuItem::Refresh( void )
{
	if( this->CustomRefresh )
		this->CustomRefresh( &this->pub );
}

void cMenuItem::Update( void )
{
	if( this->CustomUpdate )
		this->CustomUpdate( &this->pub );
}

void cMenuItem::ApplyChanges( void )
{
	if( this->CustomApplyChanges )
		this->CustomApplyChanges( &this->pub );
}


/*
* ITEMS MANAGEMENT
*
*/

cMenuItemManager guiItemManager;

void cMenuItemManager::ClickEvent( class cMenuItem *menuItem, qboolean down )
{
	if( menuItem && menuItem->CustomClickEvent )
	{
		if( down )
		{
			guiItemManager.dragItem = menuItem;
		}
		else
		{
			guiItemManager.dragItem = NULL;
			menuItem->ClickEvent( down );
		}
	}
}

void cMenuItemManager::RecurseDrawItems( class cMenuItem *menuItem )
{
	if( !menuItem )
		return;

	if( menuItem->next )
		this->RecurseDrawItems( menuItem->next );

	menuItem->Draw();
}

class cMenuItem *cMenuItemManager::RecurseFindItemFocus( class cMenuItem *item )
{
	class cMenuItem *focus = NULL;

	if( item )
	{
		item->ClampCoords();

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
			focus = this->RecurseFindItemFocus( item->next );
	}

	return focus;
}

void cMenuItemManager::UpdateFocusedItem( void )
{
	this->focusedItem = NULL;

	if( !guiWindowManager.focusedWindow )
		return;

	if( guiWindowManager.focusedWindow->IsTopWindow() || guiWindowManager.focusedWindow->mergeablefocus ) 
	{
		this->focusedItem = this->RecurseFindItemFocus( guiWindowManager.focusedWindow->menuItemsHeadnode );
	}
}

void cMenuItemManager::RecurseFreeItems( class cMenuItem *item )
{
	if( item )
	{
		if( item->next )
			this->RecurseFreeItems( item->next );

		if( item->pub.listNames )
			UI_Free( item->pub.listNames );

		if( item->pub.targetString )
			UI_Free( item->pub.targetString );

		UI_Free( item );
	}
}

class cMenuItem *cMenuItemManager::RegisterItem( const char *name, class cMenuItem **headnode )
{
	class cMenuItem *item;

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

	item = ( class cMenuItem * )UI_Malloc( sizeof( cMenuItem ) );
	memset( item, 0, sizeof( cMenuItem ) );
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

	item->CustomDraw = NULL;
	item->CustomAction = NULL;
	item->CustomRefresh = NULL;
	item->CustomUpdate = NULL;
	item->CustomApplyChanges = NULL;
	item->CustomClickEvent = NULL;

	return item;
}

//=====================================================

