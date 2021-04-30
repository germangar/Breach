
textures/common/terrain
{
	q3map_terrain
	qer_editorimage textures/common/terrain.tga
	surfaceparm nodraw
	surfaceparm nolightmap
}

textures/common/terrain_dust
{
	q3map_terrain
	qer_editorimage textures/common/terrain.tga
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm dust
}

textures/common/portal
{
	qer_editorimage textures/common/qer_portal.tga
	surfaceparm nolightmap
	portal
	{
		map textures/common/mirror1.tga
		tcMod turb 0 0.25 0 0.05
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
}

textures/common/mirror1
{
	qer_editorimage textures/common/mirror1.tga
	surfaceparm nolightmap
	portal
  
	{
		map textures/common/mirror1.tga
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
}

textures/common/mirror2
{
	qer_editorimage textures/common/qer_mirror.tga
	surfaceparm nolightmap
	portal
	{
		map textures/common/mirror1.tga
		blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		depthWrite
	}
        {
               map textures/sfx/mirror.tga
	       blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
        }

}

textures/fog/somefog
{
	qer_editorimage textures/world/lightcolors/white.tga

	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.4 0.4 0.4 ) 2048
}

