
textures/world/quicksky_strong
{
	qer_editorimage textures/world/sh/quicksky.tga
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	q3map_sun .9 .9 .9  120 130 35
	q3map_lightimage textures/colors/white.tga
	q3map_surfacelight 5
	skyParms - 512 -
	{
		map textures/world/sh/quicksky.tga
		tcMod scale 2 2
		tcMod scroll 0.1 0.1
	}
}

textures/world/lava
{
	qer_editorimage textures/world/sh/lava2.tga
	q3map_globaltexture
	surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 200
	tesssize 64

	deformVertexes wave 100 sin 3 2 .1 0.1
	
	{
		map textures/world/sh/lava1_glow.tga
		tcmod scroll .005 .005
		tcMod turb 0 .1 0 .05
		blendFunc GL_ONE GL_ZERO
		rgbGen identity
	}
	{
		map textures/world/sh/lava2.tga
		blendfunc add
		tcMod turb 0 .2 0 .1
		tcmod scroll .0025 .0025
	}
}

textures/world/blueportalwater
{
	qer_editorimage textures/wazzz/wat_gloss.jpg
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
		distortion textures/wazzz/wat_norm2 textures/wazzz/norm
		alphagen const 0.5
		rgbgen const 0.01 0.05 0.05
		tcmod scale 0.3 0.3
		tcmod scroll 0.04 -0.01
	}


	{
		material $whiteimage textures/wazzz/norm textures/wazzz/wat_gloss
		blendfunc GL_ONE GL_DST_COLOR
		tcmod scroll 0.04 -0.01
		rgbgen const 0.25 0.25 0.25
	}

	endif

	if ! GLSL || ! portalMaps

	{
		map textures/world/sh/watercaustic2.tga
		blendFunc GL_dst_color GL_one
		rgbgen identity
		tcmod scale .25 .25
		tcmod scroll .02 .01
	}

	{
		map textures/world/sh/watercaustic1.tga
		blendFunc GL_dst_color GL_one
		tcmod scale -.25 -.25
		tcmod scroll .02 .02
	}

	{
		map $whiteImage
		rgbgen const .0 .3 .4
		alphagen const 0.25
		blendfunc blend
	}

	endif
}

textures/world/blueportalwater_blend
{
	qer_editorimage textures/common/nodraw.tga
	{
		map $whiteImage
		rgbgen const .0 .3 .4
		alphagen const 0.3
		blendfunc blend
	}
}

