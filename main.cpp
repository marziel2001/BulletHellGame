#include<math.h>
#include<stdio.h>
#include<string.h>
#include<iostream>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define WINDOW_WIDTH 1600	// pixels
#define WINDOW_HEIGHT 900	// pixels

#define LEVEL_HEIGHT 2000	// pixels
#define LEVEL_WIDTH 4000	// pixels

#define PLAYER_WIDTH 100	// pixels
#define PLAYER_HEIGHT 100	// pixels

#define ENEMY_WIDTH 150		// pixels
#define ENEMY_HEIGHT 150	// pixels

#define ENEMY1_HP 10
#define ENEMY2_HP 10
#define ENEMY3_HP 40

#define PLAYER_SPEED_MULTIPLIER 9	
#define PLAYER_SHOOT_SPEED 8	
#define PLAYER_HP 10	
#define INVINCIBILITY_TIME 3	// seconds

#define MAX_BULLETS 100
#define MAX_PLAYER_BULLETS 5
#define MAX_BULLETS_PER_ATTACK 8

#define BULLET_SPEED 3
#define BULLET_SIZE 70
#define P_BULLET_SIZE 50
#define MEGA_BULLET_SIZE 200

#define SCORE_MULTIPLIER 1.1

#define FRAMERATE 60	// Target fps
#define FRAME_DELAY (1000 / FRAMERATE)

enum directions {
	up,
	right,
	down,
	left,
	ul,
	ur,
	dl,
	dr
};
typedef struct {
	double x, y;
} point;
typedef struct {
	int czarny, zielony, czerwony, zolty,
		pomaranczowy, jasnoniebieski, niebieski;
} color;
typedef struct {
	int tick1, tick2, frames;
	double delta, worldTime, fpsTimer, fps, frametime, shootTimer,
		hpTimer, scoreDisplay, highlightDelay, highlightDuration, megaDelay, godMode;
} timer;	// liczenie czasu, klatek itp.
typedef struct {
	point pos;	// aktualna pozycja	
	point velocity;	// vector toru lotu
	point size;
	directions direction;
	bool collected;
} bullet;
typedef struct {
	point size, velocity[MAX_BULLETS_PER_ATTACK];
	double speed, delay;
	int nOfBullets;
} attackPattern;
typedef struct {
	int fired, hits, hp, maxHp, shoot;
	double speed;
	bool boss, invincible;
	point pos, size, velocity;	
	bullet bullets[MAX_BULLETS];
	attackPattern attack[2];
	directions shootDirection, direction;
} character;
typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	SDL_Texture* scrtex;
	SDL_Surface* screen, * charset, * heartTex, * scoreBg,
		** activeBg, * background1, * background2, * background3, *highlight[2],
		* bulletTex[4], * pBulletTex[4], * megaBulletTex,
		** activeEnemy, * enemy1Tex, * enemy2Tex, * enemy3Tex,
		** playerState, * playerTex[8], * invincibility;
	color colors;
	timer gameTime;
	bool quit, newGame, printScore;
	int highlightType, megaBulletActive, tmpScore, stage;
	double score, scoreBonus;
	char text[128];
	point camera;
	bullet heart, highlights[2], megaBullet;
} engine;	 // informacje dot calej gry

void specialAttackMega(engine* game, character* player, character* enemy);
void specialAttackHighlight(engine* game, character* player, character * enemy);
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset);
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y);
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color);
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color);
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor);
void SDLInit();
void initGraphicElements(engine* game);
void loadBMP(SDL_Surface** destination, char fileName[], engine* game);
void assignTextures(engine* game);
void destroy(engine* game);
void drawBullets(engine* game, character* player, SDL_Surface** texture);
void renderFrame(engine* game, character* player1, character* enemy);
void initTimer(timer* timer);
void countTime(timer* gameTime);
void playerOutsideBorder(character* player);
void setCamera(engine* game, character* player1);
bool collision(bullet* bullet, character* player);
void movePlayer(engine* game, character* player1);
bool bulletOutsideBoard(bullet* bullet);
void moveBullet(bullet* bullet);
void shootEnemy(timer* timer, character* enemy);
void checkInput(engine* game, character* player1);
void createAttack1(attackPattern* attack);
void createAttack2(attackPattern* attack);
void createAttack3(attackPattern* attack);
void createAttack4(attackPattern* attack);
void initPlayer(character* player1);
void initEnemy1(character* enemy);
void initEnemy2(character* enemy);
void initEnemy3(character* enemy);
void placeHeart(engine* game, character* player);
void newGame(engine* game, character* player, character* enemy, int stage);
void shootPlayer(engine* game, character* player1, character* enemy);
void moveEnemy(engine* game, character* enemy);
void handleBullets(engine* game, character* player1, character* enemy);
void changeLevel(engine* game, character* player1, character* enemy, int number);


// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	srand(time(NULL));

	SDLInit();
	engine game;
	character player1;
	character enemy;
	
	player1.velocity.x = player1.velocity.y = 0;

	initGraphicElements(&game);
	assignTextures(&game);
	newGame(&game, &player1, &enemy, 1);
	game.score = 0;

	while (!game.quit) {
		countTime(&game.gameTime);
		checkInput(&game, &player1);

		movePlayer(&game, &player1);
		moveEnemy(&game, &enemy);

		shootPlayer(&game, &player1, &enemy);
		shootEnemy(&game.gameTime, &enemy);

		specialAttackHighlight(&game, &player1, &enemy);
		specialAttackMega(&game, &player1, &enemy);

		handleBullets(&game, &player1, &enemy);	

		placeHeart(&game, &player1);

		renderFrame(&game, &player1, &enemy);

		changeLevel(&game, &player1, &enemy, 2);
		changeLevel(&game, &player1, &enemy, 3);

		if (game.newGame) {
			game.score = 0;
			newGame(&game, &player1, &enemy, 1);
		}
		game.gameTime.frames++;
	}
	destroy(&game);
	SDL_Quit();
	return 0;
};

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y). charset to bitmapa znakow
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);	// funkcja ktora renderuje tylko fragment tekstury 
		x += 8;
		text++;
	}
}
// rysowanie na powierzchni screen, sprite'a w punkcie (x, y). (x, y) to pozycja úrodka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
}
// rysowanie pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
}
// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) bπdü poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	}
}
// rysowanie prostokπta o d≥ugoúci bokÛw l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++) {
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	}
}

void SDLInit() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}
}
// inicjalizuje Okno, Renderer i kolory
void initGraphicElements(engine* game) {
	int rc;
	rc = SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &game->window, &game->renderer);
	if (rc != 0) {
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		exit(0);
	}
	SDL_ShowCursor(0);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(game->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
	SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);	// ustawia na jaki kolor domyslnie czysci sie render.
	SDL_SetWindowTitle(game->window, "Bullet Hell");
	game->screen = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	game->scrtex = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
	game->colors.czarny = SDL_MapRGB(game->screen->format, 0, 0, 0);
	game->colors.zielony = SDL_MapRGB(game->screen->format, 0, 255, 0);
	game->colors.czerwony = SDL_MapRGB(game->screen->format, 255, 0, 0);
	game->colors.niebieski = SDL_MapRGB(game->screen->format, 0, 0, 255);
	game->colors.jasnoniebieski = SDL_MapRGB(game->screen->format, 100, 100, 255);
	game->colors.zolty = SDL_MapRGB(game->screen->format, 255, 255, 0);
	game->colors.pomaranczowy = SDL_MapRGB(game->screen->format, 255, 128, 0);
}

void loadBMP(SDL_Surface** destination, char fileName[], engine* game) {
	*destination = SDL_LoadBMP(fileName);
	if (*destination == NULL) {
		printf("SDL_LoadBMP(%s) error: %s\n", fileName, SDL_GetError());
		SDL_Quit();
		exit(0);
	}
}

