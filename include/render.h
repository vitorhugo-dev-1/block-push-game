#pragma once

#include <core_defs.h>
#include <types.h>
#include <map_data.h>
#include <util.h>
#include <raylib.h>

// Loads an image file and returns a texture
Texture2D LoadTextureFromFile(const char *fileName){
    Image image = LoadImage(fileName);
    ImageResizeNN(&image, image.width * PIXEL_SCALE, image.height * PIXEL_SCALE);

    Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture
    UnloadImage(image);  // Unload image from CPU memory (image data is now in GPU memory)

    return texture;
}

// Loads images from a folder which match a TileType value into an array of textures
void LoadTexturesFromFolder(const char *folderPath, Texture2D *textures){
    FilePathList files = LoadDirectoryFiles(folderPath);
    if (!files.count) Error("Could not open texture directory: %s", folderPath);

    for (unsigned int i = 0; i < files.count; i++){
        const char *fileName = strrchr(files.paths[i], '\\') + 1;
        if (!fileName || strlen(fileName) < 3) continue;

        int objectCode = atoi(fileName + 1);
        if (objectCode >= TILE_END){
            printf("WARNING: Texture with ID=%d is not a known object. Skipping.\n", objectCode);
            continue;
        }

        textures[objectCode] = LoadTextureFromFile(files.paths[i]);
    }

    for (unsigned int i = 0; i < TILE_END; i++){
        if (!textures[i].height) printf("WARNING: Texture ID=%d is missing.\n", i);
    }

    UnloadDirectoryFiles(files);
}

// Draws a texture with the specified rotation
void DrawRotatedTexture(Texture2D texture, Vector2 position, int rotation, Color color){
    Vector2 origin = (Vector2){ HALF_SIZE, HALF_SIZE };
    Rectangle src = { 0, 0, texture.width, texture.height };
    Rectangle dst = { position.x, position.y, texture.width, texture.height };
    DrawTexturePro(texture, src, dst, origin, rotation, color);
}

// Initializes Animation with the informed values
static inline Animation InitAnimValues(Texture2D *texture, int startRow, float duration){
    int cols = texture->width / TILE_SIZE;
    int rows = texture->height / TILE_SIZE;

    Animation anim = {
        .texture          = texture,
        .startRow         = startRow,
        .frameWidth       = TILE_SIZE,
        .frameHeight      = TILE_SIZE,
        .frameCount       = cols * rows,
        .currentFrame     = 0,
        .frameDuration    = duration,
        .elapsedTime      = 0.0f,
        .rotation         = 0.0f,
        .durationModifier = 1
    };
    return anim;
};

// Updates the animation frame and rotation based on the elapsed time
static inline void UpdateAnimation(Animation *anim, float rotationMod){
    anim->elapsedTime += deltaTime;
    if (anim->elapsedTime >= anim->frameDuration / anim->durationModifier){
        anim->elapsedTime = 0.0f;
        anim->currentFrame = (anim->currentFrame + 1) % anim->frameCount;
        anim->rotation = (anim->rotation == 360 ? rotationMod : anim->rotation + rotationMod);
    }
}

// Function that updates an animation and draws it into the specified coordinates
void DrawAnimation(Animation *anim, Vector2 pos, Color color, bool halved){
    UpdateAnimation(anim, 0.0f);

    // Find, within the texture, the coordinates of the frame that will be displayed
    int framesPerRow = anim->texture->width / anim->frameWidth;

    int frameColumn = anim->currentFrame % framesPerRow;
    int frameRow    = anim->currentFrame / framesPerRow;

    int frameX = frameColumn * anim->frameWidth;
    int frameY = frameRow * anim->frameHeight + anim->startRow;

    Rectangle source = { frameX, frameY, anim->frameWidth, anim->frameHeight };
    Rectangle dest   = { pos.x, pos.y, anim->frameWidth, anim->frameHeight };
    Vector2   origin = { 0 };

    if (halved) source.height = dest.height = anim->frameHeight / 2;

    DrawTexturePro(*anim->texture, source, dest, origin, anim->rotation, color);
}

// Function that updates rotates a sprite and draws the result into the specified coordinates
void RotateAndDrawAnimation(Animation* anim, Vector2 position, Color color){
    Vector2 dst = { position.x + HALF_SIZE, position.y + HALF_SIZE };
    DrawRotatedTexture(*anim->texture, dst, anim->rotation, color);
}

