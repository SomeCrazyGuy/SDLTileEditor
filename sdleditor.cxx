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

	editor ed(&in, &tiles, &saveFile, point(16,0));
	tiles.flip();

	int ed_x = conf.editorSize.x;
	int ed_y = conf.editorSize.y;
	bool restore;

	do {
		in.run();
		restore = false;
		if(in.leftClick || in.rightClick) {
			if(in.mouseUp.x < ed_x) {
				ed.moveCursor(in.mouseUp);
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
					restore = true;
					break;
				case SDLK_j:
					saveFile.moveOrigin(point(0,NEGATIVE(ed_y/2)));
					restore = true;
					break;
				case SDLK_k:
					saveFile.moveOrigin(point(0, ed_y/2));
					restore = true;
					break;
				case SDLK_l:
					saveFile.moveOrigin(point(ed_x/2, 0));
					restore = true;
					break;
				case SDLK_f:
					ed.fill(ed.selection);
					tiles.flip();
					break;
				case SDLK_r:
					restore = true;
					break;
				case SDLK_w:
					ed.moveCursor(point(ed.cursor.x, ed.cursor.y - 1));
					break;
				case SDLK_a:
					ed.moveCursor(point(ed.cursor.x - 1, ed.cursor.y));
					break;
				case SDLK_s:
					ed.moveCursor(point(ed.cursor.x, ed.cursor.y + 1));
					break;
				case SDLK_d:
					ed.moveCursor(point(ed.cursor.x + 1, ed.cursor.y));
					break;
				default:
					break;
			}
			if(restore) {
				ed.restore();
				ed.drawCursor();
				tiles.flip();
			}
		}

		SDL_Delay(conf.delay);
	} while(in.running);

	saveFile.write(conf.savename);
	return 0;
}
