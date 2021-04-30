// use a base shader to assign settings to all mapmodels
// this allows me to change their lightmap quality to
// speed up WIP compile times

//terrainmodel_baseshader
//{
//	q3map_forceMeta
//	q3map_nonPlanar
//	q3map_clipmodel
//	q3map_lightmapMergable

	// lightmap quality settings
//	q3map_lightmapSampleOffset 32
//	q3map_lightmapsamplesize 16
//}

//objectmodel_baseshader
//{
//	q3map_forceMeta
//	q3map_nonPlanar
//	q3map_clipmodel

	// lightmap quality settings
//	q3map_lightmapSampleOffset 16
//	q3map_lightmapsamplesize 8
//}

lambert1
{
	qer_editorimage textures/common/lambert

	{
		map $lightmap
	} 
	{
		map $dlight
		blendfunc add
	}
	{
		map textures/wip/ter256x256
		blendFunc filter
	}
}

textures/terrain/mud_dry_rivered_512x512_terrain
{
	qer_editorimage textures/terrain/mud_dry_rivered_512x512.tga
	q3map_lightmapaxis z
	//q3map_lightmapsamplesize 16
	//q3map_lightmapSampleOffset 32
	q3map_lightmapsamplesize 64
	q3map_lightmapSampleOffset 64
	surfaceparm dust

	if deluxe

	{
		material textures/terrain/mud_dry_rivered_512x512
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
		map textures/terrain/mud_dry_rivered_512x512
		blendFunc filter
	}

	endif
}

textures/terrain/mud_dry_rivered_512x512_vertex
{
	qer_editorimage textures/terrain/mud_dry_rivered_512x512.tga
	q3map_lightmapaxis z
	//q3map_lightmapsamplesize 16
	//q3map_lightmapSampleOffset 32
	q3map_lightmapsamplesize 64
	q3map_lightmapSampleOffset 64
	surfaceparm dust

	if deluxe

	{
		material textures/terrain/mud
	}

	{
		material textures/terrain/rocks001_512x512
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphagen vertex
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
		map textures/terrain/mud_dry_rivered_512x512
		blendFunc filter
	}

	endif
}
