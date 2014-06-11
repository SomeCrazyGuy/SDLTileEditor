#ifndef SDLEDITOR_H
#define SDLEDITOR_H
#include <SDL2/SDL.h>

#define RECT(X,Y,W,H) (SDL_Rect){.x=(X),.y=(Y),.w=(W),.h=(H)}
#define MAX(X,Y) (((X)<(Y))?(Y):(X))

namespace {

class point {
	public:
	int x, y;
	point(){}
	point(int w, int h): x(w), y(h) {}
	point(point xy, point wh): x(xy.x + wh.x), y(xy.y + wh.y) {}
};

class config {
	public:
	char *filename;
	char *savename;
	int fps, delay;
	int shift, tileSize;
	point editorSize;
	point spriteMapSize;
	bool canRestore;

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
	int size;
	short width;
	short height;
	short origin_x;
	short origin_y;
	short data[];
};

class mapformat {
	public:
	static short toShort(const point xy) {
		return ((xy.x << 8) + xy.y);
	}
	static point toPoint(const short data) {
		return point((data >> 8), (data & 0xff));
	}
};

class map {
	//TODO: incorporate origin tracking into get/set/restore/etc

	tmap* tileMap;
	FILE* restoreFile;
	static const int size = sizeof(struct tmap);

	public:
	map() {
		if(this->canRestore(conf.savename)) {
			conf.canRestore = true;
			restore();
		} else {
			create(conf.editorSize);
		}
	}

	void create(point size) {
		tmap* l;
		int tmap_size = (this->size + ((size.x * size.y) * 2));

		l = (tmap*) malloc(tmap_size);
		*((int*)l->magic) = *(int*)"Tmap";
		l->width = (short) (size.x & 0xffff);
		l->height = (short) (size.y & 0xffff);
		l->origin_x = 0;
		l->origin_y = 0;
		l->size = tmap_size;
		this->tileMap  = l;
	}

	void restore() {
		tmap* l;
		l = (tmap*) malloc(this->size);
		fread(l, this->size, 1, this->restoreFile);
		l = (tmap*) realloc(l, l->size);
		fread(&(l->data), (l->size - this->size), 1, this->restoreFile);
		this->tileMap = l;
		fclose(this->restoreFile);
	}

	~map() {
		free(this->tileMap);
	}

	short offset(const point loc) {
		tmap* x = this->tileMap;
		return ((x->width * (loc.y + x->origin_y)) + (loc.x + x->origin_y));
	}

	void setOrigin(point xy) {
		this->tileMap->origin_x = ((short) xy.x & 0xffff);
		this->tileMap->origin_y = ((short) xy.y & 0xffff);
	}

	point getOrigin() {
		return point(this->tileMap->origin_x, this->tileMap->origin_y);
	}

	bool canRestore(const char * const filename) {
		this->restoreFile = fopen(filename, "r");
		return this->restoreFile;
	}

	void write(const char * const filename) {
		FILE * tmp = fopen(filename, "w");
		fwrite(this->tileMap, this->tileMap->size, 1, tmp);
		fclose(tmp);
	}

	point get(point loc) {
		return mapformat::toPoint(this->tileMap->data[this->offset(loc)]);
	}

	void put(point loc, point data) {
		this->tileMap->data[this->offset(loc)] = mapformat::toShort(data);
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
	bool leftClick;
	bool rightClick;
	bool enterPressed;
	char hjkl_event;
	int skippedEvents;

	void run() {
		skippedEvents = 0;
		running = true;
		leftClick = false;
		rightClick = false;
		mouseUp = point(-1, -1);
		enterPressed = false;
		hjkl_event = 0;

		while(SDL_PollEvent(&e)) {
			switch(e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_MOUSEBUTTONUP:
					this->mouse_input();
					break;
				case SDL_KEYUP:
					this->keyboard_input();
					break;
				default: ++skippedEvents;
			}
		}
	}

	void keyboard_input() {
		switch (e.key.keysym.sym) {
			case SDLK_SPACE:
			case SDLK_RETURN:
				this->enterPressed = true;
				break;

			case SDLK_q:
			case SDLK_ESCAPE:
				this->running = false;
				break;

			case SDLK_h:
				hjkl_event = 'h';
				break;
			case SDLK_j:
				hjkl_event = 'j';
				break;
			case SDLK_k:
				hjkl_event = 'k';
				break;
			case SDLK_l:
				hjkl_event = 'l';
				break;
			default:
				break;
		}
	}

	void mouse_input() {
		mouseUp = point(conf.toTile(e.button.x), conf.toTile(e.button.y));
		if (e.button.button == SDL_BUTTON_LEFT) {
			leftClick = true;
		} else {
			rightClick = true;
		}
	}
};

class editor {
	input* i;
	graphics* g;
	map* m;

	public:
	point cursor;
	point selection;

	editor(input* a, graphics* b, map* c): i(a), g(b), m(c) {
		if(conf.canRestore) {
			this->restore();
		} else {
			this->fill(point(0,0));
		}
		this->cursor = point(0,0);
		this->selection = point(0,0);
	}

	void fill(point fillTile) {
		for(int i=0; i<conf.editorSize.x; ++i) {
			for(int j=0; j<conf.editorSize.y; ++j) {
				m->put(point(j,i), fillTile);
				g->copyTile(fillTile, point(j,i));
			}
		}
	}

	void restore() {
		point loc;
		for(int i=0; i<conf.editorSize.x; ++i) {
			for(int j=0; j<conf.editorSize.y; ++j) {
				loc = point(j,i);
				g->copyTile(m->get(loc), loc);
			}
		}
	}

	void draw() {
		m->put(this->cursor, this->selection);
		g->copyTile(this->selection, this->cursor);

		if(++cursor.x == conf.editorSize.x) {
			cursor.x = 0;
			++cursor.y;
			if(cursor.y == conf.editorSize.y) {
				cursor.y = 0;
			}
		}

		g->flip();
	}
};

}
#endif //SDLEDITOR_H