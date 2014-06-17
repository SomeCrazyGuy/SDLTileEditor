#include "sdleditor.hxx"

int main() {
	const char * const tilemap = "sprite32.bmp";
	const char * const mapfile1 = "layer1.tmap";
	const char * const mapfile2 = "layer2.tmap";


	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.mapSize = point(40,40);
	conf.spriteViewport = point(10,10);
	conf.init();

	input in;
	graphics tiles(tilemap);
	map layer1(mapfile1);
	map layer2(mapfile2);


	editor ed(&in, &tiles, &layer1, &layer2, point(16,0));
	tiles.flip();

	do {
		ed.handleEvents();
		SDL_Delay(conf.delay);
	} while (ed.running);

	layer1.write(mapfile1);
	layer2.write(mapfile2);
	return 0;
}
