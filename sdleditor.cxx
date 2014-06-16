#include "sdleditor.hxx"

int main() {
	const char * const tilemap = "sprite32.bmp";
	const char * const mapfile = "savefile.tmap";

	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.mapSize = point(40,40);
	conf.spriteViewport = point(10,10);
	conf.init();

	input in;
	graphics tiles(tilemap);
	map saveFile(mapfile);

	editor ed(&in, &tiles, &saveFile, point(16,0));
	tiles.flip();

	do {
		ed.handleEvents();
		SDL_Delay(conf.delay);
	} while (ed.running);

	saveFile.write(mapfile);
	return 0;
}
