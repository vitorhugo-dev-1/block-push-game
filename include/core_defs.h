#pragma once

// Maximum number of shots allowed
#define MAX_SHOTS 1

// Window default settings
#define WINDOW_TITLE     "Project"
#define WINDOW_WIDTH     800
#define WINDOW_HEIGHT    600
#define TARGET_FPS       60
#define UI_TEXT_OFFSET_X 270
#define UI_TEXT_OFFSET_Y 250
#define UI_TEXT_SIZE     30

// File paths
#define MAP_FILENAME     "map.csv"
#define TEXTURE_FOLDER   "./images"

// Sprite size related information
#define TILE_SIZE 64
#define DEFAULT_TEXTURE_SIZE 16
#define HALF_SIZE (TILE_SIZE / 2)
#define PIXEL_SCALE (TILE_SIZE / DEFAULT_TEXTURE_SIZE)

// Number of chars in each cell in the grid
#define CELL_SIZE 2

// Math related macros
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))
#define M_PI 3.14159265358979323846
