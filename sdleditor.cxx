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

	//point saveOrigin = saveFile.getOrigin();

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
					saveFile.setOrigin(point(saveFile.getOrigin(), point((conf.editorSize.x/2)*-1, 0)));
					break;
				case 'j':
					saveFile.setOrigin(point(saveFile.getOrigin(), point(0,(conf.editorSize.y/2)*-1)));
					break;
				case 'k':
					saveFile.setOrigin(point(saveFile.getOrigin(), point(0, (conf.editorSize.y/2))));
					break;
				case 'l':
					saveFile.setOrigin(point(saveFile.getOrigin(), point((conf.editorSize.x/2), 0)));
					break;
			}
			ed.restore();
			tiles.flip();
		}

		SDL_Delay(30);
	} while(in.running);

	saveFile.write(conf.savename);
	return 0;
}