// Draws every element from the loaded map
void DrawMap(const Map *map, const Texture2D textures[], Animation anim[]){
    // Walls
    Vector2 *walls = (Vector2 *)map->objects[TILE_WALL];
    for (unsigned int i = 0; i < map->length[TILE_WALL]; i++){
        DrawTextureV(textures[TILE_WALL], walls[i], GRAY);
    }

    // Puddles
    Vector2 *puddles = (Vector2 *)map->objects[TILE_PUDDLE];
    for (unsigned int i = 0; i < map->length[TILE_PUDDLE]; i++){
        DrawRectangleV(puddles[i], (Vector2){ TILE_SIZE, TILE_SIZE }, BLUE);
    }

    // Conveyor Belts
    DirItem *conveyors = (DirItem *)map->objects[TILE_CONVEYOR];
    for (unsigned int i = 0; i < map->length[TILE_CONVEYOR]; i++){
        int rotation = (conveyors[i].dir * 90) - 90;

        Vector2 position = (Vector2){
            conveyors[i].position.x + HALF_SIZE,
            conveyors[i].position.y + HALF_SIZE
        };

        DrawRotatedTexture(textures[TILE_CONVEYOR], position, rotation, WHITE);
    }

    // Doors
    Item *doors = (Item *)map->objects[TILE_DOOR];
    Rectangle doorFrame = { 0.0f, 0.0f, textures[TILE_DOOR].width / 2, textures[TILE_DOOR].height };
    for (unsigned int i = 0; i < map->length[TILE_DOOR]; i++){
        doorFrame.x = doors[i].isActive ? 64 : 0;
        DrawTextureRec(textures[TILE_DOOR], doorFrame, doors[i].position, WHITE);
    }

    // Keys
    Item *keys = (Item *)map->objects[TILE_KEY];
    for (unsigned int i = 0; i < map->length[TILE_KEY]; i++){
        if (!keys[i].isActive) DrawTextureV(textures[TILE_KEY], keys[i].position, YELLOW);
    }

    // Portals
    Portals *portals = (Portals *)map->objects[TILE_PORTAL];
    UpdateAnimation(&anim[ANIM_PORTAL], -90.0f);
    for (unsigned int i = 0; i < map->length[TILE_PORTAL]; i++){
        Color   color    = i&1 ? (Color){ 128, 0, 255, 255 } : GREEN;
        RotateAndDrawAnimation(&anim[ANIM_PORTAL], portals[i].entrance, color);
        RotateAndDrawAnimation(&anim[ANIM_PORTAL], portals[i].exit    , color);
    }

    // Player
    Entity *players = (Entity *)map->objects[TILE_PLAYER];
    char ***array = map->array;
    int rows = map->rows;
    int cols = map->cols;

    bool halved = false;
    int gridX = players[0].box.x / TILE_SIZE;
    int gridY = players[0].box.y / TILE_SIZE;
    int minY, maxY, minX, maxX;
    minY = maxY = gridY;
    minX = maxX = gridX;

    if (atoi(array[gridY][gridX]) == TILE_PUDDLE){ // Checks if player is inside puddle
        if (gridY < rows) maxY += atoi(array[gridY+1][ gridX ]) == TILE_PUDDLE;
        if (gridX < cols) maxX += atoi(array[ gridY ][gridX+1]) == TILE_PUDDLE;
        if (gridY > 0)    minY -= atoi(array[gridY-1][ gridX ]) == TILE_PUDDLE;
        if (gridX > 0)    minX -= atoi(array[ gridY ][gridX-1]) == TILE_PUDDLE;

        if (
            players[0].spr.x >= minX * TILE_SIZE &&
            players[0].spr.x <= maxX * TILE_SIZE &&
            players[0].spr.y >= minY * TILE_SIZE &&
            players[0].spr.y <= maxY * TILE_SIZE
        ) halved = true;
    }
    DrawAnimation(&anim[ANIM_PLAYER], players[0].spr, BLUE, halved);

    // Crates
    Entity *crates = (Entity *)map->objects[TILE_CRATE];
    for (unsigned int i = 0; i < map->length[TILE_CRATE]; i++){
        DrawTextureV(textures[TILE_CRATE], crates[i].spr, BROWN);
    }
}

static void RenderGame(Map *map, Assets assets, ScreenContext ctx){
    HideCursor();
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(*ctx.camera);

    Vector2 UIPosition = (Vector2){
        ctx.camera->target.x + UI_TEXT_OFFSET_X,
        ctx.camera->target.y + UI_TEXT_OFFSET_Y
    };

    DrawMap(map, assets.textures, assets.anim);
    DrawText(ctx.keysUI->text, UIPosition.x, UIPosition.y, UI_TEXT_SIZE, WHITE);
    DrawRotatedTexture(assets.textures[0], ctx.mouse.position, ctx.mouse.angle, WHITE);
}