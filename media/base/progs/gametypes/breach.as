

/*
const int MOVER_FLAG_TOGGLE = 1;
const int MOVER_FLAG_REVERSE = 2;
const int MOVER_FLAG_CRUSHER = 4;
const int MOVER_FLAG_TAKEDAMAGE = 8;
const int MOVER_FLAG_DENY_TOUCH_ACTIVATION = 16;
const int MOVER_FLAG_ALLOW_SHOT_ACTIVATION = 32;

const int MOVER_STATE_ENDPOS = 0;
const int MOVER_STATE_STARTPOS = 1;
const int MOVER_STATE_GO_START = 2;
const int MOVER_STATE_GO_END = 3;

const int DEFAULT_MOVER_HEALTH = 800;

const int MOVER_ENDFUNC_MOVER_HIT_START = 1;
const int MOVER_ENDFUNC_MOVER_HIT_END = 2;
const int MOVER_ENDFUNC_ROTATING_HIT_START = 3;
const int MOVER_ENDFUNC_ROTATING_HIT_END = 3;

const int THINK_MOVER_NULL = 0;
const int THINK_MOVER_GO_START = 1;
const int THINK_MOVER_WATCH = 2;
const int THINK_MOVER_DONE = 3;


class cMover
{
	cEntity @ent;

	cVec3 start_origin;
	cVec3 start_angles;
	cVec3 end_origin;
	cVec3 end_angles;

	int sound;
	int event_endpos; int event_startpos; int event_startmoving; int event_startreturning;
	cVec3 movedir;  // direction defined in the bsp

	float speed;
	float wait;

	float phase;        // pendulum only
	float accel;        // func_rotating only

	// state data
	int state;
	bool is_areaportal; // doors only
	bool rotating;

	//void ( *endfunc )( gentity_t * );

	cEntity @activator;

	cVec3 dest;
	cVec3 destangles;

	int endFuncCase;
	int thinkCase;

	cMover() {
		@this.ent = null;
		this.endFuncCase = 0;
	}
	~cMover() {}

	void Init( cEntity @e )
	{
		if( @e == null )
			return;

		@this.ent = @e;

		//memset( &ent->mover, 0, sizeof( g_moverinfo_t ) );
		this.ent.solid = SOLID_SOLID;
		this.ent.setupModel( this.ent.getModelString() );
		this.ent.moveType = MOVE_TYPE_PUSHER;
		this.ent.netFlags &= ~SVF_NOCLIENT;

		if( this.ent.getModel2String().len() > 0 )
			this.ent.modelindex2 = game.modelIndex( this.ent.getModel2String() );

		if( this.ent.wait < 0 )
			this.ent.wait = 0;
		if( this.ent.health < 0 )
			this.ent.health = 0;
		if( this.ent.damage < 0 )
			this.ent.damage = 0;

		// TEMP FIXME: allow every mover to be activated by projectiles by now
		this.ent.spawnFlags |= MOVER_FLAG_ALLOW_SHOT_ACTIVATION;

		if( ( this.ent.spawnFlags & MOVER_FLAG_TAKEDAMAGE ) != 0 )
			this.ent.flags |= SFL_TAKEDAMAGE;

		if( this.ent.health < 1 && ( this.ent.flags & SFL_TAKEDAMAGE ) != 0 )
			this.ent.health = DEFAULT_MOVER_HEALTH;
	}

	void Die( cEntity @inflictor, cEntity @attacker, float damage )
	{
		G_Print( "cMover::Die: Damage " + damage + " Health: " + this.ent.health + "\n" );
		this.ent.freeEntity();
	}

	void Done()
	{
		this.ent.setOrigin( this.dest );
		this.ent.setAngles( this.destangles );
		this.ent.setVelocity( cVec3( 0, 0, 0 ) );
		this.ent.setAVelocity( cVec3( 0, 0, 0 ) );

		switch( this.endFuncCase )
		{
		case MOVER_ENDFUNC_MOVER_HIT_START:
			this.MoverHitStart();
			break;
		case MOVER_ENDFUNC_MOVER_HIT_END:
			this.MoverHitEnd();
			break;
		case MOVER_ENDFUNC_ROTATING_HIT_START:
			this.RotatingHitStart();
			break;
		case MOVER_ENDFUNC_ROTATING_HIT_END:
			this.RotatingHitEnd();
			break;
		default:
			this.endFuncCase = 0;
			break;
		}
	}

	void Watch()
	{
		cVec3 dir;
		float dist;
		bool reached, areached;
		float secsFrameTime;

		reached = areached = false;

		secsFrameTime = float( game.frameMsecs ) * 0.001f;

		if( this.dest == this.ent.getOrigin() )
		{
			this.ent.setVelocity( cVec3( 0, 0, 0 ) );
			reached = true;
		}

		if( this.destangles == this.ent.getAngles() )
		{
			this.ent.setAVelocity( cVec3( 0, 0, 0 ) );
			areached = true;
		}

		if( reached && areached )
		{
			//this.Done();
			this.thinkCase = THINK_MOVER_DONE;
			this.ent.nextThink = levelTime + 1;
			return;
		}
		
		if( !reached ) // see if it's going to be reached, and adjust
		{
			dir = this.dest - this.ent.getOrigin();
			dist = dir.normalize();

			float speed = this.speed;
			if( dist < ( speed * secsFrameTime ) )
				speed = dist / secsFrameTime;

			ent.setVelocity( dir * speed );
		}

		if( !areached ) // see if it's going to be reached, and adjust
		{
			dir = this.destangles - this.ent.getAngles();
			dist = dir.normalize();

			float speed = ent.getAVelocity().length();
			if( dist < ( speed * secsFrameTime ) )
				speed = dist / secsFrameTime;

			ent.setAVelocity( dir * speed );
		}

		ent.nextThink = levelTime + 1;
		this.thinkCase = THINK_MOVER_WATCH;
	}

	void Begin()
	{
		cVec3 dir, destdelta;
		float remainingdist, angledist, anglespeed, time;

		// set up velocity vector
		dir = this.dest - this.ent.getOrigin();
		remainingdist = dir.normalize();
		if( remainingdist <= 0.0f )
		{
			time = 0;
			this.ent.setVelocity( cVec3( 0, 0, 0 ) );
		}
		else
		{
			time = remainingdist / this.speed;
			dir *= this.speed;
			dir.snap();
			this.ent.setVelocity( dir );
		}

		// set up avelocity vector
		destdelta = this.destangles - this.ent.getAngles();
		angledist = destdelta.normalize();
		if( time != 0 ) // if time was set up by traveling distance, scale angle speed to fit it
			anglespeed = angledist / time;
		else
			anglespeed = this.speed;

		this.ent.setAVelocity( destdelta * anglespeed );

		this.Watch(); // give it a first watch, just in case it has no distance to travel.
	}


	void Calc( cVec3 dest_origin, cVec3 dest_angles, int endfunctype )
	{
		this.endFuncCase = endfunctype;
		this.dest = dest_origin;
		this.dest.snap();  // it's important to snap the destiny, or might never be reached.
		this.destangles = dest_angles;
		this.ent.setVelocity( cVec3( 0, 0, 0 ) );
		this.ent.setAVelocity( cVec3( 0, 0, 0 ) );

		this.Begin();
	}

	void MoverHitEnd()
	{
		this.endFuncCase = 0;

		//G_AddEvent( ent, ent->mover.event_endpos, 0, qtrue );
		this.ent.sound = 0;
		this.state = MOVER_STATE_ENDPOS;

		// FIXME!!!
		if( ( this.ent.spawnFlags & MOVER_FLAG_TOGGLE ) != 0 )
			this.thinkCase = THINK_MOVER_NULL;
		else
		{
			this.ent.nextThink = levelTime + this.wait;
			this.thinkCase = THINK_MOVER_GO_START;
		}

		if( this.is_areaportal && ( this.ent.spawnFlags & MOVER_FLAG_REVERSE ) != 0 )
			this.ent.areaPortalOpen = false;

		this.ent.activateTargets( this.activator );
	}

	void MoverHitStart()
	{
		this.endFuncCase = 0;

		//G_AddEvent( ent, ent->mover.event_startpos, 0, qtrue );
		ent.sound = 0;

		//ent.think = NULL;
		//ent.nextThink = levelTime + this.wait;
		this.thinkCase = THINK_MOVER_NULL; // FIXME!!!
		ent.nextThink = 0;
		@this.activator = null;
		this.state = MOVER_STATE_STARTPOS;

		if( this.is_areaportal && ( ent.spawnFlags & MOVER_FLAG_REVERSE ) == 0 )
			this.ent.areaPortalOpen = false;
	}

	void MoverGoStart()
	{
		//G_AddEvent( ent, ent->mover.event_startreturning, 0, qtrue );
		this.ent.sound = this.sound;

		this.state = MOVER_STATE_GO_START;
		this.Calc( this.start_origin, this.start_angles, MOVER_ENDFUNC_MOVER_HIT_START );

		if( this.is_areaportal && ( ent.spawnFlags & MOVER_FLAG_REVERSE ) != 0 )
			this.ent.areaPortalOpen = true;
	}

	void MoverGoEnd()
	{
		//G_AddEvent( ent, ent->mover.event_startmoving, 0, qtrue );
		this.ent.sound = this.sound;

		this.state = MOVER_STATE_GO_END;
		this.Calc( this.end_origin, this.end_angles, MOVER_ENDFUNC_MOVER_HIT_END );

		if( this.is_areaportal && ( ent.spawnFlags & MOVER_FLAG_REVERSE ) == 0 )
			this.ent.areaPortalOpen = true;
	}

	void RotatingHitStart()
	{
		this.endFuncCase = 0;
	}

	void RotatingHitEnd()
	{
		this.endFuncCase = 0;
	}
}
*/


















