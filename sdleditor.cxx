#include <SDL2/SDL.h>
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

	editor ed(&in, &tiles, &saveFile);
	tiles.flip();

	int ed_x = conf.editorSize.x;
	int ed_y = conf.editorSize.y;

	do {
		in.run();
		if(in.leftClick || in.rightClick) {
			if(in.mouseUp.x < ed_x) {
				ed.cursor = in.mouseUp;
				if(in.leftClick) {
					ed.draw();
				}
			} else {
				ed.selection = point(in.mouseUp.x - ed_x, in.mouseUp.y);
				ed.draw();
			}
		} else if (in.enterPressed) {
			ed.draw();
		} else if (in.key_event) {
			switch (in.e.key.keysym.sym) {
				case SDLK_h:
					saveFile.moveOrigin(point(NEGATIVE(ed_x/2),0));
					break;
				case SDLK_j:
					saveFile.moveOrigin(point(0,NEGATIVE(ed_y/2)));
					break;
				case SDLK_k:
					saveFile.moveOrigin(point(0, ed_y/2));
					break;
				case SDLK_l:
					saveFile.moveOrigin(point(ed_x/2, 0));
					break;
				case SDLK_f:
					ed.fill(ed.selection);
					break;
				case SDLK_w:
					break;
				case SDLK_a:
					break;
				case SDLK_s:
					break;
				case SDLK_d:
					break;
				default:
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
