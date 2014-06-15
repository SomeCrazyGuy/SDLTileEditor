#include "sdleditor.hxx"

int main() {
	conf.filename = (char*)"sprite32.bmp";
	conf.savename = (char*)"savefile.tmap";
	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.mapSize = point(40,40);
	conf.init();

	input in;
	graphics tiles;
	map saveFile;

	editor ed(&in, &tiles, &saveFile, point(16,0));
	tiles.flip();

	do {
		ed.handleEvents();
		SDL_Delay(conf.delay);
	} while (ed.running);

	saveFile.write(conf.savename);
	return 0;
}