//===================================================

void target_print_activate( cEntity @ent, cEntity @other, cEntity @activator )
{
	if( @activator != null && @activator.client != null )
		activator.client.printMessage( ent.getSpawnKey( "message" ) + "\n" );
	//G_Print( ent.getSpawnKey( "message" ) + "\n" );
}

void target_print( cEntity @ent )
{
	if( ent.getSpawnKey( "message" ).len() == 0 )
	{
		G_Print( "Removing target_print entity with no message\n" );
		ent.freeEntity();
	}
}


void misc_rotated_box( cEntity @ent )
{
	float radius;
	
	//G_Print( "Radius : " + ent.getSpawnKey( "radius" ) + "\n" );
	
	radius = ent.getSpawnKey( "radius" ).toFloat();
	if( radius <= 0 )
	{
		ent.freeEntity();
		return;
	}

	ent.type = ET_MODEL;
	ent.solid = SOLID_SOLID;
	ent.cmodeltype = CMODEL_BBOX_ROTATED;
	
	ent.setSize( cVec3( -radius, -radius, -radius ), cVec3( radius, radius, radius ) );

	ent.moveType = MOVE_TYPE_NONE;
	ent.netFlags &= ~SVF_NOCLIENT;
	ent.modelindex1 = game.modelIndex( "models/items/ammo/pack/pack.md3" );
	ent.nextThink = levelTime + 1000;
	ent.linkEntity();
}



void GT_Frame()
{
	for( uint i = 0; i < game.numEntities; i++ )
	{
		cEntity @ent = @game.getEntity( i );
		if( !ent.inuse )
			continue;

		if( ent.className.len() == 0 )
			continue;

		//G_Print( ent.classname );
		if( ent.className == "client" )
		{
			cVec3 origin;
			
			origin = ent.getOrigin();
			//G_Print( "client origin: " + ent.origin.x + ", " + ent.origin.y + ", " + ent.origin.z + "\n" );
			//ent.origin.z = 0;
			cVec3 testv( 0, 0, 0 );
			testv.z = 1;

			ent.setOrigin( origin );
			
		}
	}
}

void GT_InitGametype()
{
	G_Print( "Gametype initialized\n" );
}
