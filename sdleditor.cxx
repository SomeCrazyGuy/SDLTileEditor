#include <SDL2/SDL.h>

#define RECT(X,Y,W,H) (SDL_Rect){.x=(X),.y=(Y),.w=(W),.h=(H)}
#define MAX(X,Y) (((X)<(Y))?(Y):(X))

namespace {

class point {
	public:
	int x, y;
	point(){}
	point(int w, int h): x(w), y(h) {}
};

class config {
	public:
	char *filename;
	int fps, delay;
	int shift, tileSize;
	point editorSize;
	point spriteMapSize;

	void init() {
		this->delay = ( 1000 / this->fps );
		this->shift = toShift(this->tileSize);
	}

	int toTile(const int pixelVal) {
		return (pixelVal >> this->shift);
	}

	int toPixel(const int tileVal) {
		return (tileVal << this->shift);
	}

	point toTilexy(const int x, const int y) {
		return point(this->toTile(x), this->toTile(y));
	}

	int toShift(int pixelSize) {
		int ret = 0;
		while (pixelSize >>= 1) { ++ret; }
		return ret;
	}
};
config conf;

struct tmap {
	char magic[4];
	short width;
	short height;
	int size;
	short padding[2];
	short data[];
};

class map {
	struct tmap* tileMap;

	public:
	map(point size) {
		int tmap_size = (sizeof(struct tmap) + ((size.x * size.y) * 2));
		this->tileMap = (struct tmap*) malloc(tmap_size);
		*((int*)this->tileMap->magic) = *(int*)"Tmap";
		this->tileMap->width = (short) (size.x & 0xffff);
		this->tileMap->height = (short) (size.y & 0xffff);
		this->tileMap->size = tmap_size;
	}

	map(const char * const filename) {
		FILE *tmp = fopen(filename, "rw");
		this->tileMap = (struct tmap*) malloc(sizeof(struct tmap));
		fread(this->tileMap, sizeof(struct tmap), 1, tmp);
		this->tileMap = (struct tmap*) realloc(this->tileMap, this->tileMap->size);
		fread(&(this->tileMap->data), (this->tileMap->size - sizeof(struct tmap)), 1, tmp);
		fclose(tmp);
	}

	void write(const char * const filename) {
		FILE * tmp = fopen(filename, "w");
		fwrite(this->tileMap, this->tileMap->size, 1, tmp);
		fclose(tmp);
	}

	point get(point loc) {
		short ret = this->tileMap->data[(this->tileMap->width * loc.y) + loc.x];
		return point(ret >> 8, ret & 0xff);
	}

	void put(point loc, point data) {
		this->tileMap->data[(this->tileMap->width * loc.y) + loc.x] = ((data.x << 8) + (data.y));
	}
};


class graphics {
	SDL_Window* win;
	SDL_Renderer* ren;
	SDL_Texture* tex;

	public:
	graphics() {
		SDL_Init(SDL_INIT_EVERYTHING);

		SDL_Surface* tmp = SDL_LoadBMP(conf.filename);
		conf.spriteMapSize = conf.toTilexy(tmp->w, tmp->h);

		this->win = SDL_CreateWindow(
			"Editor",
			100,
			100,
			conf.toPixel(conf.editorSize.x + conf.spriteMapSize.x),
			conf.toPixel(MAX(conf.editorSize.y, conf.spriteMapSize.y)),
			SDL_WINDOW_SHOWN
		);

		this->ren = SDL_CreateRenderer(
			this->win,
			-1, //auto choose any suitable renderer
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
		);

		this->tex = SDL_CreateTextureFromSurface(this->ren, tmp);
		SDL_FreeSurface(tmp);

		//clear the framebuffer, draw the spritemap and display
		SDL_SetRenderDrawColor(this->ren, 0, 0, 0, 255);
		this->clear();
		this->copy(
			RECT(0, 0, conf.toPixel(conf.spriteMapSize.x), conf.toPixel(conf.spriteMapSize.y)),
			RECT(conf.toPixel(conf.editorSize.x), 0, conf.toPixel(conf.spriteMapSize.x), conf.toPixel(conf.spriteMapSize.y))
			);
		this->flip();
	}

