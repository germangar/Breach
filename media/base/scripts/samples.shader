// These shaders aren't intended to be used, but as examples of the most typical shaders

textures/terra/terrain_00000
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 512 512
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	surfaceparm dust

	nomodulativeDlights

	if deluxe

	{
	material textures/terrain/ground018.tga textures/terrain/ground018_norm.tga textures/terrain/ground018_gloss.tga
	}

	endif

	
	if ! deluxe

	{
		map $lightmap
		tcGen lightmap
	} 
	{
		map $dlight
		blendfunc add
	}
	{
		map textures/terrain/ground018.tga
		blendFunc filter
	}

	endif
}

textures/basic_material
{
	qer_editorimage textures/basic_material.tga
	surfaceparm metal

	if deluxe

	{
	material textures/basic_material.tga textures/basic_material_norm.tga textures/basic_material_gloss.tga
	}

	endif

	if ! deluxe

	{
		map $lightmap
	} 
	{
		map $dlight
		blendfunc add
	}
	{
		map textures/basic_material.tga
		blendFunc filter
	}

	endif
}

models/mapmodels/terrain/model_without_lightmap
{
	qer_editorimage mapmodels/terrain/model_without_lightmap.tga

	//q3map_novertexshadows
	//q3map_forcesunlight

	surfaceparm nolightmap
	surfaceparm dust
	{
		map models/mapmodels/terrain/model_without_lightmap.tga
		rgbGen vertex
	}
}

models/mapmodels/terrain/model_with_lightmap
{
	qer_editorimage models/mapmodels/terrain/model_with_lightmap.tga
	q3map_forceMeta
	q3map_nonPlanar
	//q3map_lightmapsamplesize 4
	//q3map_splotchFix

	surfaceparm dust
	noModulativeDlights
	
	{
		map $lightmap
	}
	{
		map $dlight
		blendfunc add
	}
	{
		map models/mapmodels/terrain/model_with_lightmap.tga
		blendFunc filter
	}
}
