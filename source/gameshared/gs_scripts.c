/*
   Copyright (C) 2007 German Garcia
 */

#include "gs_local.h"

enum
{
	LNODE_NUMERIC,
	LNODE_DYNAMIC,
	LNODE_STRING,
	LNODE_COMMAND
};

static const gs_scriptcommand_t	*scriptCommands = NULL;
static const gs_script_constant_t *scriptConstants = NULL;
static const gs_script_dynamic_t *scriptDynamics = NULL;

//=============================================================================
// Operators
//=============================================================================

// we will always operate with floats so we don't have to code 2 different numeric paths
// it's not like using float or ints would make a difference in this simple-scripting case.

static float CG_OpFuncAdd( const float a, const float b )
{
	return a + b;
}

static float CG_OpFuncSubtract( const float a, const float b )
{
	return a - b;
}

static float CG_OpFuncMultiply( const float a, const float b )
{
	return a * b;
}

static float CG_OpFuncDivide( const float a, const float b )
{
	return a / b;
}

static float CG_OpFuncAND( const float a, const float b )
{
	return (int)a & (int)b;
}

static float CG_OpFuncOR( const float a, const float b )
{
	return (int)a | (int)b;
}

static float CG_OpFuncXOR( const float a, const float b )
{
	return (int)a ^ (int)b;
}

static float CG_OpFuncCompareEqual( const float a, const float b )
{
	return ( a == b );
}

static float CG_OpFuncCompareNotEqual( const float a, const float b )
{
	return ( a != b );
}

static float CG_OpFuncCompareGreater( const float a, const float b )
{
	return ( a > b );
}

static float CG_OpFuncCompareGreaterOrEqual( const float a, const float b )
{
	return ( a >= b );
}

static float CG_OpFuncCompareSmaller( const float a, const float b )
{
	return ( a < b );
}

static float CG_OpFuncCompareSmallerOrEqual( const float a, const float b )
{
	return ( a <= b );
}

static float CG_OpFuncCompareAnd( const float a, const float b )
{
	return ( a && b );
}

static float CG_OpFuncCompareOr( const float a, const float b )
{
	return ( a || b );
}

static gs_scriptoperators_t scriptOperators[] =
{
	{
		"+",
		CG_OpFuncAdd
	},

	{
		"-",
		CG_OpFuncSubtract
	},

	{
		"*",
		CG_OpFuncMultiply
	},

	{
		"/",
		CG_OpFuncDivide
	},

	{
		"&",
		CG_OpFuncAND
	},

	{
		"|",
		CG_OpFuncOR
	},

	{
		"^",
		CG_OpFuncXOR
	},

	{
		"==",
		CG_OpFuncCompareEqual
	},

	{
		"!=",
		CG_OpFuncCompareNotEqual
	},

	{
		">",
		CG_OpFuncCompareGreater
	},

	{
		">=",
		CG_OpFuncCompareGreaterOrEqual
	},

	{
		"<",
		CG_OpFuncCompareSmaller
	},

	{
		"<=",
		CG_OpFuncCompareSmallerOrEqual
	},

	{
		"&&",
		CG_OpFuncCompareAnd
	},

	{
		"||",
		CG_OpFuncCompareOr
	},

	{
		NULL,
		NULL
	},
};

//=============================================================================
// SCRIPTS EXECUTION
//=============================================================================

//================
//GS_GetStringArg
//================
char *GS_GetStringArg( struct gs_scriptnode_s **argumentsnode )
{
	struct gs_scriptnode_s *anode = *argumentsnode;

	if( !anode || anode->type == LNODE_COMMAND )
		GS_Error( "'CG_LayoutGetIntegerArg': bad arg count" );

	// we can return anything as string
	*argumentsnode = anode->next;
	return anode->string;
}

//================
//GS_GetNumericArg
//can use recursion for mathematical operations
//================
float GS_GetNumericArg( struct gs_scriptnode_s **argumentsnode )
{
	struct gs_scriptnode_s *anode = *argumentsnode;
	float value;

	if( !anode || anode->type == LNODE_COMMAND )
		GS_Error( "'CG_GetNumericArg': bad arg count" );

	if( anode->type != LNODE_NUMERIC && anode->type != LNODE_DYNAMIC )
		GS_Printf( "WARNING: 'CG_GetNumericArg': arg %s is not numeric", anode->string );

	*argumentsnode = anode->next;
	if( anode->type == LNODE_DYNAMIC )
		value = scriptDynamics[anode->integer].func( scriptDynamics[anode->integer].parameter );
	else
		value = anode->value;

	// recurse if there are operators
	if( anode->opFunc != NULL )
	{
		value = anode->opFunc( value, GS_GetNumericArg( argumentsnode ) );
	}

	return value;
}

