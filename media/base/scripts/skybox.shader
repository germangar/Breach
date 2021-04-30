textures/skybox/skyfogged1
{
	qer_editorimage env/skyfogged1/hull_pz.tga
	surfaceparm nomarks
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_sun .9 .9 .9  100 130 35
	q3map_lightimage textures/colors/white.tga
	q3map_surfacelight 10

	skyParms env/skyfogged1/hull 256 -
}

textures/skybox/skyfogged1_fog
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