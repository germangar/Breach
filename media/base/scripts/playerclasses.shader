// ==================
// ENGINEER DIRECTORY
// ==================

textport_padpork_head
{
if GLSL

      {
		material models/players/engineer/padpork_head
		rgbgen identity

      }

endif

if ! GLSL

	{
		map $whiteImage
		rgbgen entity
	}

      {
		map models/players/engineer/padpork_head
		rgbgen lightingDiffuse
		blendFunc filter
      }

endif
}

textport_padpork_torso
{
if GLSL

      {
		material models/players/engineer/padpork_torso
		rgbgen entity

      }

endif

if ! GLSL

	{
		map $whiteImage
		rgbgen entity
	}

      {
		map models/players/engineer/padpork_torso
		rgbgen lightingDiffuse
		blendFunc filter
      }

endif
}

textport_padpork_legs
{
if GLSL

      {
		material models/players/engineer/padpork_legs
		rgbgen entity

      }

endif

if ! GLSL

	{
		map $whiteImage
		rgbgen entity
	}

      {
		map models/players/engineer/padpork_legs
		rgbgen lightingDiffuse
		blendFunc filter
      }

endif
}


// ==================
// SNIPER DIRECTORY
// ==================

monada_bas
{
if GLSL

      {
		material models/players/sniper/monada_bas
		rgbgen entity

      }

endif

if ! GLSL

	{
		map $whiteImage
		rgbgen entity
	}

      {
		map models/players/sniper/monada_bas
		rgbgen lightingDiffuse
		blendFunc filter
      }

endif
}

monada_haut
{
if GLSL

      {
		material models/players/sniper/monada_haut
		rgbgen entity

      }

endif

if ! GLSL

	{
		map $whiteImage
		rgbgen entity
	}

      {
		map models/players/sniper/monada_haut
		rgbgen lightingDiffuse
		blendFunc filter
      }

endif
}

// ==================
// SOLDIER DIRECTORY
// ==================

//models/players/bigvic/bigvic_head
vic_head_diff02
{
	nopicmip
	cull front
	
	if GLSL

	{
		map $whiteImage
		rgbGen identity
	}

	{
		material  models/players/soldier/vic_head_diff02.tga models/players/soldier/vic_head_norm.tga
		rgbGen identityLighting
		blendFunc filter
	}
	endif

	if ! GLSL

	// shadow
	//for 3d cards supporting cubemaps
	if textureCubeMap
	{
		shadecubemap env/celldouble
		rgbGen identity
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		rgbGen identity
		tcGen environment
	}
	endif

	//tint pass
	// there is no tint for the head

	{
		map models/players/soldier/vic_head_diff02.tga
		rgbGen identityLighting
		blendFunc blend
	}

	// light
	if textureCubeMap
	{
		shadecubemap env/celllight
		rgbGen identityLighting
		blendFunc add
	}
	endif

	endif

	{
		map models/players/soldier/vic_head_eye.tga
		rgbGen identityLighting
		blendFunc add
	}

}

//models/players/bigvic/bigvic_torso
vic_torso_diff01
{
	nopicmip
	cull front

	if GLSL

	{
		map $whiteImage
		rgbGen identity
	}

	{
		map models/players/soldier/vic_torso_color.tga
		rgbGen entity
		blendFunc blend
	}

	{
		material models/players/soldier/vic_torso_diff01.tga models/players/soldier/vic_torso_norm.tga
		rgbGen identityLighting
		blendFunc filter
	}
	endif

	if ! GLSL

	// shadow

	//for 3d cards supporting cubemaps
	if textureCubeMap
	{
		shadecubemap env/celldouble
		rgbGen identityLighting
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		rgbGen identityLighting
		tcGen environment
	}
	endif

	//tint pass
	{
		map models/players/soldier/vic_torso_colorpass.tga
		rgbgen entity
		blendFunc filter
	}

	{
		map models/players/soldier/vic_torso_diff01.tga
		rgbGen identityLighting
		blendFunc blend
	}

	// light
	
	if textureCubeMap
	{
		shadecubemap env/celllight
		rgbGen identityLighting
		blendFunc add
	}
	endif

	endif
}

//models/players/bigvic/bigvic_legs
vic_legs_diff01
{
	nopicmip
	cull front

	if GLSL

	{
		map $whiteImage
		rgbGen identity
	}

	{
		map models/players/soldier/vic_legs_color.tga
		rgbGen entity
		blendFunc blend
	}

	{
		material models/players/soldier/vic_legs_diff01.tga models/players/soldier/vic_legs_norm.tga
		rgbGen identityLighting
		blendFunc filter
	}
	endif

	if ! GLSL

	// shadow

	//for 3d cards supporting cubemaps
	if textureCubeMap
	{
		shadecubemap env/celldouble
		rgbGen identityLighting
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.tga
		rgbGen identityLighting
		tcGen environment
	}
	endif

	//tint pass
	{
		map models/players/soldier/vic_legs_colorpass.jpg
		blendFunc filter
		rgbgen entity
	}

	{
		map models/players/soldier/vic_legs_diff01.tga
		rgbGen identityLighting
		blendFunc blend
	}

	// light
	
	if textureCubeMap
	{
		shadecubemap env/celllight
		rgbGen identityLighting
		blendFunc add
	}
	endif

	endif
}