//================
//GS_RecurseExecuteScriptThread
// Execution works like this: First node (on backwards) is expected to be the command, followed by arguments nodes.
// we keep a pointer to the command and run the tree counting arguments until we reach the next command,
// then we call the command function sending the pointer to first argument and the pointer to the command.
// At return we advance one node (we stopped at last argument node) so it starts again from the next command (if any).
//
// When finding an "if" command with a subtree, we execute the "if" command. In the case it
// returns any value, we recurse execute the subtree
//================
static void GS_RecurseExecuteScriptThread( gs_scriptnode_t *rootnode )
{
	gs_scriptnode_t	*argumentnode = NULL;
	gs_scriptnode_t	*commandnode = NULL;
	int numArguments;

	if( !rootnode )
		return;

	// run until the real root
	commandnode = rootnode;
	while( commandnode->parent )
	{
		commandnode = commandnode->parent;
	}

	// now run backwards up to the next command node
	while( commandnode )
	{
		argumentnode = commandnode->next;

		// we could trust the parser, but I prefer counting the arguments here
		numArguments = 0;
		while( argumentnode )
		{
			if( argumentnode->type == LNODE_COMMAND )
				break;

			argumentnode = argumentnode->next;
			numArguments++;
		}

		// reset
		argumentnode = commandnode->next;

		// Execute the command node
		if( commandnode->integer != numArguments )
		{
			GS_Printf( "ERROR: Layout command %s: invalid argument count (expecting %i, found %i)\n", commandnode->string, commandnode->integer, numArguments );
			return;
		}
		if( commandnode->func )
		{
			//special case for if commands
			if( commandnode->func( commandnode, argumentnode, numArguments ) )
			{
				// execute the "if" thread when command returns a value
				if( commandnode->ifthread )
					GS_RecurseExecuteScriptThread( commandnode->ifthread );
			}
		}

		//move up to next command node
		commandnode = argumentnode;
		if( commandnode == rootnode )
			return;

		while( commandnode && commandnode->type != LNODE_COMMAND )
		{
			commandnode = commandnode->next;
		}
	}
}

//================
//GS_ExecuteLayoutProgram
//================
void GS_ExecuteScriptThread( struct gs_scriptnode_s *rootnode, const gs_scriptcommand_t *commands, const gs_script_constant_t *constants, const gs_script_dynamic_t *dynamics )
{
	assert( commands && constants && dynamics );
	scriptCommands = commands;
	scriptConstants = constants;
	scriptDynamics = dynamics;

	GS_RecurseExecuteScriptThread( rootnode );
	scriptDynamics = NULL;
	scriptConstants = NULL;
	scriptCommands = NULL;
}

//=============================================================================
// SCRIPTS LOAD
//=============================================================================

//================
//GS_ScriptFixCommasInToken
//commas are accepted in the scripts. They actually do nothing, but are good for readability
//================
static qboolean GS_ScriptFixCommasInToken( char **ptr, char **backptr )
{
	char *token;
	char *back;
	int offset, count;
	qboolean stepback = qfalse;

	token = *ptr;
	back = *backptr;

	if( !token[0] ) return qfalse;

	// check that sizes match (quotes are removed from tokens)
	offset = count = strlen( token );
	back = *backptr;
	while( count-- )
	{
		if( *back == '"' )
		{
			count++;
			offset++;
		}
		back--;
	}

	back = *backptr - offset;
	while( offset )
	{
		if( *back == '"' )
		{
			offset--;
			back++;
			continue;
		}

		if( *token != *back )
			GS_Printf( "Token and Back mismatch %c - %c\n", *token, *back );

		if( *back == ',' )
		{
			*back = ' ';
			stepback = qtrue;
		}

		offset--;
		token++;
		back++;
	}

	return stepback;
}

//================
//GS_OperatorFuncForArgument
//================
static void *GS_OperatorFuncForArgument( char *token )
{
	gs_scriptoperators_t *op;

	while( *token == ' ' )
		token++;

	for( op = scriptOperators; op->name; op++ )
	{
		if( !Q_stricmp( token, op->name ) )
			return op->opFunc;
	}

	return NULL;
}