void assignTextures(engine* game) {
	loadBMP(&game->charset, "./bmp/cs8x8.bmp", game);	// za≥adowanie tekstur 
	SDL_SetColorKey(game->charset, true, 0x000000); //ustawia jaki pixel ma definiowac przezroczystosc

	loadBMP(&game->playerTex[up], "./bmp/playerU.bmp", game);
	loadBMP(&game->playerTex[down], "./bmp/playerD.bmp", game);
	loadBMP(&game->playerTex[right], "./bmp/playerR.bmp", game);
	loadBMP(&game->playerTex[left], "./bmp/playerL.bmp", game);
	loadBMP(&game->playerTex[ul], "./bmp/playerUl.bmp", game);
	loadBMP(&game->playerTex[ur], "./bmp/playerUr.bmp", game);
	loadBMP(&game->playerTex[dl], "./bmp/playerDl.bmp", game);
	loadBMP(&game->playerTex[dr], "./bmp/playerDr.bmp", game);

	loadBMP(&game->invincibility, "./bmp/invincibility.bmp", game);

	loadBMP(&game->enemy1Tex, "./bmp/enemy1.bmp", game);
	loadBMP(&game->enemy2Tex, "./bmp/enemy2.bmp", game);
	loadBMP(&game->enemy3Tex, "./bmp/enemy3.bmp", game);

	loadBMP(&game->bulletTex[0], "./bmp/fireball.bmp", game);

	loadBMP(&game->megaBulletTex, "./bmp/megaBullet.bmp", game);

	loadBMP(&game->pBulletTex[up], "./bmp/projectileU.bmp", game);
	loadBMP(&game->pBulletTex[right], "./bmp/projectileR.bmp", game);
	loadBMP(&game->pBulletTex[down], "./bmp/projectileD.bmp", game);
	loadBMP(&game->pBulletTex[left], "./bmp/projectileL.bmp", game);

	loadBMP(&game->heartTex, "./bmp/repair.bmp", game);
	loadBMP(&game->scoreBg, "./bmp/boom.bmp", game);

	loadBMP(&game->background1, "./bmp/level1.bmp", game);
	loadBMP(&game->background2, "./bmp/level2.bmp", game);
	loadBMP(&game->background3, "./bmp/level3.bmp", game);

	loadBMP(&game->highlight[0], "./bmp/highlight.bmp", game);
	loadBMP(&game->highlight[1], "./bmp/highlight2.bmp", game);
}
// dealokuje grafiki
void destroy(engine* game) {
	SDL_FreeSurface(game->screen);
	SDL_FreeSurface(game->charset);
	SDL_FreeSurface(game->heartTex);
	SDL_FreeSurface(game->scoreBg);

	SDL_FreeSurface(game->background1);
	SDL_FreeSurface(game->background2);
	SDL_FreeSurface(game->background3);

	SDL_FreeSurface(game->bulletTex[0]);
	SDL_FreeSurface(game->megaBulletTex);

	SDL_FreeSurface(game->pBulletTex[up]);
	SDL_FreeSurface(game->pBulletTex[right]);
	SDL_FreeSurface(game->pBulletTex[down]);
	SDL_FreeSurface(game->pBulletTex[left]);
	
	SDL_FreeSurface(game->highlight[0]);
	SDL_FreeSurface(game->highlight[1]);

	SDL_FreeSurface(game->enemy1Tex);
	SDL_FreeSurface(game->enemy2Tex);
	SDL_FreeSurface(game->enemy3Tex);

	SDL_FreeSurface(game->playerTex[up]);
	SDL_FreeSurface(game->playerTex[down]);
	SDL_FreeSurface(game->playerTex[left]);
	SDL_FreeSurface(game->playerTex[right]);

	SDL_FreeSurface(game->playerTex[ul]);
	SDL_FreeSurface(game->playerTex[ur]);
	SDL_FreeSurface(game->playerTex[dl]);
	SDL_FreeSurface(game->playerTex[dr]);

	SDL_FreeSurface(game->invincibility);

	SDL_DestroyTexture(game->scrtex);
	SDL_DestroyWindow(game->window);
	SDL_DestroyRenderer(game->renderer);
}
// rysuje pociski dowolnej postaci
void drawBullets(engine* game, character* player, SDL_Surface** texture) {
	for (int i = player->fired - 1; i >= 0; i--) {
		DrawSurface(game->screen, texture[player->bullets[i].direction],
			player->bullets[i].pos.x + player->bullets[i].size.x / 2 - game->camera.x,
			player->bullets[i].pos.y + player->bullets[i].size.y / 2 - game->camera.y
		);
	}
}
// rysuje nowπ klatkÍ
void renderFrame(engine* game, character* player1, character* enemy) {
	SDL_FillRect(game->screen, NULL, game->colors.czarny);	// czarne t≥o

	DrawSurface(game->screen, *game->activeBg,
		LEVEL_WIDTH / 2 - game->camera.x,
		LEVEL_HEIGHT / 2 - game->camera.y
	);	// rysuje obrazek w tle gry zaleznie od poziomu

	if (game->highlightType > 0) {
		DrawSurface(game->screen, game->highlight[game->highlightType-1],
				game->highlights[game->highlightType-1].pos.x + game->highlights[game->highlightType-1].size.x /2 - game->camera.x,
				game->highlights[game->highlightType-1].pos.y + game->highlights[game->highlightType-1].size.y / 2 - game->camera.y
		);	// rysuje podúwietlenie ataku specjalnego 
	}

	drawBullets(game, enemy, game->bulletTex);	// rysuje pociski przeciwnika
	drawBullets(game, player1, game->pBulletTex); // rysuje pociski gracza

	if (game->megaBulletActive) {
		DrawSurface(game->screen, game->megaBulletTex,
			game->megaBullet.pos.x + game->megaBullet.size.x /2 - game->camera.x,
			game->megaBullet.pos.y + game->megaBullet.size.y / 2 - game->camera.y
		);	// rysuje bombÍ
	}

	DrawSurface(game->screen, game->playerTex[player1->direction],
		player1->pos.x + player1->size.x / 2 - game->camera.x,
		player1->pos.y + player1->size.y / 2 - game->camera.y
	);	// rysuje gracza

	if (player1->invincible) {
		DrawSurface(game->screen, game->invincibility, player1->pos.x - game->camera.x, player1->pos.y - game->camera.y
		);	// efekt nieúmiertelnoúci gracza
	}

	DrawSurface(game->screen, *game->activeEnemy,
		enemy->pos.x + enemy->size.x / 2 - game->camera.x,
		enemy->pos.y + enemy->size.y / 2 - game->camera.y
	);	// rysuje przeciwnika

	if (!game->heart.collected) {
		DrawSurface(game->screen, game->heartTex,
			game->heart.pos.x + game->heart.size.x / 2 - game->camera.x,
			game->heart.pos.y + game->heart.size.y / 2 - game->camera.y
		); // rysuje apteczkÍ
	}

	// rysowanie paskÛw øycia
	DrawRectangle(game->screen, 0, WINDOW_HEIGHT - 80, 200, 80, game->colors.czerwony, game->colors.jasnoniebieski);

	sprintf(game->text, "Enemy health: %d", enemy->hp);
	DrawString(game->screen, 24, WINDOW_HEIGHT - 35, game->text, game->charset);
	DrawRectangle(game->screen, 50, WINDOW_HEIGHT - 20, (enemy->hp / (double)enemy->maxHp) * 100, 8, game->colors.czerwony, game->colors.czerwony);

	sprintf(game->text, "Player health: %d", player1->hp);
	DrawString(game->screen, 24, WINDOW_HEIGHT - 65, game->text, game->charset);
	DrawRectangle(game->screen, 50, WINDOW_HEIGHT - 50, (player1->hp / (double)player1->maxHp) * 100, 8, game->colors.czerwony, game->colors.czerwony);
	
	// rysowanie bloku z tekstem
	DrawRectangle(game->screen, 500, 0, WINDOW_WIDTH - 1000, 60, game->colors.czerwony, game->colors.pomaranczowy);

	sprintf(game->text, "Stage %d, Time: = %.1lf s  score: %.1f, %.0lf fps ", game->stage, game->gameTime.worldTime,  game->score, game->gameTime.fps);
	DrawString(game->screen, game->screen->w / 2 - strlen(game->text) * 8 / 2, 8, game->text, game->charset);

	sprintf(game->text, "Esc - exit, n - new game  player hits: %d, enemy hits: %d", player1->hits, enemy->hits);
	DrawString(game->screen, game->screen->w / 2 - strlen(game->text) * 8 / 2, 24, game->text, game->charset);

	sprintf(game->text, "WSAD - shooting, arrows - movement");
	DrawString(game->screen, game->screen->w / 2 - strlen(game->text) * 8 / 2, 40, game->text, game->charset);

	if (game->printScore) {
		if (game->gameTime.scoreDisplay - game->gameTime.worldTime < 0) {
			game->printScore = false;
		}
		DrawSurface(game->screen, game->scoreBg, WINDOW_WIDTH / 2, 300);
		sprintf(game->text, "Score: %.1f ", game->score);
		DrawString(game->screen, game->screen->w / 2 - strlen(game->text) * 8 / 2, 300, game->text, game->charset);
	}	// wyswietla wynik po ukonczeniu poziomu

	// aktualizuj teksture scrtex pikselami z screen
	SDL_UpdateTexture(game->scrtex, NULL, game->screen->pixels, game->screen->pitch);

	SDL_RenderCopy(game->renderer, game->scrtex, NULL, NULL); // kopiuje teksture na render do buckbuffer
	SDL_RenderPresent(game->renderer); // podmienia frontbuffer na backbuffer
}

