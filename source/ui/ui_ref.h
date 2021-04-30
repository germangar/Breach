/*
 */

#ifndef __UI_REF_H__
#define __UI_REF_H__

#define ITEM_MAX_TITTLE_SIZE	64

typedef struct menuitem_public_s
{
	char tittle[ITEM_MAX_TITTLE_SIZE];

	int x, y, w, h;
	int align, marginW, marginH;
	int lineHeight; // listboxes only

	float value;
	int minvalue;
	int maxvalue;
	float stepvalue;

	char *targetString; // hmpf, targetstrings only can be allocated inside the UI, this is not good to be public
	char *listNames; // spinners, listboxes...

	unsigned int timeRepeat;

	char windowname[32];

	struct mufont_s *font;
	struct shader_s *custompic, *custompicfocus, *custompicselected;
	float picS1, picS2, picT1, picT2;
	vec4_t color, colorText;
} menuitem_public_t;

#endif // __UI_REF_H__