//================
//GS_ScriptParseCommandNode
//alloc a new node for a command
//================
static gs_scriptnode_t *GS_ScriptParseCommandNode( char *token )
{

	int i = 0;
	const gs_scriptcommand_t *command = NULL;
	gs_scriptnode_t *node;

	for( i = 0; scriptCommands[i].name; i++ )
	{
		if( !Q_stricmp( token, scriptCommands[i].name ) )
		{
			command = &scriptCommands[i];
			break;
		}
	}

	if( command == NULL )
		return NULL;

	node = ( gs_scriptnode_t * )module_Malloc( sizeof( gs_scriptnode_t ) );
	node->type = LNODE_COMMAND;
	node->integer = command->numparms;
	node->value = 0.0f;
	node->string = GS_CopyString( command->name );
	node->func = command->func;
	node->ifthread = NULL;

	return node;
}

//================
//GS_ScriptParseArgumentNode
//alloc a new node for an argument
//================
static gs_scriptnode_t *GS_ScriptParseArgumentNode( char *token )
{
	gs_scriptnode_t *node;
	int type = LNODE_NUMERIC;
	char *valuetok;
	static char tmpstring[8];

	// find what's it
	if( !token[0] )
		return NULL;

	valuetok = token;

	if( token[0] == '%' )
	{                  // it's a dynamic numerical reference
		int i;
		type = LNODE_DYNAMIC;
		valuetok++; // skip %

		// replace names by dynamic return indexes
		for( i = 0; scriptDynamics[i].name != NULL; i++ )
		{
			if( !Q_stricmp( valuetok, scriptDynamics[i].name ) )
			{
				Q_snprintfz( tmpstring, sizeof( tmpstring ), "%i", i );
				valuetok = tmpstring;
				break;
			}
		}
		if( scriptDynamics[i].name == NULL )
		{
			GS_Printf( "Warning: HUD: %s is not valid numeric reference\n", valuetok );
			valuetok--;
			valuetok = "0";
		}
	}
	else if( token[0] == '#' )
	{                         // it's a constant numerical reference
		int i;
		type = LNODE_NUMERIC;
		valuetok++; // skip #

		// replace constants names by values
		for( i = 0; scriptConstants[i].name != NULL; i++ )
		{
			if( !Q_stricmp( valuetok, scriptConstants[i].name ) )
			{
				Q_snprintfz( tmpstring, sizeof( tmpstring ), "%i", scriptConstants[i].value );
				valuetok = tmpstring;
				break;
			}
		}
		if( scriptConstants[i].name == NULL )
		{
			GS_Printf( "Warning: HUD: %s is not valid numeric reference\n", valuetok );
			valuetok--;
			valuetok = "0";
		}

	}
	else if( token[0] < '0' && token[0] > '9' && token[0] != '.' )
	{
		type = LNODE_STRING;
	}

	// alloc
	node = ( gs_scriptnode_t * )module_Malloc( sizeof( gs_scriptnode_t ) );
	node->type = type;
	node->integer = atoi( valuetok );
	node->value = atof( valuetok );
	node->string = GS_CopyString( token );
	node->func = NULL;
	node->ifthread = NULL;

	// return it
	return node;
}

//================
//GS_LayoutCathegorizeToken
//================
static int GS_CathegorizeScriptToken( char *token )
{
	int i = 0;

	for( i = 0; scriptCommands[i].name; i++ )
	{
		if( !Q_stricmp( token, scriptCommands[i].name ) )
			return LNODE_COMMAND;
	}

	if( token[0] == '%' )
	{                  // it's a dynamic numerical reference
		return LNODE_DYNAMIC;
	}
	else if( token[0] == '#' )
	{                         // it's a constant numerical reference
		return LNODE_NUMERIC;
	}
	else if( token[0] < '0' && token[0] > '9' && token[0] != '.' )
	{
		return LNODE_STRING;
	}

	return LNODE_NUMERIC;
}

