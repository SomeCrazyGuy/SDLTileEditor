#ifndef SDLEDITOR_H
#define SDLEDITOR_H
#include <SDL2/SDL.h>

namespace {
class Factory {
	public:
	static SDL_Rect rect(int a, int b, int c, int d) {
		return ((SDL_Rect){.x=a,.y=b,.w=c,.h=d});
	}
	static int max(int a, int b) {
		return (a>b)? a : b;
	}
	static unsigned long date(int yyyy, int mm, int dd) {
		return (yyyy*100*100) + (mm*100) + dd;
	}
};

static const unsigned long BuildID = Factory::date(2014, 6, 15);
static const unsigned long Version = 0.04;
static const unsigned long MinorBuild = 2;

/* BUG: wasd will move one tile beyond the editor
 * BUG: need full redraw with restore function
 * */

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
};
class config {
	public:
	int fps, delay;
	int shift, tileSize;
	point editorSize;
	point spriteViewport;
	point spriteMapSize;
	point mapSize;

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
	SDL_RWops* restoreFile;
	static const int size = sizeof(struct tmap);

	public:
	map(const char * const mapname) {
		if(this->canRestore(mapname)) {
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
		SDL_RWread(this->restoreFile, l, this->size, 1);
		l = (tmap*) realloc(l, l->size);
		SDL_RWread(this->restoreFile, &(l->data), (l->size - this->size), 1);
		this->tileMap = l;
		SDL_RWclose(this->restoreFile);
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

		p.x += change.x;
		p.y += change.y;

		p.clamp(
			point(0,0),
			point(l->width - conf.editorSize.x, l->height - conf.editorSize.y)
		);

		l->origin_x = p.x;
		l->origin_y = p.y;
	}

	bool canRestore(const char * const filename) {
		this->restoreFile = SDL_RWFromFile(filename, "r");
		return this->restoreFile;
	}

	void write(const char * const filename) {
		SDL_RWops* tmp = SDL_RWFromFile(filename, "w");
		SDL_RWwrite(tmp, this->tileMap, this->tileMap->size, 1);
		SDL_RWclose(tmp);
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
	graphics(const char * const tilemap) {
		SDL_Init(SDL_INIT_EVERYTHING);

		SDL_Surface* tmp = SDL_LoadBMP(tilemap);
		conf.spriteMapSize = point(conf.toTile(tmp->w), conf.toTile(tmp->h));
		SDL_SetColorKey(tmp, SDL_TRUE, SDL_MapRGB(tmp->format, 0xff, 0xff, 0xff));

		//window
		this->win = SDL_CreateWindow(
			"Editor",
			100,
			100,
			conf.toPixel(conf.editorSize.x + conf.spriteMapSize.x),
			conf.toPixel(Factory::max(conf.editorSize.y, conf.spriteMapSize.y)),
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
			Factory::rect(0, 0, conf.toPixel(conf.spriteMapSize.x), conf.toPixel(conf.spriteMapSize.y)),
			Factory::rect(conf.toPixel(conf.editorSize.x), 0, conf.toPixel(conf.spriteMapSize.x), conf.toPixel(conf.spriteMapSize.y))
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
		static SDL_Rect srcRect = Factory::rect(0, 0, x, x);
		static SDL_Rect destRect = Factory::rect(0, 0, x, x);

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
	bool quitEvent;
	bool mouseClick;
	bool keyClick;
	char ascii;
	int keysym;
	point clickPos;

	void run() {
		quitEvent = false;
		mouseClick = false;
		keyClick = false;
		keysym = 0;
		clickPos = point(-1,-1);

		while(SDL_PollEvent(&e)) {
			switch(e.type) {
				case SDL_QUIT:
					quitEvent = true;
					break;
				case SDL_MOUSEBUTTONDOWN:
					mouseClick = true;
					clickPos = point(conf.toTile(e.button.x), conf.toTile(e.button.y));
					break;
				case SDL_KEYDOWN:
					keyClick = true;
					keysym = e.key.keysym.sym;
					break;
				case SDL_TEXTINPUT:
					ascii = e.text.text[0];
					break;
				default: continue;
			}
		}
	}

	void enableTextInput() {
		SDL_StartTextInput();
	}

	void dsableTextInput() {
		SDL_StopTextInput();
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
	bool running;
	bool cursorY;

	editor(input* a, graphics* b, map* c, point cur): i(a), g(b), m(c) {
		this->restore();
		this->cursorTile = cur;
		this->cursor = point(0,0);
		this->selection = point(0,0);
		this->drawCursor();
	}

	void moveCursor(point dest) {
		this->setCursor(point(this->cursor.x + dest.x, this->cursor.y + dest.y));
	}

	void setCursor(point dest) {
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
		m->put(this->cursor, this->selection);
		g->copyTile(this->selection, this->cursor);
		if(i->keyClick) {
			if(this->cursorY) {
				this->cursor.y++;
				if(this->cursor.y >= conf.editorSize.y) {
					this->cursor.y = 0;
				}
			} else {
				this->cursor.inc2d(conf.editorSize);
			}
		}
		this->drawCursor();
		g->flip();
	}

	void handleEvents() {
		this->running = true;
		bool keyContinue = false;
		i->run();

		if(i->quitEvent) {
			this->running = false;
		}

		/* Mouse Input */
		if (i->mouseClick) {
			if(i->clickPos.x < conf.editorSize.x) {
				this->setCursor(i->clickPos);
				if(i->e.button.button == SDL_BUTTON_LEFT) {
					this->draw();
				}
			} else {
				this->selection = i->clickPos;
				this->selection.x -= conf.editorSize.x;
				this->draw();
			}
		}

		/* Keyboard Input */
		if (i->keyClick) {
			switch(i->keysym) {
				case SDLK_q:
				case SDLK_ESCAPE:
					this->running = false;
					break;

				case SDLK_SPACE:
				case SDLK_RETURN:
					this->draw();
					break;

				case SDLK_r:
					g->clear();
					this->restore();
					this->drawCursor();
					g->flip();
					break;

				case SDLK_x:
					this->cursorY = !this->cursorY;
					break;

				case SDLK_f:
					this->fill(this->selection);
					g->flip();
					break;

				default: keyContinue = true;
			}
		}
		if(keyContinue) {
			keyContinue = false;
			switch(i->keysym) {
				case SDLK_w:
					this->moveCursor(point(0,-1));
					break;
				case SDLK_a:
					this->moveCursor(point(-1,0));
					break;
				case SDLK_s:
					this->moveCursor(point(0,1));
					break;
				case SDLK_d:
					this->moveCursor(point(1,0));
					break;
				default: keyContinue = true;
			}
		}
		if(keyContinue) {
			switch(i->keysym) {
				case SDLK_h:
					m->moveOrigin(point((conf.editorSize.x/2)*-1, 0));
					break;
				case SDLK_j:
					m->moveOrigin(point(0, (conf.editorSize.x/2)*-1));
					break;
				case SDLK_k:
					m->moveOrigin(point(0, (conf.editorSize.x/2)));
					break;
				case SDLK_l:
					m->moveOrigin(point((conf.editorSize.x/2), 0));
					break;
			}
			this->restore();
			this->drawCursor();
			g->flip();
		}
	}
};

}
#endif //SDLEDITOR_H
