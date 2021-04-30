/*
   Copyright (C) 2007 German Garcia
 */

typedef struct
{
	unsigned int frameTime;
	qboolean newEntityEvents;
} cg_snapshot_t;

extern void CG_NewSnapshot( unsigned int snapNum, unsigned int serverTime );
