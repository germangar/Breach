
textures/water/calmwater_darken
{
	qer_editorimage textures/water/water_gloss.jpg
	q3map_globaltexture
	qer_trans .75
	surfaceparm trans
	surfaceparm nonsolid
	//surfaceparm water
	surfaceparm nodlight
	
	q3map_lightmapsamplesize 64

	{
		map $lightmap
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbgen const 1 .8 .4
	}

	{
		map $lightmap
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbgen const 1 .8 .4
	}

//	{
//		map $whiteImage
//		rgbgen const 0 0.05 0.15
//		alphagen const 0.9
//		blendFunc blend
//	}
}

textures/water/calmwater
{
	qer_editorimage textures/water/water_gloss.jpg
	q3map_globaltexture
	qer_trans .75
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nodlight
	q3map_lightmapsamplesize 64

	if GLSL && portalMaps

	portal

	{
		distortion textures/water/water5_DUDV textures/water/water5_norm
		alphagen const 0.45
		rgbgen const 0.13 0.17 0.16
		tcmod scale 0.25 0.25
		tcmod scroll -0.05 -0.05
	}

	endif

	if ! GLSL || ! portalMaps

	{
		map textures/water/water_gloss.jpg
		blendFunc GL_dst_color GL_one
		rgbgen identity
		tcmod scale .25 .25
		tcmod scroll .02 .01
	}

	{
		map $whiteImage
		rgbgen const .0 .3 .4
		alphagen const 0.25
		blendfunc blend
	}

	endif
}


textures/water/testwater2
{
	qer_editorimage textures/water/water_gloss.jpg
	q3map_globaltexture
	qer_trans .75
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nodlight
	q3map_lightmapsamplesize 64

	if GLSL && portalMaps

	portal

	{
		distortion textures/water/water_norm2b textures/water/water_norm2b
		alphagen const 0.5
		//rgbgen const 0.01 0.05 0.05
		rgbgen const 0.15 0.19 0.18
		tcmod scale 0.075 0.075
		tcmod scroll 0.01 -0.005
	}

	endif

	if ! GLSL || ! portalMaps

	{
		map textures/water/water_gloss.jpg
		blendFunc GL_dst_color GL_one
		rgbgen identity
		tcmod scale .25 .25
		tcmod scroll .02 .01
	}

	{
		map $whiteImage
		rgbgen const .0 .3 .4
		alphagen const 0.25
		blendfunc blend
	}

	endif
}


