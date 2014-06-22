#ifndef SDLEDITOR_H
#define SDLEDITOR_H
#include <SDL2/SDL.h>

#define DATE(YYYY,MM,DD) ((YYYY*100*100)+(MM*100)+DD)

namespace {
static const unsigned long BuildID = DATE(2014, 6, 18);
static const unsigned long Version = 0.06;
static const unsigned long MinorBuild = 3;
static const char * const ProgName = "GameEditor (SDL)";

class point {
	public:
	Sint32 x;
	Sint32 y;
	point(): x(0), y(0) {}
	point(const int w, const int h): x(w), y(h) {}
	void clamp(const point min, const point max = point(0,0)) {
		if(x < min.x) { x = min.x; }
		if(y < min.y) { y = min.y; }
		if(x > max.x) { x = max.x; }
		if(y > max.y) { y = max.y; }
	}
	void math2d(const point xy, const point lim) {
		x+=xy.x;y+=xy.y;
		if(x >= lim.x) { ++y; x=0; }
		if(y >= lim.y) { y=0; }
	}
	point operator+(const point xy) const {
		return point(x + xy.x, y + xy.y);
	}
	point operator-(const point xy) const {
		return point(x - xy.x, y - xy.y);
	}
};

class rect {
	public:
	SDL_Rect quad;
	rect(const int x, const int y, const int w, const int h) {
		quad.x = x;
		quad.y = y;
		quad.w = w;
		quad.h = h;
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

	int toPixel(const int pixelVal) {
		return (pixelVal << this->shift);
	}
};
config conf;

struct tmap {
	Uint8  magic[4];
	Uint32 size;
	Uint16 width;
	Uint16 height;
	Uint16 origin_x;
	Uint16 origin_y;
	Uint32 data[];
};

class map {
	tmap* tileMap;
	static const int struct_size = sizeof(struct tmap);
	union { Uint32 i; Uint16 s[2];} layerformat;

	static short toShort(const point xy) {
		return (((xy.x & 0xff) << 8) + (xy.y & 0xff));
	}

	static point toPoint(const short data) {
		return point((data >> 8), (data & 0xff));
	}

	public:
	map(const char * const mapname) {
		SDL_RWops* file = SDL_RWFromFile(mapname, "r");
		tmap* l;
		int tmap_size;

		if (file) {
			l = (tmap*) malloc(this->struct_size);
			SDL_RWread(file, l, this->struct_size, 1);
			l = (tmap*) realloc(l, l->size);
			SDL_RWread(file, &(l->data), (l->size - this->struct_size), 1);
			SDL_RWclose(file);
		} else {
			tmap_size = (this->struct_size + ((conf.mapSize.x * conf.mapSize.y) * sizeof(Uint32)));
			l = (tmap*) calloc(1, tmap_size);
			*((int*)l->magic) = *(int*)"Tmap";
			l->width = (short) (conf.mapSize.x & 0xffff);
			l->height = (short) (conf.mapSize.y & 0xffff);
			l->size = tmap_size;
		}
		this->tileMap = l;
	}

	~map() {
		free(this->tileMap);
	}

	void moveOrigin(point change) {
		tmap* l = this->tileMap;
		point p = point(l->origin_x, l->origin_y) + change;

		p.clamp(point(0,0), point(l->width, l->height) - conf.editorSize);

		l->origin_x = p.x;
		l->origin_y = p.y;
	}

	void write(const char * const filename) {
		SDL_RWops* tmp = SDL_RWFromFile(filename, "w");
		SDL_RWwrite(tmp, this->tileMap, this->tileMap->size, 1);
		SDL_RWclose(tmp);
	}

	int offset(const point loc) {
		tmap* x = this->tileMap;
		int tmpOffset = (x->width * x->origin_y) + x->origin_x;
		return tmpOffset + ((x->width * loc.y) + loc.x);
	}

	point get(point loc, int layer) {
		layerformat.i = this->tileMap->data[this->offset(loc)];
		return this->toPoint(layerformat.s[layer]);
	}

	void put(point loc, point data, int layer) {
		layerformat.i = this->tileMap->data[this->offset(loc)];
		layerformat.s[layer] = this->toShort(data);
		this->tileMap->data[this->offset(loc)] = layerformat.i;

	}
};

class graphics {
	SDL_Window* win;
	SDL_Renderer* ren;
	SDL_Texture* tex;

	public:
	int tilePosition;

