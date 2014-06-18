#include "sdleditor.hxx"

int main() {
	const char * const tilemap = "sprite32.bmp";
	const char * const mapfile1 = "gamemap.tmap";

	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.mapSize = point(40,40);
	conf.spriteViewport = point(10,20);
	conf.init();

	input in;
	graphics tiles(
		tilemap,
		point(conf.toPixel(30), conf.toPixel(21)),
		point(100, 100),
		SDL_WINDOW_SHOWN,
		SDL_RENDERER_PRESENTVSYNC);

	map gamemap(mapfile1);

	editor ed(&in, &tiles, &gamemap, point(16,0));
	tiles.flip();

	do {
		ed.handleEvents();
		SDL_Delay(conf.delay);
	} while (ed.running);

	gamemap.write(mapfile1);
	return 0;
}
