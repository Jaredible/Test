#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define LEVEL_WIDTH 128
#define LEVEL_HEIGHT 128

struct Screen
{
	int width;
	int height;
	int* pixels;
	int offsetX;
	int offsetY;
};

struct World
{
	struct Level* levels;
};

struct Level
{
	int width;
	int height;
	int* tiles;
	int* data;
	struct Entity* player;
	struct Room** rooms;
	int startX;
	int startY;
	int endX;
	int endY;
};

struct Room
{
};

struct Entity
{
	int x;
	int y;
};

struct Entity* createEntity(int x, int y)
{
	struct Entity* entity = (struct Entity*) malloc(sizeof(struct Entity));
	entity->x = x;
	entity->y = y;
	return entity;
}

void setOffset(struct Screen* screen, int offsetX, int offsetY)
{
	screen->offsetX = offsetX;
	screen->offsetY = offsetY;
}

int generateWorld()
{
}

int generateLevel(struct Level* level, unsigned int seed)
{
	int i = 10;
	srand(seed);
	
	int first = 1;
	while (i > 0)
	{
		break; // remove
		if (canPlaceRoom(first))
		{
			i--;
			first = 0;
		}
	}
	level->endX = level->startX;
	level->endY = level->startY;
	
	int dx, dy;
	float dist;
	float bestDist = 0;
	struct Room* room;
	size_t numRooms = sizeof(level->rooms) / sizeof(int);
	for (i = 0; i < numRooms; i++)
	{
		room = level->rooms[i];
	}
	
	for (i = 0; i < 60; i++)
	{
		setTile(level, i, i, 65 + i, 0);
	}
	
	return 1;
}

int canPlaceRoom(int first)
{
	return 1;
}

int loadLevel(struct Level* level)
{
	FILE* fd = fopen("level.dat", "r");
	
	if (!fd)
	{
		return 0;
	}
	
	while (fread(&level, sizeof(level), 1, fd))
	{
	}
	
	fclose(fd);
	
	return 1;
}

int saveLevel(struct Level* level)
{
	FILE* fd = fopen("level.dat", "w");
	
	if (!fd)
	{
		return 0;
	}
	
	fwrite(&level, sizeof(level), 1, fd);
	
	fclose(fd);
	
	return 1;
}

struct Screen* createScreen(int width, int height)
{
	struct Screen* screen = (struct Screen*) malloc(sizeof(struct Screen));
	screen->width = width;
	screen->height = height;
	screen->pixels = (int*) malloc(sizeof(int) * width * height);
	return screen;
}

void clearScreen(struct Screen* screen)
{
	int i;
	for (i = 0; i < screen->width * screen->height; i++)
	{
		screen->pixels[i] = ' ';
	}
}

void render(struct Screen* screen, int xp, int yp, int tile)
{
	xp -= screen->offsetX;
	yp -= screen->offsetY;
	
	int x, y;
	for (y = 0; y < 8; y++)
	{
		int ys = y;
		if (y + yp < 0 || y + yp >= screen->height)
		{
			continue;
		}
		for (x = 0; x < 8; x++)
		{
			int xs = x;
			if (x + xp < 0 || x + xp >= screen->width)
			{
				continue;
			}
			int p = tile;
			if (p < 255) screen->pixels[(x + xp) + (y + yp) * screen->width] = p;
		}
	}
}

struct Level* createLevel(int width, int height, int numRooms)
{
	struct Level* level = (struct Level*) malloc(sizeof(struct Level));
	level->width = width;
	level->height = height;
	level->tiles = (int*) malloc(sizeof(int) * width * height);
	level->data = (int*) malloc(sizeof(int) * width * height);
	level->rooms = (struct Room**) malloc(sizeof(struct Room*) * numRooms);
	int i;
	for (i = 0; i < width * height; i++)
	{
		level->tiles[i] = ' ';
	}
	return level;
}

int getTile(struct Level* level, int x, int y)
{
	if (x < 0 || y < 0 || x >= level->width || y >= level->height) return 0;
	return level->tiles[x + y * level->width];
}

int setTile(struct Level* level, int x, int y, int tile, int data)
{
	if (x < 0 || y < 0 || x >= level->width || y >= level->height) return 0;
	level->tiles[x + y * level->width] = tile;
	level->data[x + y * level->height] = data;
	return 1;
}