void countTime(timer* gameTime) {
	gameTime->tick2 = SDL_GetTicks();
	gameTime->frametime = (gameTime->tick2 - gameTime->tick1);
	gameTime->delta = gameTime->frametime * 0.001;
	gameTime->tick1 = gameTime->tick2;
	gameTime->worldTime += gameTime->delta;

	gameTime->fpsTimer += gameTime->delta;
	if (gameTime->fpsTimer > 1) {
		gameTime->fps = gameTime->frames;
		gameTime->frames = 0;
		gameTime->fpsTimer = 0;
	}
	if (FRAME_DELAY > gameTime->frametime) {
		SDL_Delay(FRAME_DELAY - gameTime->frametime);
	}
}
// sprawdza kolizje hitboxÛw 
bool collision(bullet* bullet, character* player) {
	if ((bullet->pos.x >= player->pos.x - bullet->size.x && bullet->pos.x <= player->pos.x + player->size.x)
		&& (bullet->pos.y >= player->pos.y - bullet->size.y && bullet->pos.y <= player->pos.y + player->size.y)) {
		return true;
	}
	else return false;
}

bool bulletOutsideBoard(bullet* bullet) {
	if (bullet->pos.x < 0 - bullet->size.x || bullet->pos.x > LEVEL_WIDTH ||
		bullet->pos.y < 0 - bullet->size.y || bullet->pos.y > LEVEL_HEIGHT) {
		return true;
	}
	else return false;
}
// sprawdza czy gracz nie wychodzi poza plansze
void playerOutsideBorder(character* player) {

	if (player->pos.x < 0) {
		player->pos.x = 0;
	}
	else if (player->pos.x + player->size.x > LEVEL_WIDTH) {
		player->pos.x = LEVEL_WIDTH - player->size.x;
	}

	if (player->pos.y < 0) {
		player->pos.y = 0;
	}
	else if (player->pos.y + player->size.y > LEVEL_HEIGHT) {
		player->pos.y = LEVEL_HEIGHT - player->size.y;
	}
}
// przypina kamere do gracza i trzyma jπ w granicach
void setCamera(engine* game, character* player1) {
	game->camera.x = player1->pos.x - WINDOW_WIDTH / 2 + player1->size.x / 2;
	game->camera.y = player1->pos.y - WINDOW_HEIGHT / 2 + player1->size.y / 2;

	if (game->camera.x < 0) {
		game->camera.x = 0;
	}
	else if (game->camera.x > LEVEL_WIDTH - WINDOW_WIDTH) {
		game->camera.x = LEVEL_WIDTH - WINDOW_WIDTH;
	}
	if (game->camera.y < 0) {
		game->camera.y = 0;
	}
	else if (game->camera.y > LEVEL_HEIGHT - WINDOW_HEIGHT) {
		game->camera.y = LEVEL_HEIGHT - WINDOW_HEIGHT;
	}
}
// porusza graczem 
void movePlayer(engine* game, character* player1) {
	player1->pos.x += player1->velocity.x * PLAYER_SPEED_MULTIPLIER;
	player1->pos.y += player1->velocity.y * PLAYER_SPEED_MULTIPLIER;
	if (player1->velocity.x == 0 && player1->velocity.y < 0) {
		player1->direction = up;
	}
	else if (player1->velocity.x == 0 && player1->velocity.y > 0) {
		player1->direction = down;
	}
	else if (player1->velocity.x > 0 && player1->velocity.y == 0) {
		player1->direction = right;
	}
	else if (player1->velocity.x < 0 && player1->velocity.y == 0) {
		player1->direction = left;
	}
	else if (player1->velocity.x < 0 && player1->velocity.y < 0) {
		player1->direction = ul;
	}
	else if (player1->velocity.x > 0 && player1->velocity.y > 0) {
		player1->direction = dr;
	}
	else if (player1->velocity.x < 0 && player1->velocity.y > 0) {
		player1->direction = dl;
	}
	else if (player1->velocity.x > 0 && player1->velocity.y < 0) {
		player1->direction = ur;
	}

	playerOutsideBorder(player1);
	setCamera(game, player1);

	if (collision(&game->heart, player1) && !game->heart.collected) {
		player1->hp++;
		game->heart.collected = true;
	}
}
// porusza pociskami
void moveBullet(bullet* bullet) {
	bullet->pos.x += bullet->velocity.x * BULLET_SPEED;
	bullet->pos.y += bullet->velocity.y * BULLET_SPEED;
}
// porusza przeciwnikiem 
void moveEnemy(engine* game, character* enemy) {
	enemy->pos.x += enemy->velocity.x;
	enemy->pos.y += enemy->velocity.y;
	if (enemy->pos.y <= 0 || enemy->pos.y >= LEVEL_HEIGHT - enemy->size.y) {
		enemy->velocity.y *= -1;
	}
	if (enemy->pos.x <= 200 + enemy->size.x || enemy->pos.x >= LEVEL_WIDTH - 200 - enemy->size.x) {
		enemy->velocity.x *= -1;
	}
}
// mechanika pociskÛw gracza i przeciwnika
void handleBullets(engine* game, character* player1, character* enemy) {
	// strzaly przeciwnika
	for (int i = enemy->fired - 1; i >= 0; i--) {
		moveBullet(&enemy->bullets[i]);
		// sprawdzamy wyjscie pociskow za plansze
		if (bulletOutsideBoard(&enemy->bullets[i])) {
			enemy->bullets[i] = enemy->bullets[--enemy->fired];	// zamienia pocisk wystrzelony z niewystrzelonym
		}
		else if (game->gameTime.worldTime - game->gameTime.godMode >= 0) {
			player1->invincible = 0;
			if (collision(&enemy->bullets[i], player1)) {	//sprawdza czy gracz zostaje postrzelony
				game->scoreBonus = 1;
				game->score--;
				game->tmpScore--;
				enemy->hits++;
				player1->hp--;
				player1->invincible = 1;
				enemy->bullets[i] = enemy->bullets[--enemy->fired];
				game->gameTime.godMode = game->gameTime.worldTime + INVINCIBILITY_TIME;
			}
		}
	}
	// strzaly gracza
	for (int i = player1->fired - 1; i >= 0; i--) {
		moveBullet(&player1->bullets[i]);
		if (bulletOutsideBoard(&player1->bullets[i])) {
			player1->bullets[i] = player1->bullets[--player1->fired];
		}
		else if (collision(&player1->bullets[i], enemy)) { //sprawdza czy przeciwnik zostaje postrzelony
			player1->bullets[i] = player1->bullets[--player1->fired];
			player1->hits++;
			enemy->hp--;
			game->score += 1 * game->scoreBonus;
			game->scoreBonus *= SCORE_MULTIPLIER;
			game->tmpScore++;
		}
	}
}
// wysy≥a strza≥y gracza
void shootPlayer(engine* game, character* player1, character* enemy) {
	if (player1->shoot == 1) {
		if (player1->fired < MAX_PLAYER_BULLETS) {
			player1->bullets[player1->fired].size.x = P_BULLET_SIZE;
			player1->bullets[player1->fired].size.y = P_BULLET_SIZE;
			player1->bullets[player1->fired].pos.x = 
				player1->pos.x + player1->size.x / 2 - player1->bullets[player1->fired].size.x / 2;
			player1->bullets[player1->fired].pos.y = 
				player1->pos.y + player1->size.y / 2 - player1->bullets[player1->fired].size.y / 2;
			if (player1->shootDirection == right) {
				player1->bullets[player1->fired].velocity.x = 1 * player1->attack->speed;
				player1->bullets[player1->fired].velocity.y = 0 * player1->attack->speed;
				player1->bullets[player1->fired].direction = player1->shootDirection;
			}
			else if (player1->shootDirection == left) {
				player1->bullets[player1->fired].velocity.x = -1 * player1->attack->speed;
				player1->bullets[player1->fired].velocity.y = 0 * player1->attack->speed;
				player1->bullets[player1->fired].direction = player1->shootDirection;
			}
			else if (player1->shootDirection == down) {
				player1->bullets[player1->fired].velocity.x = 0 * player1->attack->speed;
				player1->bullets[player1->fired].velocity.y = 1 * player1->attack->speed;
				player1->bullets[player1->fired].direction = player1->shootDirection;
			}
			else if (player1->shootDirection == up) {
				player1->bullets[player1->fired].velocity.x = 0 * player1->attack->speed;
				player1->bullets[player1->fired].velocity.y = -1 * player1->attack->speed;
				player1->bullets[player1->fired].direction = player1->shootDirection;
			}
			player1->fired++;
		}
		player1->shoot = 0;
	}
}
// odpala pociski przeciwnika
void shootEnemy(timer* timer, character* enemy) {
	static int specialAttack = 0;
	double timeRandom = (double)(rand() % 50 - 10) / 10;
	double breakT = 0;

	if (specialAttack == 5) {
		breakT = 3;
	}
	if (timer->shootTimer == 0) {
		timer->shootTimer = timer->worldTime;
	}
	else if (timer->worldTime - timer->shootTimer >= enemy->attack[0].delay + timeRandom + breakT) {
		int no = 0;
		if (specialAttack >= 4 + rand() % 3) {	// losuje za ktorym razem nastapi atak specjalny trzeciego bossa
			if (enemy->boss) {
				no = 1;
			}
			specialAttack = 0;
		}
		else specialAttack++;
		// tu nastepuje odpalenie pocisku zaleznie od typu ataku 
		// shoot nadaje pociskowi koordynaty poczatkowe enemy i vector lotu
		for (int j = 0; j < enemy->attack[no].nOfBullets; j++) {
			if (enemy->fired < MAX_BULLETS) {
				enemy->bullets[enemy->fired].direction = up;
				enemy->bullets[enemy->fired].velocity.x = enemy->attack[no].velocity[j].x * enemy->attack->speed;
				enemy->bullets[enemy->fired].velocity.y = enemy->attack[no].velocity[j].y * enemy->attack->speed;
				enemy->bullets[enemy->fired].size.x = enemy->attack[no].size.x;
				enemy->bullets[enemy->fired].size.y = enemy->attack[no].size.y;
				enemy->bullets[enemy->fired].pos.x = enemy->pos.x + enemy->size.x / 2 - enemy->bullets[enemy->fired].size.x / 2;
				enemy->bullets[enemy->fired].pos.y = enemy->pos.y + enemy->size.y / 2 - enemy->bullets[enemy->fired].size.y / 2;
				enemy->fired++;
			}
		}
		timer->shootTimer = 0;
	}
}

