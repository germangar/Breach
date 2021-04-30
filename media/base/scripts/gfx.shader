
gfx/misc/shadow
{
	nopicmip
	polygonOffset
	{
		clampmap gfx/misc/shadow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}	
}

gfx/misc/colorlaser
{
	nopicmip
	nomipmaps
	cull none
	{
		map gfx/misc/colorlaser.tga
		rgbgen vertex
		blendFunc add
		tcMod scroll 10 0
	}
}

gfx/explosion1
{
	nopicmip
	cull disable
	{
		animmap 8 gfx/explosion1/ex1.tga  gfx/explosion1/ex2.tga gfx/explosion1/ex3.tga gfx/explosion1/ex4.tga gfx/explosion1/ex5.tga gfx/explosion1/ex6.tga gfx/explosion1/ex7.tga textures/colors/black.tga
		rgbGen wave inversesawtooth 0 1 0 8
		blendfunc add
	}
	{
		animmap 8 gfx/explosion1/ex2.tga gfx/explosion1/ex3.tga gfx/explosion1/ex4.tga gfx/explosion1/ex5.tga gfx/explosion1/ex6.tga gfx/explosion1/ex7.tga textures/colors/black.tga textures/colors/black.tga
		rgbGen wave sawtooth 0 1 0 8
		blendfunc add
	}
}