	~graphics() {
		SDL_DestroyTexture(this->tex);
		SDL_DestroyRenderer(this->ren);
		SDL_DestroyWindow(this->win);
		SDL_Quit();
	}

	void copy(const SDL_Rect src, const SDL_Rect dest) {
		SDL_RenderCopy(this->ren, this->tex, &src, &dest);
	}

	void clear() {
		SDL_RenderClear(this->ren);
	}

	void flip() {
		SDL_RenderPresent(this->ren);
	}

	void copyTile(const point src, const point dest) {
		const int x = conf.tileSize;
		static SDL_Rect srcRect = RECT(0, 0, x, x);
		static SDL_Rect destRect = RECT(0, 0, x, x);

		srcRect.x = conf.toPixel(src.x);
		srcRect.y = conf.toPixel(src.y);

		destRect.x = conf.toPixel(dest.x);
		destRect.y = conf.toPixel(dest.y);

		this->copy(srcRect, destRect);
	}


};

class input {
	public:
	SDL_Event e;
	point mouseUp;
	bool running;
	bool mouseClick;
	bool enterPressed;
	int skippedEvents;

	void run() {
		skippedEvents = 0;
		running = true;
		mouseClick = false;
		mouseUp = point(-1, -1);
		enterPressed = false;

		while(SDL_PollEvent(&e)) {
			switch(e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_MOUSEBUTTONUP:
					mouseClick = true;
					mouseUp = point(conf.toTile(e.button.x), conf.toTile(e.button.y));
					break;
				case SDL_KEYUP:
					switch (e.key.keysym.sym) {
						case SDLK_RETURN:
							enterPressed = true;
							break;
						case SDLK_SPACE:
							enterPressed = true;
							break;
						case SDLK_q:
							running = false;
							break;
						default:
							break;
					}
				default: ++skippedEvents;
			}
		}
	}
};

class editor {
	input* i;
	graphics* g;
	map* m;
	point cursor;

	public:
	editor(input* a, graphics* b, map* c, point cur): i(a), g(b), m(c), cursor(cur) {}

	void fill(point fillTile) {
		for(int i=0; i<conf.editorSize.x; ++i) {
			for(int j=0; j<conf.editorSize.y; ++j) {
				m->put(point(j,i), fillTile);
				g->copyTile(fillTile, point(j,i));
			}
		}
	}

	void saveTile(point srcTile) {
		m->put(cursor, srcTile);
		g->copyTile(srcTile, cursor);

		if(++cursor.x == conf.editorSize.x) {
			cursor.x = 0;
			++cursor.y;
			if(cursor.y == conf.editorSize.y) {
				cursor.y = 0;
			}
		}

		g->flip();
	}

	void setCursor(point curLoc) {
		this->cursor = curLoc;
	}
};

}

int main() {
	conf.filename = (char*)"sprite32.bmp";
	conf.tileSize = 32;
	conf.fps = 30;
	conf.editorSize = point(20,20);
	conf.init();

	input in;
	graphics tiles;
	map saveFile(conf.editorSize);

	editor ed(&in, &tiles, &saveFile, point(0,0));

	ed.fill(point(15,0));
	tiles.flip();

	point selection;

	while(in.running) {
		in.run();
		if(in.mouseClick) {
			if(in.mouseUp.x < conf.editorSize.x) {
				if(in.e.button.button == SDL_BUTTON_LEFT) {
					ed.setCursor(in.mouseUp);
					ed.saveTile(selection);
				} else {
					ed.setCursor(in.mouseUp);
				}
			} else {
				selection = point(in.mouseUp.x - conf.editorSize.x, in.mouseUp.y);
				ed.saveTile(selection);
			}
		} else if (in.enterPressed) {
			ed.saveTile(selection);
		}

		SDL_Delay(30);
	}

	saveFile.write("output.tmap");

	return 0;
}
