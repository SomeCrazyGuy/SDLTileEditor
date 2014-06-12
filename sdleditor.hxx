#ifndef SDLEDITOR_H
#define SDLEDITOR_H
#include <SDL2/SDL.h>

#define RECT(X,Y,W,H) (SDL_Rect){.x=(X),.y=(Y),.w=(W),.h=(H)}
#define MAX(X,Y) (((X)<(Y))?(Y):(X))
#define NEGATIVE(X) ((X)*-1)
#define DATE(X,Y,Z) ((((X)*100*100)+((Y)*100))+(Z))

namespace {
static const unsigned long BuildID = DATE(2014, 6, 12);
static const unsigned long Version = 0.02;


class point {
	public:
	int x, y;
	point(){}
	point(const int w, const int h): x(w), y(h) {}
	void clamp(const point min, const point max) {
		if(x < min.x) { x = min.x; }
		if(y < min.y) { y = min.y; }
		if(x > max.x) { x = max.x; }
		if(y > max.y) { y = max.y; }
	}
	void inc2d(const point limit) {
		++x;
		if(x >= limit.x) { ++y;	x=0; }
		if(y >= limit.y) { y=0;	}
	}
	bool equals(const point other) const {
		return (x == other.x && y == other.y);
	}
	void copyFrom(const point source) {
		x = source.x;
		y = source.y;
	}
};

class config {
	public:
	char *filename;
	char *savename;
	int fps, delay;
	int shift, tileSize;
	point editorSize;
	point spriteMapSize;
	point mapSize;
	bool canRestore;

	void init() {
		int tmp = 0;
		while (this->tileSize >> tmp) { ++tmp; }
		this->shift = --tmp;
		this->delay = ( 1000 / this->fps );
	}

	int toTile(const int pixelVal) {
		return (pixelVal >> this->shift);
	}

	int toPixel(const int tileVal) {
		return (tileVal << this->shift);
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
		return (((xy.x & 0xff) << 8) + (xy.y & 0xff));
	}
	static point toPoint(const short data) {
		return point((data >> 8), (data & 0xff));
	}
};

class map {
	tmap* tileMap;
	FILE* restoreFile;
	static const int size = sizeof(struct tmap);

	public:
	map() {
		if(this->canRestore(conf.savename)) {
			conf.canRestore = true;
			restore();
		} else {
			create(conf.mapSize);
		}
	}

	void create(point size) {
		tmap* l;
		int tmap_size = (this->size + ((size.x * size.y) * 2));

		l = (tmap*) calloc(1, tmap_size);
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

	int offset(const point loc) {
		tmap* x = this->tileMap;
		int tmpOffset = (x->width * x->origin_y) + x->origin_x;
		return tmpOffset + ((x->width * loc.y) + loc.x);
	}

	void moveOrigin(point change) {
		tmap* l = this->tileMap;
		point p = point(l->origin_x, l->origin_y);

		p.copyFrom(change);
		p.clamp(
			point(0,0),
			point(l->width - conf.editorSize.x, l->height - conf.editorSize.y)
		);

		l->origin_x = p.x;
		l->origin_y = p.y;
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
		conf.spriteMapSize = point(conf.toTile(tmp->w), conf.toTile(tmp->h));
		SDL_SetColorKey(tmp, SDL_TRUE, SDL_MapRGB(tmp->format, 0xff, 0xff, 0xff));

		//window
		this->win = SDL_CreateWindow(
			"Editor",
			100,
			100,
			conf.toPixel(conf.editorSize.x + conf.spriteMapSize.x),
			conf.toPixel(MAX(conf.editorSize.y, conf.spriteMapSize.y)),
			SDL_WINDOW_SHOWN
		);

		//renderer
		this->ren = SDL_CreateRenderer(
			this->win,
			-1, //auto choose any suitable renderer
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
		);

		//tilemap texture
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
	char key_event;
	int skippedEvents;

	void run() {
		skippedEvents = 0;
		running = true;
		leftClick = false;
		rightClick = false;
		mouseUp = point(-1, -1);
		enterPressed = false;
		key_event = 0;

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

			default:
				this->key_event = true;
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
	point cursorTile;
	point selection;

	editor(input* a, graphics* b, map* c, point fil, point cur): i(a), g(b), m(c) {
		if(conf.canRestore) {
			this->restore();
		}
		this->cursorTile = cur;
		this->cursor = point(0,0);
		this->selection = point(0,0);
	}

	void moveCursor(point dest) {
		g->copyTile(m->get(this->cursor), this->cursor);
		this->cursor = dest;
		this->cursor.clamp(point(0,0), conf.editorSize);
		this->drawCursor();
		g->flip();
	}

	void drawCursor() {
		g->copyTile(this->cursorTile, this->cursor);
	}

	void fill(point fillTile) {
		point loc(0,0);
		do {
			m->put(loc, fillTile);
			g->copyTile(fillTile, loc);
			loc.inc2d(conf.editorSize);
		} while (loc.x || loc.y);
	}

	void restore() {
		point loc(0,0);
		do {
			g->copyTile(m->get(loc), loc);
			loc.inc2d(conf.editorSize);
		} while (loc.x || loc.y);
	}

	void draw() {
		this->quickDraw();
		this->cursor.inc2d(conf.editorSize);
		this->drawCursor();
		g->flip();
	}

	void quickDraw() {
		m->put(this->cursor, this->selection);
		g->copyTile(this->selection, this->cursor);
	}
};

}
#endif //SDLEDITOR_H
