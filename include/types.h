#pragma once

#include <raylib.h>
#include <stdbool.h>

// Game tile types
typedef enum {
    TILE_EMPTY,
    TILE_PLAYER,
    TILE_WALL,
    TILE_CRATE,
    TILE_DOOR,
    TILE_KEY,
    TILE_PUDDLE,
    TILE_CONVEYOR,
    TILE_PORTAL,
    TILE_END
} TileType;

// Animated tile types
typedef enum {
    ANIM_PLAYER,
    ANIM_PORTAL,
    ANIM_END
} AnimatedTile;

// Specific tile information
typedef struct { unsigned int x, y, index; char code[CELL_SIZE + 1]; } TileInfo;

// Movement directions
typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} Direction;

// Structs for the different map objects
typedef struct { Vector2 box, spr; } Entity;
typedef struct { Vector2 entrance, exit; } Portals;
typedef struct { Vector2 position; bool isActive; } Item;
typedef struct { Vector2 position; Direction dir; } DirItem;

// Animation data
typedef struct {
    Texture2D *texture;          // Sprite sheet
    unsigned int startRow;       // Texture row from where each frame will be displayed
    unsigned int frameWidth;
    unsigned int frameHeight;
    unsigned int frameCount;     // Total number of frames
    unsigned int currentFrame;
    float frameDuration;         // Duration of each frame in seconds
    float elapsedTime;
    float rotation;
    int durationModifier;        // Speed modifier for the animation
} Animation;

// Visual assets
typedef struct{
    Texture2D *textures;
    Animation *anim;
} Assets;

// Window info
typedef struct { char title[20]; unsigned int width, height, vCenter, hCenter; } Window;

// Mouse cursor
typedef struct{ Vector2 position; float angle;} Cursor;

// UI data
typedef struct {
    char text[20];
    int value;
} UIElement;

// Camera and HUD elements
typedef struct{
    UIElement *keysUI;
    Camera2D *camera;
    Cursor mouse;
} ScreenContext;

// Parameters for movement functions
typedef struct {
    Direction dir;
    int speed, timer;
} MoveParams;