int getData(struct Level* level, int x, int y)
{
	if (x < 0 || y < 0 || x >= level->width || y >= level->height) return 0;
	return level->data[x + y * level->width];
}

int setData(struct Level* level, int x, int y, int data)
{
	if (x < 0 || y < 0 || x >= level->width || y >= level->height) return 0;
	level->data[x + y * level->width];
	return 1;
}

void printTile(struct Screen* screen, struct Level* level, int x, int y)
{
	int tile = getTile(level, x, y);
	render(screen, x * 8, y * 8, tile);
}

void printLevel(struct Screen* screen, struct Level* level, int scrollX, int scrollY)
{
	int xo = scrollX >> 3;
	int yo = scrollY >> 3;
	int w = (screen->width + 7) >> 3;
	int h = (screen->height + 7) >> 3;
	setOffset(screen, scrollX, scrollY);
	int x, y;
	for (y = yo; y <= h + yo; y++)
	{
		for (x = xo; x <= w + xo; x++)
		{
			printTile(screen, level, x, y);
		}
	}
	setOffset(screen, 0, 0);
}

void printEntity(struct Screen* screen, struct Level* level, struct Entity* entity)
{
	render(screen, entity->x, entity->y, '#');
}

void printEntities(struct Screen* screen, struct Level* level, int scrollX, int scrollY)
{
	setOffset(screen, scrollX, scrollY);
	printEntity(screen, level, level->player);
	setOffset(screen, scrollX, scrollY);
}

void test()
{
	//struct Level* before = createLevel(128, 128);
	//struct Level* after;
	//saveLevel(before);
	//loadLevel(after);
	//printf("level width: %d\n", after->width);
	//printf("level height: %d\n", after->height);
}

void updateGame(struct Level* level)
{
}

void renderGame(int* pixels, struct Screen* screen, struct Level* level, struct Entity* player)
{
	system("clear");
	
	clearScreen(screen);
	
	int scrollX = player->x - (screen->width - 8) / 2;
	int scrollY = player->y - (screen->height - 8) / 2;
	if (scrollX < 0)
	{
		scrollX = 0;
	}
	if (scrollY < 0)
	{
		scrollY = 0;
	}
	if (scrollX > level->width * 8 - screen->width)
	{
		scrollX = level->width * 8 - screen->width;
	}
	if (scrollY > level->height * 8 - screen->height)
	{
		scrollY = level->height * 8 - screen->height;
	}
	
	printLevel(screen, level, scrollX, scrollY);
	printEntities(screen, level, scrollX, scrollY);
	
	int x, y;
	for (y = 0; y < screen->height; y++)
	{
		for (x = 0; x < screen->width; x++)
		{
			int p = screen->pixels[x + y * screen->width];
			if (p < 255)
			{
				pixels[x + y * SCREEN_WIDTH] = p;
			}
		}
	}
	
	for (y = 0; y < screen->height; y++)
	{
		for (x = 0; x < screen->width; x++)
		{
			printf("%c ", pixels[x + y * screen->width]);
		}
		printf("\n");
	}
}

int main(const int argc, char* const argv[])
{
	//test();
	//return 0;
	
	system("clear");
	
	int* pixels = (int*) malloc(sizeof(int) * SCREEN_WIDTH * SCREEN_HEIGHT);
	struct Screen* screen = createScreen(SCREEN_WIDTH, SCREEN_HEIGHT);
	struct Level* level = createLevel(LEVEL_WIDTH, LEVEL_HEIGHT, 10);
	struct Entity* player = createEntity(0, 0);
	
	level->player = player;
	generateLevel(level, 0);
	
	system("/bin/stty raw");
	
	int i = 0;
	int c;
	do
	{
		switch (c)
		{
			case 'w':
				player->y -= 8;
				break;
			case 's':
				player->y += 8;
				break;
			case 'a':
				player->x -= 8;
				break;
			case 'd':
				player->x += 8;
				break;
			default:
				break;
		}
		
		system("/bin/stty cooked");
		updateGame(level);
		renderGame(pixels, screen, level, player);
		// TODO: debug info
		printf("x: %d y: %d\n", player->x, player->y);
		system("/bin/stty raw");
	} while ((c = getchar()) != '.');
	
	system("/bin/stty cooked");
	
	free(pixels);
	free(screen);
	free(level);
	
	return 0;
}