//================
//GS_RecurseParseScript
//recursive for generating "if" subtrees
//================
static gs_scriptnode_t *GS_RecurseParseScript( char **ptr, int level )
{
	gs_scriptnode_t	*command = NULL;
	gs_scriptnode_t	*node = NULL;
	gs_scriptnode_t	*rootnode = NULL;
	int expecArgs = 0, numArgs = 0;
	int token_type;
	qboolean add;
	char *token, *s_tokenback;

	if( !ptr )
		return NULL;

	if( !*ptr || !*ptr[0] )
		return NULL;

	while( *ptr )
	{
		s_tokenback = *ptr;

		token = COM_Parse( ptr );

		while( *token == ' ' )
			token++; // eat up whitespaces

		if( !Q_stricmp( ",", token ) )
			continue; // was just a comma

		if( GS_ScriptFixCommasInToken( &token, ptr ) )
		{
			*ptr = s_tokenback; // step back
			continue;
		}

		if( !*token )
			continue;

		add = qfalse;
		token_type = GS_CathegorizeScriptToken( token );

		// if it's an operator, we don't create a node, but add the operation to the last one
		if( GS_OperatorFuncForArgument( token ) != NULL )
		{
			if( !node )
			{
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" Operator hasn't any prior argument\n", level, token );
				continue;
			}
			if( node->type == LNODE_COMMAND || node->type == LNODE_STRING )
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" Operator was assigned to a command node\n", level, token );
			else
				expecArgs++; // we now expect one extra argument (not counting the operator one)

#ifdef __cplusplus
			node->opFunc = ( float (__cdecl *)(const float,float) )GS_OperatorFuncForArgument( token );
#else
			node->opFunc = GS_OperatorFuncForArgument( token );
#endif
			continue; // skip and continue
		}

		if( expecArgs > numArgs )
		{                   // we are expecting an argument
			switch( token_type )
			{
			case LNODE_NUMERIC:
			case LNODE_STRING:
			case LNODE_DYNAMIC:
				break;
			case LNODE_COMMAND:
			{
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid argument for \"%s\"\n", level, token, command ? command->string : "" );
				continue;
			}
				break;
			default:
			{
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i) skip and continue: Unrecognized token \"%s\"\n", level, token );
				continue;
			}
				break;
			}
		}
		else
		{
			if( token_type != LNODE_COMMAND )
			{                       // we are expecting a command
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): unrecognized command \"%s\"\n", level, token );
				continue;
			}

			//special case: endif commands interrupt the thread and are not saved
			if( !Q_stricmp( token, "endif" ) )
			{
				//finish the last command properly
				if( command )
					command->integer = expecArgs;
				return rootnode;
			}

			//special case: last command was "if", we create a new sub-thread and ignore the new command
			if( command && !Q_stricmp( command->string, "if" ) )
			{
				*ptr = s_tokenback; // step back one token
				command->ifthread = GS_RecurseParseScript( ptr, level + 1 );
			}
		}

		// things look fine, proceed creating the node
		switch( token_type )
		{
		case LNODE_NUMERIC:
		case LNODE_STRING:
		case LNODE_DYNAMIC:
		{
			node = GS_ScriptParseArgumentNode( token );
			if( !node )
			{
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid argument for \"%s\"\n", level, token, command ? command->string : "" );
				break;
			}
			numArgs++;
			add = qtrue;
		}
			break;
		case LNODE_COMMAND:
		{
			node = GS_ScriptParseCommandNode( token );
			if( !node )
			{
				GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid command\n", level, token );
				break; // skip and continue
			}

			// expected arguments could have been extended by the operators
			if( command )
				command->integer = expecArgs;

			// move on into the new command
			command = node;
			numArgs = 0;
			expecArgs = command->integer;
			add = qtrue;
		}
			break;
		default:
			break;
		}

		if( add == qtrue )
		{
			if( rootnode )
				rootnode->next = node;
			node->parent = rootnode;
			rootnode = node;
		}
	}

	if( level > 0 )
		GS_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): If without endif\n", level );

	return rootnode;
}

//================
//GS_RecurseFreeScriptThread
//recursive for freeing "if" subtrees
//================
void GS_FreeScriptThread( gs_scriptnode_t *rootnode )
{
	gs_scriptnode_t *node;

	if( !rootnode )
		return;

	while( rootnode )
	{
		node = rootnode;
		rootnode = rootnode->parent;

		if( node->ifthread )
			GS_FreeScriptThread( node->ifthread );

		if( node->string )
			module_Free( node->string );

		module_Free( node );
	}

	rootnode = NULL;
}



