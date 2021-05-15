
textures/eX256qf/eX_clangfloor_01b
{
	qer_editorimage textures/eX256qf/eX_clangfloor_01b.tga 

	if deluxe
	{
	material textures/eX256qf/eX_clangfloor_01b.tga textures/eX256qf/eX_clangfloor_01_norm.tga textures/eX256qf/eX_clangfloor_01_gloss.tga
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
		map textures/eX256qf/eX_clangfloor_01b.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eX_cretebase_02
{
	qer_editorimage textures/eX256qf/eX_cretebase_02.tga 

	if deluxe
	{
	material textures/eX256qf/eX_cretebase_02.tga textures/eX256qf/eX_cretebase_01_norm.tga textures/eX256qf/eX_cretebase_01_gloss.tga
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
		map textures/eX256qf/eX_cretebase_02.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eX_cretebase_03_dark
{
	qer_editorimage textures/eX256qf/eX_cretebase_03_dark.tga 

	if deluxe
	{
	material textures/eX256qf/eX_cretebase_03_dark.tga textures/eX256qf/eX_cretebase_01_norm.tga textures/eX256qf/eX_cretebase_01_gloss.tga
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
		map textures/eX256qf/eX_cretebase_03_dark.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eX_light_u201
{
	qer_editorimage textures/eX256qf/eX_light_u201.tga 

	if deluxe
	{
	material textures/eX256qf/eX_light_u201.tga textures/eX256qf/eX_light_u201_norm.tga textures/eX256qf/eX_light_u201_gloss.tga
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
		map textures/eX256qf/eX_light_u201.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_light_u201_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_light_u201_soft
{
	qer_editorimage textures/eX256qf/eX_light_u201.tga
	q3map_lightimage map textures/eX256qf/eX_light_u201_blend2.tga
	q3map_surfacelight 5000

	if deluxe
	{
	material textures/eX256qf/eX_light_u201.tga textures/eX256qf/eX_light_u201_norm.tga textures/eX256qf/eX_light_u201_gloss.tga
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
		map textures/eX256qf/eX_light_u201.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_light_u201_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_light_u201_strong
{
	qer_editorimage textures/eX256qf/eX_light_u201.tga
	q3map_lightimage map textures/eX256qf/eX_light_u201_blend2.tga
	q3map_surfacelight 10000

	if deluxe
	{
	material textures/eX256qf/eX_light_u201.tga textures/eX256qf/eX_light_u201_norm.tga textures/eX256qf/eX_light_u201_gloss.tga
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
		map textures/eX256qf/eX_light_u201.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_light_u201_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_light_u201_off
{
	qer_editorimage textures/eX256qf/eX_light_u201.tga 

	if deluxe
	{
	material textures/eX256qf/eX_light_u201.tga textures/eX256qf/eX_light_u201_norm.tga textures/eX256qf/eX_light_u201_gloss.tga
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
		map textures/eX256qf/eX_light_u201.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eX_lightpanel_01
{
	qer_editorimage textures/eX256qf/eX_lightpanel_01.tga 

	if deluxe
	{
	material textures/eX256qf/eX_lightpanel_01.tga textures/eX256qf/eX_lightpanel_01_norm.tga textures/eX256qf/eX_lightpanel_01_gloss.tga
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
		map textures/eX256qf/eX_lightpanel_01.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_lightpanel_01_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_lightpanel_01_soft
{
	qer_editorimage textures/eX256qf/eX_lightpanel_01.tga
	q3map_lightimage textures/eX256qf/eX_lightpanel_01_blend2.tga
	q3map_surfacelight 5000

	if deluxe
	{
	material textures/eX256qf/eX_lightpanel_01.tga textures/eX256qf/eX_lightpanel_01_norm.tga textures/eX256qf/eX_lightpanel_01_gloss.tga
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
		map textures/eX256qf/eX_lightpanel_01.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_lightpanel_01_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_lightpanel_01_strong
{
	qer_editorimage textures/eX256qf/eX_lightpanel_01.tga
	q3map_lightimage textures/eX256qf/eX_lightpanel_01_blend2.tga
	q3map_surfacelight 10000

	if deluxe
	{
	material textures/eX256qf/eX_lightpanel_01.tga textures/eX256qf/eX_lightpanel_01_norm.tga textures/eX256qf/eX_lightpanel_01_gloss.tga
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
		map textures/eX256qf/eX_lightpanel_01.tga
		blendFunc filter
	}
	endif

	{
		map textures/eX256qf/eX_lightpanel_01_blend.tga
		blendFunc add
	}
}

textures/eX256qf/eX_lightpanel_01_off
{
	qer_editorimage textures/eX256qf/eX_lightpanel_01.tga 

	if deluxe
	{
	material textures/eX256qf/eX_lightpanel_01.tga textures/eX256qf/eX_lightpanel_01_norm.tga textures/eX256qf/eX_lightpanel_01_gloss.tga
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
		map textures/eX256qf/eX_lightpanel_01.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eX_wall_01b
{
	qer_editorimage textures/eX256qf/eX_wall_01b.tga 

	if deluxe
	{
	material textures/eX256qf/eX_wall_01b.tga textures/eX256qf/eX_wall_b01_norm.tga textures/eX256qf/eX_wall_b01_gloss.tga
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
		map textures/eX256qf/eX_wall_01b.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eXmetalBase04
{
	qer_editorimage textures/eX256qf/eXmetalBase04.tga 

	if deluxe
	{
	material textures/eX256qf/eXmetalBase04.tga textures/eX256qf/eXmetalBase03_norm.tga textures/eX256qf/eXmetalBase03_gloss.tga
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
		map textures/eX256qf/eXmetalBase04.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eXmetalBase06rust
{
	qer_editorimage textures/eX256qf/eXmetalBase06rust.tga 

	if deluxe
	{
	material textures/eX256qf/eXmetalBase06rust.tga textures/eX256qf/eXmetalBase05Rust_norm.tga textures/eX256qf/eXmetalBase05Rust_gloss.tga
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
		map textures/eX256qf/eXmetalBase06rust.tga
		blendFunc filter
	}
	endif
}

textures/eX256qf/eXmetalBase07rust
{
	qer_editorimage textures/eX256qf/eXmetalBase07rust.tga 

	if deluxe
	{
	material textures/eX256qf/eXmetalBase07rust.tga textures/eX256qf/eXmetalBase05Rust_norm.tga textures/eX256qf/eXmetalBase05Rust_gloss.tga
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
		map textures/eX256qf/eXmetalBase07rust.tga
		blendFunc filter
	}
	endif
}