	graphics(const char * const tilemap, const point size, const point loc, const long wflags, const long rflags) {
		SDL_Init(SDL_INIT_EVERYTHING);

		this->tilePosition = 0;

		SDL_Surface* tmp = SDL_LoadBMP(tilemap);
		conf.spriteMapSize = point(conf.toTile(tmp->w), conf.toTile(tmp->h));
		SDL_SetColorKey(tmp, SDL_TRUE, SDL_MapRGB(tmp->format, 0xff, 0xff, 0xff));

		//window
		this->win = SDL_CreateWindow(ProgName, loc.x, loc.y, size.x, size.y, wflags);

		//renderer
		this->ren = SDL_CreateRenderer(this->win, -1, rflags);

		//tilemap texture
		this->tex = SDL_CreateTextureFromSurface(this->ren, tmp);
		SDL_FreeSurface(tmp);

		//clear the framebuffer, draw the spritemap and display
		SDL_SetRenderDrawColor(this->ren, 0, 0, 0, 255);
		this->clear();
		this->drawTileMap(0);
		this->flip();
	}

	~graphics() {
		SDL_DestroyTexture(this->tex);
		SDL_DestroyRenderer(this->ren);
		SDL_DestroyWindow(this->win);
		SDL_Quit();
	}

	void drawTileMap(const int origin) {
		static const int height = conf.toPixel(SDL_min(conf.spriteMapSize.y, conf.spriteViewport.y));

		static SDL_Rect src = rect(0, 0, conf.toPixel(conf.spriteViewport.x), height).quad;

		static const SDL_Rect dest = rect(
			conf.toPixel(conf.editorSize.x),
			0,
			conf.toPixel(conf.spriteViewport.x),
			height
		).quad;

		this->tilePosition += origin;
		this->fill(dest); //clear the tilemap
		this->tilePosition = SDL_min(SDL_max(this->tilePosition, 0),conf.spriteMapSize.x - conf.spriteViewport.x);
		src.x = conf.toPixel(this->tilePosition);

		this->copy(src, dest);
		this->flip();
	}

	void copy(const SDL_Rect src, const SDL_Rect dest) {
		SDL_RenderCopy(this->ren, this->tex, &src, &dest);
	}

	void clear() {
		SDL_RenderClear(this->ren);
	}

	void fill(const SDL_Rect area) {
		SDL_RenderFillRect(this->ren, &area);
	}

	void flip() {
		SDL_RenderPresent(this->ren);
	}

	void copyTile(const point src, const point dest) {
		const int x = conf.tileSize;
		static SDL_Rect srcRect = rect(0, 0, x, x).quad;
		static SDL_Rect destRect = rect(0, 0, x, x).quad;

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
	char layer;

	public:
	point cursor;
	point cursorTile;
	point selection;
	bool running;
	bool cursorY;

	editor(input* a, graphics* b, map* c, point cur): i(a), g(b), m(c) {
		this->layer = 0;
		this->cursorY = false;
		this->restore();
		this->cursorTile = cur;
		this->drawCursor();
	}

	void moveCursor(point dest) {
		this->setCursor(this->cursor + dest);
	}

	void setCursor(point dest) {
		g->copyTile(m->get(this->cursor, 0), this->cursor);
		g->copyTile(m->get(this->cursor, 1), this->cursor);
		this->cursor = dest;
		this->cursor.clamp(point(0,0), conf.editorSize - point(1,1));
		this->drawCursor();
		g->flip();
	}

	void drawCursor() {
		g->copyTile(this->cursorTile, this->cursor);
	}

	void fill(point fillTile) {
		point loc(0,0);
		do {
			m->put(loc, fillTile, this->layer);
			g->copyTile(fillTile, loc);
			loc.math2d(point(1,0), conf.editorSize);
		} while (loc.x || loc.y);
	}

	void restore() {
		point loc(0,0);
		do {
			g->copyTile(m->get(loc, 0), loc);
			g->copyTile(m->get(loc, 1), loc);
			loc.math2d(point(1,0), conf.editorSize);
		} while (loc.x || loc.y);
	}

	void draw() {
		m->put(this->cursor, this->selection, this->layer);
		g->copyTile(m->get(this->cursor, 0), this->cursor);
		g->copyTile(m->get(this->cursor, 1), this->cursor);
		if(i->keyClick) {
			if(this->cursorY) {
				this->cursor.math2d(point(0,1), conf.editorSize);
			} else {
				this->cursor.math2d(point(1,0), conf.editorSize);
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
				this->selection.x += g->tilePosition;
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
					g->drawTileMap(0);
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

				case SDLK_1:
					layer = 0;
					break;

				case SDLK_2:
					layer = 1;
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

				case SDLK_PAGEDOWN:
					g->drawTileMap(-10);
					break;

				case SDLK_PAGEUP:
					g->drawTileMap(10);
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