//================
//GS_LoadScriptFile - Damien Deville ("Pb")
//================
static char *GS_LoadScriptFile( const char *name, const char *path, const char *extension, qboolean skip_include )
{
	int length, f;
	char *temp_buffer;
	char *opt_buffer;
	const char *parse;
	char *token, *toinclude;
	int optimized_length, included_length;
	int fi, fi_length;
	char fipath[MAX_QPATH], shortname[MAX_QPATH];

	// load the file
	Q_snprintfz( fipath, sizeof( fipath ), "%s/%s", path, name );
	COM_ReplaceExtension( fipath, extension, sizeof( fipath ) );

	length = module.trap_FS_FOpenFile( fipath, &f, FS_READ );
	if( length == -1 )
		return NULL;
	if( !length )
	{
		module.trap_FS_FCloseFile( f );
		return NULL;
	}

	// alloc a temp buffer according to size
	temp_buffer = ( char * )module_Malloc( length + 1 );

	// load layout file in memory
	module.trap_FS_Read( temp_buffer, length, f );
	module.trap_FS_FCloseFile( f );

	// first pass: scan buffer line by line and check for include lines
	// if found compute needed length for included files
	// else count token length as Com_Parse as skipped comments and stuff like that
	parse = temp_buffer;
	optimized_length = 0;
	included_length = 0;
	while( parse )
	{
		token = COM_ParseExt2( &parse, qtrue, qfalse );

		if( ( !Q_stricmp( token, "include" ) ) && skip_include == qfalse )
		{
			toinclude = COM_ParseExt2( &parse, qtrue, qfalse );

			Q_strncpyz( shortname, toinclude, sizeof( shortname ) );
			Q_snprintfz( fipath, sizeof( fipath ), "%s/%s", path, shortname );
			COM_ReplaceExtension( fipath, extension, sizeof( fipath ) );
			fi_length = module.trap_FS_FOpenFile( fipath, &fi, FS_READ );

			if( fi_length == -1 )
			{
				// failed to include file
				GS_Printf( "CG_LoadScriptFile: Failed to include subfile: %s \n", fipath );
			}

			if( fi_length > 0 )
			{
				// not an empty file
				// we have the size we can close it
				included_length += fi_length;
			}
			module.trap_FS_FCloseFile( fi );
		}
		else
		{
			// not an include line
			// simply count token size for optimized script
			optimized_length += strlen( token ) + 1; // for spaces
		}
	}

	// second pass: we now have the needed size
	// alloc optimized buffer
	opt_buffer = ( char * )module_Malloc( optimized_length + included_length + 1 );

	// reparse all file and copy it
	parse = temp_buffer;
	while( parse )
	{
		token = COM_ParseExt2( &parse, qtrue, qfalse );

		if( ( !Q_stricmp( token, "include" ) ) && skip_include == qfalse )
		{
			toinclude = COM_ParseExt2( &parse, qtrue, qfalse );

			Q_strncpyz( shortname, toinclude, sizeof( shortname ) );
			Q_snprintfz( fipath, sizeof( fipath ), "%s/%s", path, shortname );
			COM_ReplaceExtension( fipath, extension, sizeof( fipath ) );
			fi_length = module.trap_FS_FOpenFile( fipath, &fi, FS_READ );

			if( fi_length == -1 )
			{
				// failed to include file
				GS_Printf( "CG_LoadScriptFile: Failed to include subfile: %s \n", fipath );
			}

			if( fi_length > 0 )
			{
				const char *fi_parse;
				char *include_buffer;

				// reparse all lines from included file to skip include commands

				// alloc a temp buffer according to size
				include_buffer = ( char * )module_Malloc( fi_length + 1 );

				// load included layout file in memory
				module.trap_FS_Read( include_buffer, fi_length, fi );

				fi_parse = include_buffer;
				while( fi_parse )
				{
					token = COM_ParseExt2( &fi_parse, qtrue, qfalse );

					if( !Q_stricmp( token, "include" ) )
					{
						// skip recursive include
						toinclude = COM_ParseExt2( &fi_parse, qtrue, qfalse );
						GS_Printf( "CG_LoadScriptFile: No recursive include allowed: %s/%s\n", path, toinclude );
					}
					else
					{
						// normal token
						strcat( opt_buffer, token );
						strcat( opt_buffer, " " );
					}
				}

				// release memory
				module_Free( include_buffer );
			}

			// close included file
			module.trap_FS_FCloseFile( fi );
		}
		else
		{
			// normal token
			strcat( opt_buffer, token );
			strcat( opt_buffer, " " );
		}
	}

	// free temp buffer
	module_Free( temp_buffer );

	return opt_buffer;
}

gs_scriptnode_t *GS_LoadScript( const char *name, const char *path, const char *extension, const gs_scriptcommand_t *commands, const gs_script_constant_t *constants, const gs_script_dynamic_t *dynamics )
{
	gs_scriptnode_t *rootnode;
	char *buf;
	buf = GS_LoadScriptFile( name, path, extension, qfalse );
	if( buf == NULL )
	{
		GS_Printf( "GS_LoadScript: failed to load %s/%s%s file\n", path, name, extension );
		return NULL;
	}

	assert( commands && constants && dynamics );
	scriptCommands = commands;
	scriptConstants = constants;
	scriptDynamics = dynamics;

	rootnode = GS_RecurseParseScript( &buf, 0 );
	if( !rootnode )
		GS_Printf( "GS_LoadScript: failed to parse %s/%s%s file\n", path, name, extension );

	module_Free( buf );
	scriptDynamics = NULL;
	scriptConstants = NULL;
	scriptCommands = NULL;

	return rootnode;
}
