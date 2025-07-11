#pragma once

#define MAX_SHOTS 1
#define CELL_SIZE 2
#define TILE_SIZE 64
#define HALF_SIZE (TILE_SIZE/2)

#include <raylib.h>

//List containing all of game's tile objects
typedef enum Dictionary {
    EMPTY,
    PLAYER,
    WALL,
    CRATE,
    DOOR,
    KEY,
    PUDDLE,
    CONVEYOR,
    PORTAL,
    END
} Dictionary;

typedef enum Direction {
    NOWHERE,
    UP,
    RIGHT,
    DOWN,
    LEFT
} Direction;

//Declaring structs for map objects
typedef struct Entity  { Vector2 box, spr; } Entity;
typedef struct Portals { Vector2 entrance, exit; } Portals;
typedef struct Item    { Vector2 position; bool active; } Item;
typedef struct DirItem { Vector2 position; Direction dir; } DirItem;

typedef struct Animation {
    Texture2D *texture;   //Sprite sheet
      int startingRow;    //Texture row from where each frame will be displayed
      int frameWidth;
      int frameHeight;
      int frameCount;     // Total number of frames
      int currentFrame;
    float frameDuration;  // Duration of each frame in seconds
    float frameTimer;
    float rotation;
} Animation;

typedef struct Window {
    char title[20];
    int width, height, center_w, center_h;
} Window;

typedef struct UI_Element {
    char display[20];
    int counter;
} UI_Element;

typedef struct TileInfo {
    int x, y, index;
    char code[CELL_SIZE+1];
} TileInfo;

//Initializes Animation struct with the informed values
Animation InitAnimValues(Texture2D *texture, int startingRow, float duration, int totalFrames){
    Animation anim = {
        .texture       = texture,
        .startingRow   = startingRow,
        .frameWidth    = TILE_SIZE,
        .frameHeight   = TILE_SIZE,
        .frameCount    = totalFrames,
        .currentFrame  = 0,
        .frameDuration = duration,
        .frameTimer    = 0.0f
    };
    return anim;
};

int getDictionaryValue(char ch){
    switch (ch){
        case 'C': return CONVEYOR;
        case 'P': return PORTAL;
        default : return false;
    }
}