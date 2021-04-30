/*
   Copyright (C) 2007 German Garcia
 */

extern cvar_t *cg_drawEntityBoxes;

extern void CG_PutEntityAtTag( entity_t *ent, orientation_t *tag, const vec3_t ref_origin, vec3_t ref_axis[3], const vec3_t ref_lightingorigin );
extern void CG_PutRotatedEntityAtTag( entity_t *ent, orientation_t *tag, const vec3_t ref_origin, vec3_t ref_axis[3], const vec3_t ref_lightingorigin );
extern void CG_MoveToTag( vec3_t move_origin, vec3_t move_axis[3], const vec3_t tag_origin, const vec3_t tag_axis[3], const vec3_t space_origin, const vec3_t space_axis[3] );

extern void CG_SoundEntityNewState( centity_t *cent );

extern void CG_AddEntitiesToScene( void );
extern void CG_InterpolateEntities( void );
extern void CG_UpdateEntitiesState( void );

extern void CG_PlayerModelEntityAddToScene( centity_t *cent );
extern void CG_PlayerModelEntityNewState( centity_t *cent );
