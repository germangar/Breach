/*
Copyright (C) 2007 German Garcia
*/

typedef struct
{
	int type;
	qboolean thirdperson;
	qboolean playerPrediction;
	refdef_t refdef;
	float fracDistFOV;
	vec3_t origin;
	vec3_t angles;
	vec3_t axis[3];
	vec3_t velocity;

	qboolean drawWeapon;
} cg_viewdef_t;

void CG_RefreshTimeFracs( void );