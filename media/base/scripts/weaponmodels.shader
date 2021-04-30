
//WEAPONS

models/v_weapons/electrobolt/electrobolt
{
	nopicmip
	cull front

	{
		map models/weapons/electrobolt/electrobolt.tga
		rgbGen identity
	}

	//for 3d cards supporting cubemaps
	if textureCubeMap
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/glauncher/glauncher
{
	nopicmip
	cull front

	{
		map models/weapons/glauncher/glauncher.tga
		rgbGen identity
	}
	
	if textureCubeMap // for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif
	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/plasmagun/plasmagun
{
	nopicmip
	cull front

	{
		map models/weapons/plasmagun/plasmagun.tga
		rgbGen identity
	}

	if textureCubeMap	//for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/riotgun/riotgun
{
	nopicmip
	cull front

	{
		map models/weapons/riotgun/riotgun.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif
	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/gunblade/gunblade
{
	nopicmip
	cull front

	{
		map models/weapons/gunblade/gunblade.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif
	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/gunblade/barrel
{
	nopicmip
	cull front

	{
		map models/weapons/gunblade/barrel.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif
	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/rlauncher/rlauncher
{
	nopicmip
	cull front

	{
		map models/weapons/rlauncher/rlauncher.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}
models/v_weapons/lasergun/lasergun
{
	nopicmip
	cull front

	{
		map models/weapons/lasergun/lasergun.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif
	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

//lasergun temp shaders
models/v_weapons/lasergun/lg1_temp
{
	nopicmip
	cull front

	{
		map models/weapons/lasergun/lg1_temp.tga
		rgbGen identity
	}
	
	if textureCubeMap // for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}

	endif

	if ! textureCubeMap // for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/lasergun/lg2_temp
{
	nopicmip
	cull front

	{
		map models/weapons/lasergun/lg2_temp.tga
		rgbGen identity
	}
	
	if textureCubeMap // for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap // for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

// NOTE: *** This is the machinegun using the old Plasmagun Model ****
models/v_weapons/plasmagun/pg_main
{
	nopicmip
	cull front

	{
		map models/weapons/machinegun/machinegun_main.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/plasmagun/pg_canon
{
	nopicmip
	cull front

	{
		map models/weapons/machinegun/machinegun_canon.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

models/v_weapons/plasmagun/pg_wires
{
	nopicmip
	cull front

	{
		map models/weapons/machinegun/machinegun_wires.tga
		rgbGen identity
	}
	
	if textureCubeMap //for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		rgbGen identity
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
	endif
}

// NOTE: *** This is the machinegun using the old Plasmagun Model ****


//FLASH WEAPONS

models/v_weapons/generic/f_generic
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/generic/f_generic.tga
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/v_weapons/plasmagun/f_plasma
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/plasmagun/f_plasma.tga
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/v_weapons/plasmagun/f_plasma_2
{
	nopicmip
	sort additive
	cull disable
	deformVertexes autosprite2
	{
		map models/weapons/plasmagun/f_plasma_2.tga
		rgbgen entity
		blendFunc add
	}
}

models/v_weapons/glauncher/f_glaunch
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/glauncher/f_glaunch.tga
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/v_weapons/riotgun/f_riot
{
	nopicmip
	sort additive
	cull disable
	{
		map models/weapons/riotgun/f_riot.tga
		rgbgen entity
		blendfunc add
		tcmod rotate 200
	}
}

models/v_weapons/gunblade/f_gunblade
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/gunblade/f_gunblade.tga
	rgbgen entity
	tcmod rotate 200
	blendFunc add
	}
	{
	map models/weapons/gunblade/f_gunblade_1.tga
	rgbgen entity
	tcmod rotate -175
	blendFunc add
	}
}

models/v_weapons/rlauncher/f_rlaunch
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/rlauncher/f_rlaunch.tga
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/v_weapons/rlauncher/f_rlaunch_2
{
	nopicmip
	sort additive
	cull disable
	deformVertexes autosprite2
	{
		map models/weapons/rlauncher/f_rlaunch_2.tga
		rgbgen entity
		blendFunc add
	}
}

models/v_weapons/electrobolt/f_electro
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/electrobolt/f_electro.tga
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/v_weapons/electrobolt/f_electro_2
{
	nopicmip
	sort additive
	cull disable
	deformVertexes autosprite2
	{
		map models/weapons/electrobolt/f_electro_2.tga
		rgbgen entity
		blendFunc add
	}
}

models/v_weapons/lasergun/f_laser
{
	nopicmip
	sort additive
	cull disable
	{
	map models/weapons/lasergun/f_laser.tga
	rgbgen entity
	tcmod rotate 180
	blendFunc add
	}
}

