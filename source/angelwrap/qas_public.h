#ifndef __QAS_PUBLIC_H__
#define __QAS_PUBLIC_H__

#define	ANGELWRAP_API_VERSION   7

typedef struct
{
	void ( *Print )( const char *msg );
	void ( *Error )( const char *msg );

	unsigned int ( *Milliseconds )( void );

	// console variable interaction
	cvar_t *( *Cvar_Get )( const char *name, const char *value, int flags );
	cvar_t *( *Cvar_Set )( const char *name, const char *value );
	void ( *Cvar_SetValue )( const char *name, float value );
	cvar_t *( *Cvar_ForceSet )( const char *name, const char *value );
	float ( *Cvar_Value )( const char *name );
	const char *( *Cvar_String )( const char *name );

	int ( *Cmd_Argc )( void );
	char *( *Cmd_Argv )( int arg );
	char *( *Cmd_Args )( void );

	void ( *Cmd_AddCommand )( const char *name, void ( *cmd )( void ) );
	void ( *Cmd_RemoveCommand )( const char *cmd_name );
	void ( *Cmd_ExecuteText )( int exec_when, const char *text );

	// managed memory allocation
	struct mempool_s *( *Mem_AllocPool )( const char *name, const char *filename, int fileline );
	void *( *Mem_Alloc )( struct mempool_s *pool, int size, const char *filename, int fileline );
	void ( *Mem_Free )( void *data, const char *filename, int fileline );
	void ( *Mem_FreePool )( struct mempool_s **pool, const char *filename, int fileline );
	void ( *Mem_EmptyPool )( struct mempool_s *pool, const char *filename, int fileline );
} angelwrap_import_t;

typedef struct
{
	int ( *API )( void );
	int ( *Init )( void );
	void ( *Shutdown )( void );

	struct angelwrap_api_s *( *asGetAngelExport )( void );
} angelwrap_export_t;

#endif // __QAS_PUBLIC_H__
