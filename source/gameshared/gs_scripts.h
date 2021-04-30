/*
   Copyright (C) 2007 German Garcia
 */

typedef struct gs_scriptnode_s
{
	int ( *func )( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments );
	int type;
	char *string;
	int integer;
	float value;
	float ( *opFunc )( const float a, float b );
	struct gs_scriptnode_s *parent;
	struct gs_scriptnode_s *next;
	struct gs_scriptnode_s *ifthread;
} gs_scriptnode_t;

typedef struct
{
	char *name;
	int ( *func )( void *parameter );
	void *parameter;
} gs_script_dynamic_t;

typedef struct
{
	char *name;
	int value;
} gs_script_constant_t;

typedef struct gs_scriptoperators_s
{
	char *name;
	float ( *opFunc )( const float a, const float b );
} gs_scriptoperators_t;

typedef struct gs_scriptcommand_s
{
	char *name;
	int ( *func )( struct gs_scriptnode_s *commandnode, struct gs_scriptnode_s *argumentnode, int numArguments );
	int numparms;
	char *help;
} gs_scriptcommand_t;

extern float GS_GetNumericArg( struct gs_scriptnode_s **argumentsnode );
extern char *GS_GetStringArg( struct gs_scriptnode_s **argumentsnode );
extern void GS_FreeScriptThread( gs_scriptnode_t *rootnode );
extern gs_scriptnode_t *GS_LoadScript( const char *name, const char *path, const char *extension, const gs_scriptcommand_t *commands, const gs_script_constant_t *constants, const gs_script_dynamic_t *dynamics );
extern void GS_ExecuteScriptThread( struct gs_scriptnode_s *rootnode, const gs_scriptcommand_t *commands, const gs_script_constant_t *constants, const gs_script_dynamic_t *dynamics );