void specialAttackHighlight(engine* game, character* player, character * enemy) {

	if ((game->highlightType == 0) && (game->gameTime.highlightDelay - game->gameTime.worldTime <=0)) {
		game->highlightType = 1 + (int)rand() % 2;
		game->highlights[game->highlightType-1].pos.x = rand() % 3600 + 200;
		game->highlights[game->highlightType-1].pos.y = rand() % 1600 + 200;
		game->gameTime.highlightDuration = game->gameTime.worldTime + 7;
		game->gameTime.highlightDelay = game->gameTime.worldTime + 13;
	}
	if (game->highlightType) {
		if (game->gameTime.highlightDuration - game->gameTime.worldTime <= 0) {
			if (collision(&game->highlights[game->highlightType-1], player)) {
				player->hp--;
				game->scoreBonus = 1;
				game->score--;
				game->tmpScore--;
				enemy->hits++;
			}
			game->highlightType = 0;
		}
	}
}
// sprawdza wejúcie
void checkInput(engine* game, character* player1) {
	while (SDL_PollEvent(&game->event)) {
		if (game->event.type == SDL_KEYDOWN
			&& game->event.key.repeat == 0) {
			if (game->event.key.keysym.sym == SDLK_UP) {
				player1->velocity.y -= player1->speed;
			}
			else if (game->event.key.keysym.sym == SDLK_DOWN) {
				player1->velocity.y += player1->speed;
			}
			else if (game->event.key.keysym.sym == SDLK_RIGHT) {
				player1->velocity.x += player1->speed;
			}
			else if (game->event.key.keysym.sym == SDLK_LEFT) {
				player1->velocity.x -= player1->speed;
			}
			else if (game->event.key.keysym.sym == SDLK_w) {
				player1->shoot = 1;
				player1->shootDirection = up;
			}
			else if (game->event.key.keysym.sym == SDLK_s) {
				player1->shoot = 1;
				player1->shootDirection = down;
			}
			else if (game->event.key.keysym.sym == SDLK_a) {
				player1->shoot = 1;
				player1->shootDirection = left;
			}
			else if (game->event.key.keysym.sym == SDLK_d) {
				player1->shoot = 1;
				player1->shootDirection = right;
			}
			else if (game->event.key.keysym.sym == SDLK_n) game->newGame = 1;
			else if (game->event.key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
		}
		else if (game->event.type == SDL_KEYUP && game->event.key.repeat == 0) {
			if (game->event.key.keysym.sym == SDLK_UP) player1->velocity.y += player1->speed;
			else if (game->event.key.keysym.sym == SDLK_DOWN) player1->velocity.y -= player1->speed;
			else if (game->event.key.keysym.sym == SDLK_RIGHT) player1->velocity.x -= player1->speed;
			else if (game->event.key.keysym.sym == SDLK_LEFT) player1->velocity.x += player1->speed;
		}
		else if (game->event.type == SDL_QUIT) {
			game->quit = 1;
		}
	}
}

void initPlayer(character* player1) {
	player1->size.x = PLAYER_WIDTH;
	player1->size.y = PLAYER_HEIGHT;
	player1->pos.x = 300;
	player1->pos.y = LEVEL_HEIGHT / 2 - player1->size.y / 2;
	player1->speed = 1;
	player1->fired = 0;
	player1->hits = 0;
	player1->hp = player1->maxHp = PLAYER_HP;
	player1->attack->speed = PLAYER_SHOOT_SPEED;
	player1->direction = up;
	player1->invincible = 0;
}

void initEnemy1(character* enemy) {
	enemy->boss = 0;
	enemy->velocity.x = 1;
	enemy->velocity.y = -3;
	enemy->hits = 0;
	enemy->hp = enemy->maxHp = ENEMY1_HP;
	enemy->fired = 0;
	enemy->size.x = ENEMY_WIDTH;
	enemy->size.y = ENEMY_HEIGHT;
	enemy->pos.x = LEVEL_WIDTH / 2 - enemy->size.x / 2;
	enemy->pos.y = LEVEL_HEIGHT / 2 - enemy->size.y / 2;
	createAttack1(&enemy->attack[0]);
}

void initEnemy2(character* enemy) {
	enemy->boss = 0;
	enemy->velocity.x = 0;
	enemy->velocity.y = 0;
	enemy->hits = 0;
	enemy->hp = enemy->maxHp = ENEMY2_HP;
	enemy->fired = 0;
	enemy->size.x = ENEMY_WIDTH;
	enemy->size.y = ENEMY_HEIGHT;
	enemy->pos.x = LEVEL_WIDTH - ENEMY_WIDTH;
	enemy->pos.y = LEVEL_HEIGHT / 2 - enemy->size.y / 2;
	createAttack2(&enemy->attack[0]);
}

void initEnemy3(character* enemy) {
	enemy->boss = 1;
	enemy->velocity.x = 3;
	enemy->velocity.y = -6;
	enemy->hits = 0;	
	enemy->fired = 0;
	enemy->hp = enemy->maxHp = ENEMY3_HP;
	enemy->size.x = ENEMY_WIDTH;
	enemy->size.y = ENEMY_HEIGHT;
	enemy->pos.x = 200 + enemy->size.x;
	enemy->pos.y = 100 + enemy->size.y;
	createAttack3(&enemy->attack[0]);
	createAttack1(&enemy->attack[1]);
}

void initTimer(timer* timer) {
	timer->fps = 0;	
	timer->frames = 0;
	timer->fpsTimer = 0;
	timer->hpTimer = 0;
	timer->shootTimer = 0;
	timer->scoreDisplay = 0;
	timer->godMode = 0;
	timer->worldTime = 0;
	timer->highlightDelay = timer->worldTime + 5;
	timer->highlightDuration = timer->worldTime +  5 ;	
	timer->megaDelay = timer->worldTime + 3;
	timer->tick2 = 0;	
	timer->tick1 = SDL_GetTicks();
}
// tworzy wzorce strza≥Ûw
void createAttack1(attackPattern* attack) {
	attack->size.x = BULLET_SIZE;
	attack->size.y = BULLET_SIZE;

	attack->speed = 3;
	attack->delay = 1.6;
	attack->nOfBullets = 8;

	attack->velocity[0].x = 1;
	attack->velocity[0].y = 0;

	attack->velocity[1].x = 0;
	attack->velocity[1].y = 1;

	attack->velocity[2].x = -1;
	attack->velocity[2].y = 0;

	attack->velocity[3].x = 0;
	attack->velocity[3].y = -1;

	attack->velocity[4].x = -1 / sqrt(2);
	attack->velocity[4].y = -1 / sqrt(2);

	attack->velocity[5].x = 1 / sqrt(2);
	attack->velocity[5].y = 1 / sqrt(2);

	attack->velocity[6].x = -1 / sqrt(2);
	attack->velocity[6].y = 1 / sqrt(2);

	attack->velocity[7].x = 1 / sqrt(2);
	attack->velocity[7].y = -1 / sqrt(2);
}

void createAttack2(attackPattern* attack) {
	attack->size.x = BULLET_SIZE;
	attack->size.y = BULLET_SIZE;

	attack->speed = 1;
	attack->delay = 1;
	attack->nOfBullets = 5;

	attack->velocity[0].x = -0.5;
	attack->velocity[0].y = -sqrt(3) / 2;

	attack->velocity[1].x = -sqrt(3) / 2;
	attack->velocity[1].y = -0.5;

	attack->velocity[2].x = -1;
	attack->velocity[2].y = 0;

	attack->velocity[3].x = -sqrt(3) / 2;
	attack->velocity[3].y = 0.5;

	attack->velocity[4].x = -0.5;
	attack->velocity[4].y = sqrt(3) / 2;

}

void createAttack3(attackPattern* attack) {
	attack->size.x = BULLET_SIZE;
	attack->size.y = BULLET_SIZE;

	attack->speed = 3;
	attack->delay = 0.9;
	attack->nOfBullets = 2;

	attack->velocity[0].x = 1;
	attack->velocity[0].y = 0;

	attack->velocity[1].x = -1;
	attack->velocity[1].y = 0;
}

void createAttack4(attackPattern* attack) {
	attack->size.x = BULLET_SIZE;
	attack->size.y = BULLET_SIZE;

	attack->speed = 2;
	attack->delay = 1;
	attack->nOfBullets = 8;

	attack->velocity[0].x = 1;
	attack->velocity[0].y = 0;

	attack->velocity[1].x = 0;
	attack->velocity[1].y = 1;

	attack->velocity[2].x = -1;
	attack->velocity[2].y = 0;

	attack->velocity[3].x = 0;
	attack->velocity[3].y = -1;

	attack->velocity[4].x = -1 / sqrt(2);
	attack->velocity[4].y = -1 / sqrt(2);

	attack->velocity[5].x = 1 / sqrt(2);
	attack->velocity[5].y = 1 / sqrt(2);

	attack->velocity[6].x = -1 / sqrt(2);
	attack->velocity[6].y = 1 / sqrt(2);

	attack->velocity[7].x = 1 / sqrt(2);
	attack->velocity[7].y = -1 / sqrt(2);
}
// generuje apteczkÍ
void placeHeart(engine* game, character* player) {
	if (game->heart.collected
		&& (game->gameTime.hpTimer - game->gameTime.worldTime <= 0)
		&& player->hp < PLAYER_HP)
	{
		game->heart.collected = false;
		game->heart.size.x = 50;
		game->heart.size.y = 50;
		game->heart.pos.x = (rand() % (LEVEL_WIDTH - 400)) + 200;
		game->heart.pos.y = (rand() % (LEVEL_HEIGHT - 300)) + 150;
		game->gameTime.hpTimer = game->gameTime.worldTime + 10 + rand()%10;
	}
}

void newGame(engine* game, character* player, character* enemy, int stage) {
	game->stage = stage;
	game->heart.collected = true;
	game->tmpScore = 0;
	game->scoreBonus = 1;
	game->printScore = false;
	game->newGame = 0;
	game->quit = 0;
	game->camera.x = game->camera.y = 0;
	game->gameTime.shootTimer = 0;
	game->activeBg = &game->background1;

	game->highlightType = 0;
	game->highlights[0].pos.x = game->highlights[0].pos.y = 0;
	game->highlights[0].size.x = 2000;
	game->highlights[0].size.y = 60;

	game->highlights[1].pos.x = game->highlights[1].pos.y = 0;
	game->highlights[1].size.x = 600;
	game->highlights[1].size.y = 600;

	game->megaBulletActive = 0;

	initTimer(&game->gameTime);
	initPlayer(player);
	initEnemy1(enemy);

	if (game->stage == 1) {
		initEnemy1(enemy);
		game->activeBg = &game->background1;
		game->activeEnemy = &game->enemy1Tex;
	}
	else if (game->stage == 2) {
		initEnemy2(enemy);
		game->activeBg = &game->background2;
		game->activeEnemy = &game->enemy2Tex;
	}
	else if (game->stage == 3) {
		initEnemy3(enemy);
		game->activeBg = &game->background3;
		game->activeEnemy = &game->enemy3Tex;
	}
}

void changeLevel(engine* game, character* player1, character* enemy, int number) {
	if (game->stage == number - 1 && enemy->hp<=0) {
		newGame(game, player1, enemy, number);
		game->printScore = true;
		game->gameTime.scoreDisplay = game->gameTime.worldTime + 5;
	}
}
// wysy≥a pojedyÒczy duøy pocisk
void specialAttackMega(engine* game, character* player, character* enemy) {
	
	if ((game->megaBulletActive == 0) && (game->gameTime.megaDelay - game->gameTime.worldTime <= 0)) {
		game->megaBulletActive = 1;
		
		game->megaBullet.velocity.x = -2;
		game->megaBullet.velocity.y = 0;

		game->megaBullet.pos.x = enemy->pos.x;
		game->megaBullet.pos.y = enemy->pos.y;

		game->megaBullet.size.x = MEGA_BULLET_SIZE;
		game->megaBullet.size.y = MEGA_BULLET_SIZE;
	}

	if (game->megaBulletActive) {
		game->megaBullet.pos.x += game->megaBullet.velocity.x;
		game->megaBullet.pos.y += game->megaBullet.velocity.y;
			if (collision(&game->megaBullet, player)) {
				player->hp--;
				enemy->hits++;
				game->scoreBonus = 1;
				game->score-=10;
				game->tmpScore-=10;
				game->megaBulletActive = 0;
				game->gameTime.megaDelay = game->gameTime.worldTime + rand() % 8 + 4;
			} else if (bulletOutsideBoard(&game->megaBullet)) {
				game->megaBulletActive = 0;
				game->gameTime.megaDelay = game->gameTime.worldTime + rand()% 8 + 4;
			}	
	}
}