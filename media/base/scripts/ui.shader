2d/console
{
	nopicmip
	nomipmaps
      {
		map $whiteimage
		rgbgen const 0.2 0.1 0.3
	}
}

2d/white
{
      {
		map $whiteimage
		rgbgen vertex
		alphagen vertex
	}
}

2d/ui/videoback
{
	nopicmip
	nomipmaps
	{
		//videomap backsequence.roq
		map $whiteimage
		rgbgen const 0.5 0.5 0.5
	}
	{
    		map 2d/ui/screenlines.tga
		blendFunc GL_ONE_MINUS_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scroll -.1  0
		tcMod scale 4 4
	}
	{
		map $whiteimage
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const 0.3843 0.2902 0.5843
		alphaGen constant 0.5
	}
}

2d/ui/cursor
{
	nopicmip
	nomipmaps
	{
		map 2d/ui/cursor.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

2d/ui/item_default
{
	nopicmip
	nomipmaps
	{
		map 2d/ui/item_default.tga
		rgbgen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

2d/ui/item_default_focus
{
	nopicmip
	nomipmaps
	{
		map 2d/ui/item_default_focus.tga
		rgbgen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

2d/ui/window_default
{
	nopicmip
	nomipmaps
	{
		map 2d/ui/window_default.tga
		rgbgen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

