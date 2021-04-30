textures/world/quicksky
{
	qer_editorimage env/quickfog/hull_pz.tga
	surfaceparm nomarks
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_sun .9 .9 .9  100 130 35
	q3map_lightimage textures/colors/white.tga
	q3map_surfacelight 10

	skyParms env/quickfog/hull 256 -
}

textures/world/quickfog
{
	qer_editorimage textures/colors/white.tga

	surfaceparm fog
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.5 0.5 0.5 ) 7000 2000
}

textures/world/quickfog_hull
{
	qer_editorimage env/quickfog/hull_pz.tga
	surfaceparm noimpact
	surfaceparm nolightmap
	skyParms env/quickfog/hull 256 -
}


textures/terra/terrain_0
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	noModulativeDlights
	surfaceparm dust

	if ! deluxe

	{
		map textures/terrain/desertrock1.jpg
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif

	if deluxe

	{
		material textures/terrain/desertrock1.jpg textures/terrain/desertrock1_norm.jpg
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}

	endif
}

textures/terra/terrain_1
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	noModulativeDlights

	if ! deluxe

	{
		map textures/terrain/limestone.jpg
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif

	if deluxe

	{
		material textures/terrain/limestone.jpg textures/terrain/limestone_norm.jpg
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}

	endif
}

textures/terra/terrain_2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	noModulativeDlights
	surfaceparm dust

	if ! deluxe

	{
		map textures/terrain/mud.jpg
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif

	if deluxe

	{
		material textures/terrain/mud.jpg textures/terrain/mud_norm.jpg
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}

	endif
}

textures/terra/terrain_0to1
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	surfaceparm dust
	
	if ! deluxe

	{
		map textures/terrain/desertrock1.jpg
	}
	{
		map textures/terrain/limestone.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif


	if deluxe

	{
		material textures/terrain/desertrock1.jpg textures/terrain/desertrock1_norm.jpg
	}

	{
		material textures/terrain/limestone.jpg textures/terrain/limestone_norm.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}
	endif
}


textures/terra/terrain_0to2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	surfaceparm dust
	
	if ! deluxe

	{
		map textures/terrain/desertrock1.jpg
	}
	{
		map textures/terrain/mud.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif


	if deluxe

	{
		material textures/terrain/desertrock1.jpg textures/terrain/desertrock1_norm.jpg
	}

	{
		material textures/terrain/mud.jpg textures/terrain/mud_norm.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}

	endif
}


textures/terra/terrain_1to2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	surfaceparm dust
	
	if ! deluxe

	{
		map textures/terrain/limestone.jpg
	}
	{
		map textures/terrain/mud.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}

	endif


	if deluxe

	{
		material textures/terrain/limestone.jpg textures/terrain/limestone_norm.jpg
	}

	{
		material textures/terrain/mud.jpg textures/terrain/mud_norm.jpg
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}

	{
		clampmap $dlight
		depthFunc equal
		blendFunc GL_DST_COLOR GL_ONE
	}

	endif
}


textures/terra/terrain.vertex
{
	{
		map textures/terrain/desertrock1.jpg
		rgbGen vertex
	}
}