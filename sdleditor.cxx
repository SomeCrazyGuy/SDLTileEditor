#include <SDL2/SDL.h>
#include "sdleditor.hxx"

int main() {
	conf.filename = (char*)"sprite32.bmp";
	conf.savename = (char*)"savefile.tmap";
	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.init();

	input in;
	graphics tiles;
	map saveFile;

	editor ed(&in, &tiles, &saveFile);
	tiles.flip();

	do {
		in.run();
		if(in.leftClick || in.rightClick) {
			if(in.mouseUp.x < conf.editorSize.x) {
				ed.cursor = in.mouseUp;
				if(in.leftClick) {
					ed.draw();
				}
			} else {
				ed.selection = point(in.mouseUp.x - conf.editorSize.x, in.mouseUp.y);
				ed.draw();
			}
		} else if (in.enterPressed) {
			ed.draw();
		} else if (in.hjkl_event) {
			switch (in.hjkl_event) {
				case 'h':
					saveFile.moveOrigin(point(-10,0));
					break;
				case 'j':
					saveFile.moveOrigin(point(0,-10));
					break;
				case 'k':
					saveFile.moveOrigin(point(0, 10));
					break;
				case 'l':
					saveFile.moveOrigin(point(10, 0));
					break;
			}
			ed.restore();
			tiles.flip();
		}

		SDL_Delay(conf.delay);
	} while(in.running);

	saveFile.write(conf.savename);
	return 0;
}
