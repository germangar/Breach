/*
 */

#ifndef __UI_PLAYERMODELS_H__
#define __UI_PLAYERMODELS_H__

typedef struct
{
	int nskins;
	char **skinnames;
	char directory[MAX_QPATH];
} playermodelinfo_s;

extern m_itemslisthead_t playermodelsItemsList;
extern byte_vec4_t playerColor;

void UI_Playermodel_Init( void );
void UI_FindIndexForModelAndSkin( char *model, char *skin, int *modelindex, int *skinindex );
void UI_DrawPlayerModel( char *model, char *skin, byte_vec4_t color, int xpos, int ypos, int width, int height, int frame, int oldframe );
qboolean UI_PlayerModelNextFrameTime( void );

#endif // __UI_PLAYERMODELS_H__
